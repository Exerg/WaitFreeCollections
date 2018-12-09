#ifndef WF_WAIT_FREE_UNORDERED_MAP
#define WF_WAIT_FREE_UNORDERED_MAP

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <climits>
#include <cassert>
#include <cmath>
#include <optional>
#include <functional>

namespace wf
{
	template <typename T>
	T clz(T x) // FIXME
	{
		assert(x != 0); // clz is undefined for 0

		if constexpr (std::is_same_v<T, unsigned int>)
		{
			return static_cast<T>(__builtin_clz(x));
		}
		else if constexpr (std::is_same_v<T, unsigned long int>)
		{
			return static_cast<T>(__builtin_clzl(x));
		}
		else if constexpr (std::is_same_v<T, unsigned long long int>)
		{
			return static_cast<T>(__builtin_clzll(x));
		}
		else
		{
			static_assert(!std::is_same_v<T, T>);
		}
	}

	template <typename Key>
	class identity_hash
	{
	public:
		std::size_t operator()(const Key& k) const
		{
			return static_cast<std::size_t>(k);
		}
	};

	template <typename Key, typename Value, typename HashFunction = identity_hash<Key>>
	class unordered_map
	{
	public:
		using key_t = Key;
		using hash_t = std::invoke_result_t<HashFunction, Key>;
		using value_t = Value;

		explicit unordered_map(std::size_t initialSize);
		unordered_map(const unordered_map&) = delete;
		~unordered_map() noexcept = default;

		unordered_map& operator=(const unordered_map&) = delete;

		bool insert(const Key& key, const Value& value);

		std::optional<Value> get(const Key& key);

		bool update(const Key& key, const Value& value, Value& expected_value); // TODO

		bool update(const Key& key, const Value& value); // TODO

		bool remove(const Key& key); // TODO

		bool remove(const Key& key, Value& expected_value); // TODO

		/**
		 * Applies functor on every element in the map.
		 * This function is NOT thread safe.
		 * @tparam VisitorFun The type should be compatible with this prototype void(const std::pair<const hash_t, value_t>&);
		 * @param fun
		 */
		template <typename VisitorFun>
		void visit(VisitorFun&& fun) noexcept(
		    noexcept(std::is_nothrow_invocable_v<VisitorFun, const std::pair<const hash_t, value_t>&>));

		/**
		 * Returns the number of elements into the collection
		 */
		std::size_t size() const noexcept;

		bool is_empty() const noexcept;

	private:
		struct node__;
		struct arraynode__;
		union node_ptr;

		using node_t = node__;
		using arraynode_t = arraynode__;

		struct node__
		{
			hash_t hash;
			value_t value;
		};

		struct arraynode__
		{
			using value_t = std::atomic<node_ptr>;
			using reference_t = value_t&;

			explicit arraynode__(std::size_t size);

			arraynode__(const arraynode__&) noexcept = delete;
			~arraynode__() noexcept;

			arraynode__& operator=(const arraynode__&) = delete;

			reference_t operator[](std::size_t i);

		private:
			value_t* m_ptr;
			std::size_t m_size;
		};

		union node_ptr
		{
			node_ptr() noexcept;
			explicit node_ptr(node_t* datanode) noexcept;
			explicit node_ptr(arraynode_t* arraynode) noexcept;

			node_t* datanode_ptr;
			arraynode_t* arraynode_ptr;
			std::uintptr_t ptr_int;
		};

		node_ptr allocate_node(hash_t hash, value_t value) const;

		node_ptr expandNode(node_ptr arraynode, std::size_t position, std::size_t level) noexcept;

		template <typename VisitorFun>
		void visit_array_node(node_ptr node, VisitorFun&& fun) noexcept(
		    noexcept(std::is_nothrow_invocable_v<VisitorFun, const std::pair<const hash_t, value_t>&>));

		std::tuple<std::size_t, hash_t> compute_pos_and_hash(size_t array_pow, hash_t lasthash, size_t level) const;

		constexpr static std::size_t log2_power_two(std::size_t x) noexcept;

		static bool is_power_of_two(std::size_t nbr) noexcept;

		static node_ptr mark_datanode(node_ptr arraynode, std::size_t position) noexcept;

		static void mark_datanode(node_ptr& node) noexcept;

		static void unmark_datanode(node_ptr& node) noexcept;

		static bool is_marked(node_ptr node) noexcept;

		static void mark_arraynode(node_ptr& node) noexcept;

		static void unmark_arraynode(node_ptr& node) noexcept;

		static bool is_array_node(node_ptr node) noexcept;

		static node_ptr get_node(node_ptr arraynode, std::size_t pos) noexcept;

		static node_ptr sanitize_ptr(node_ptr arraynode) noexcept;

		arraynode_t m_head;
		std::size_t m_head_size;
		std::size_t m_arrayLength;
		std::atomic<std::size_t> m_size;
		static constexpr std::size_t hash_size_in_bits = sizeof(hash_t) * std::numeric_limits<unsigned char>::digits;
	};

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::node_ptr::node_ptr() noexcept : datanode_ptr{nullptr}
	{
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::node_ptr::node_ptr(node_t* datanode) noexcept : datanode_ptr{datanode}
	{
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::node_ptr::node_ptr(arraynode_t* arraynode) noexcept
	    : arraynode_ptr{arraynode}
	{
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::arraynode__::arraynode__(std::size_t size)
	    : m_ptr{new value_t[size]}, m_size(size)
	{
		for (std::size_t i = 0; i < size; ++i)
		{
			new (&m_ptr[i]) value_t();
		}
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::arraynode__::~arraynode__() noexcept
	{
		for (std::size_t i = 0; i < m_size; ++i)
		{
			node_ptr child = m_ptr[i].load();
			if (child.arraynode_ptr != nullptr)
			{
				if (is_array_node(child))
				{
					delete sanitize_ptr(child).arraynode_ptr;
				}
				else
				{
					delete child.datanode_ptr;
				}
			}
		}

		delete[] m_ptr;
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::arraynode__::operator[](std::size_t i) -> value_t&
	{
		return m_ptr[i];
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::unordered_map(std::size_t initialSize)
	    : m_head(2UL << initialSize), m_head_size(2UL << initialSize), m_arrayLength(initialSize), m_size(0UL)
	{
		static_assert(std::atomic<std::size_t>::is_always_lock_free, "Atomic implementation is not lock free");
		static_assert(std::atomic<node_ptr>::is_always_lock_free, "Atomic implementation is not lock free");

		if (!is_power_of_two(initialSize))
		{
			throw std::runtime_error("Size should be a power of four");
		}
	}

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::insert(const Key& key, const Value& value)
	{
		std::size_t array_pow = log2_power_two(m_arrayLength);

		std::size_t position;
		std::size_t failCount;
		node_ptr local{&m_head};
		mark_arraynode(local);

		hash_t fullhash = HashFunction{}(key);
		hash_t hash = fullhash;

		for (std::size_t r = 0; r < hash_size_in_bits - array_pow; r += array_pow)
		{
			failCount = 0;
			std::tie(position, hash) = compute_pos_and_hash(array_pow, hash, r);
			node_ptr node = get_node(local, position);

			while (true)
			{
				if (failCount > 8) // FIXME
				{
					node = mark_datanode(local, position);
				}
				if (node.datanode_ptr == nullptr)
				{
					node_ptr node_to_insert = allocate_node(fullhash, value);
					node_ptr null{static_cast<node_t*>(nullptr)};

					if ((*sanitize_ptr(local).arraynode_ptr)[position].compare_exchange_weak(null, node_to_insert))
					{
						// watch(nullptr);
						++m_size;
						return true;
					}
					else
					{
						delete node_to_insert.datanode_ptr;
					}
				}

				if (is_marked(node))
				{
					node = expandNode(local, position, r);
				}
				if (is_array_node(node))
				{
					local = node;
					break;
				}
				else
				{
					node_ptr node2 = get_node(local, position);
					if (node.ptr_int != node2.ptr_int)
					{
						++failCount;
						node = node2;
					}
					else
					{
						if (node.datanode_ptr->hash == fullhash)
						{
							// watch(nullptr);
							return false;
						}
						else
						{
							node = expandNode(local, position, r);
							if (is_array_node(node))
							{
								local = node;
								break;
							}
							else
							{
								++failCount;
							}
						}
					}
				}
			}
		}

		// watch(nullptr);
		position = hash & (m_arrayLength - 1);
		node_ptr node = get_node(local, position);

		if (node.datanode_ptr == nullptr)
		{
			node_ptr node_to_insert = allocate_node(fullhash, value);
			node_ptr null{static_cast<node_t*>(nullptr)};

			bool inserted = (*sanitize_ptr(local).arraynode_ptr)[position].compare_exchange_weak(null, node_to_insert);
			if (inserted) {
			    ++m_size;
			}

            return inserted;
		}
		else
		{
			return false;
		}
	}

    template <typename Key, typename Value, typename HashFunction>
    std::optional<Value> unordered_map<Key, Value, HashFunction>::get(const Key& key)
    {
        std::size_t array_pow = log2_power_two(m_arrayLength);

        std::size_t position;
        std::size_t failCount;
        node_ptr local{&m_head};
        mark_arraynode(local);

        hash_t fullhash = HashFunction{}(key);
        hash_t hash = fullhash;

        for (std::size_t r = 0; r < hash_size_in_bits - array_pow; r += array_pow)
        {
            failCount = 0;
            std::tie(position, hash) = compute_pos_and_hash(array_pow, hash, r);
            node_ptr node = get_node(local, position);

            while (true)
            {
                if (failCount > 8)
                { // FIXME
                    node = mark_datanode(local, position);
                }
                if (node.datanode_ptr == nullptr)
                {
                    return {};
                }

                if (is_marked(node))
                {
                    node = expandNode(local, position, r);
                }
                if (is_array_node(node))
                {
                    local = node;
                    break;
                }
                else
                {
                    node_ptr node2 = get_node(local, position);
                    if (node.ptr_int != node2.ptr_int)
                    {
                        ++failCount;
                        node = node2;
                        continue;
                    }
                    else
                    {
                        if (node.datanode_ptr->hash == fullhash)
                        {
                            // watch(nullptr);
                            return {node.datanode_ptr->value};
                        }
                        else
                        {
                            node = expandNode(local, position, r);
                            if (is_array_node(node))
                            {
                                local = node;
                                break;
                            }
                            else
                            {
                                ++failCount;
                            }
                        }
                    }
                }
            }
        }

        return {};
    }

    template <typename Key, typename Value, typename HashFunction>
    template <typename VisitorFun>
    void unordered_map<Key, Value, HashFunction>::visit(VisitorFun&& fun) noexcept(
    noexcept(std::is_nothrow_invocable_v<VisitorFun, const std::pair<const hash_t, value_t>&>))
    {
        static_assert(std::is_invocable_v<VisitorFun, const std::pair<const hash_t, value_t>&>,
                      "Visitor doesn't respect the concept");

        for (std::size_t i = 0; i < m_head_size; ++i)
        {
            node_ptr node = m_head[i].load();
            if (node.datanode_ptr != nullptr)
            {
                if (is_array_node(node))
                {
                    visit_array_node(node, fun);
                }
                else
                {
                    std::invoke(
                            fun,
                            std::pair<const hash_t&, const value_t&>(node.datanode_ptr->hash, node.datanode_ptr->value));
                }
            }
        }
    }

    template <typename Key, typename Value, typename HashFunction>
    std::size_t unordered_map<Key, Value, HashFunction>::size() const noexcept
    {
        return m_size;
    }

    template <typename Key, typename Value, typename HashFunction>
    bool unordered_map<Key, Value, HashFunction>::is_empty() const noexcept
    {
        return size() == 0;
    }

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::allocate_node(hash_t hash, value_t value) const -> node_ptr
	{
		return node_ptr{new (std::align_val_t{8}) node_t{hash, value}};
	}

    template <typename Key, typename Value, typename HashFunction>
    auto unordered_map<Key, Value, HashFunction>::expandNode(node_ptr arraynode,
                                                             std::size_t position,
                                                             std::size_t level) noexcept -> node_ptr
    {
        std::atomic<node_ptr>& node_atomic = (*sanitize_ptr(arraynode).arraynode_ptr)[position];
        node_ptr old_value = node_atomic.load();

        // watch(value);
        if (is_array_node(old_value))
        {
            return old_value;
        }
        node_ptr value = node_atomic.load();

        if (value.ptr_int != old_value.ptr_int)
        {
            return value;
        }

        node_ptr array_node{new (std::align_val_t{8}) arraynode_t{m_arrayLength}};

        std::size_t new_pos = value.datanode_ptr->hash >> (m_arrayLength + level) & (m_arrayLength - 1);
        unmark_datanode(value);

        (*array_node.arraynode_ptr)[new_pos] = value;
        mark_arraynode(array_node);

        if (!node_atomic.compare_exchange_weak(old_value, array_node))
        {
            array_node = sanitize_ptr(array_node);
            (*array_node.arraynode_ptr)[new_pos] = node_ptr{};
            delete array_node.arraynode_ptr;
        }

        return array_node;
    }

    template <typename Key, typename Value, typename HashFunction>
    template <typename VisitorFun>
    void unordered_map<Key, Value, HashFunction>::visit_array_node(node_ptr node, VisitorFun&& fun) noexcept(
    noexcept(std::is_nothrow_invocable_v<VisitorFun, const std::pair<const hash_t, value_t>&>))
    {
        for (std::size_t i = 0; i < m_arrayLength; ++i)
        {
            node_ptr child = get_node(node, i);
            if (child.datanode_ptr != nullptr)
            {
                if (is_array_node(child))
                {
                    visit_array_node(child, fun);
                }
                else
                {
                    std::invoke(
                            fun,
                            std::pair<const hash_t&, const value_t&>(child.datanode_ptr->hash, child.datanode_ptr->value));
                }
            }
        }
    }

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::compute_pos_and_hash(size_t array_pow,
	                                                                   hash_t lasthash,
	                                                                   size_t level) const
	    -> std::tuple<std::size_t, hash_t>
	{
		size_t position;

		if (level == 0)
		{
			position = lasthash & (m_head_size - 1);
			lasthash >>= m_arrayLength;
		}
		else
		{
			position = lasthash & (m_arrayLength - 1);
			lasthash >>= array_pow;
		}

		return {position, lasthash};
	}

    template <typename Key, typename Value, typename HashFunction>
    constexpr std::size_t unordered_map<Key, Value, HashFunction>::log2_power_two(std::size_t x) noexcept
    {
        return hash_size_in_bits - clz(x) - 1UL;
    }

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::is_power_of_two(std::size_t nbr) noexcept
	{
		return nbr && !(nbr & (nbr - 1));
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::mark_datanode(node_ptr arraynode, std::size_t position) noexcept
	    -> node_ptr
	{
		node_ptr oldValue = get_node(arraynode, position);
		node_ptr value = oldValue;
		mark_datanode(value);

		(*sanitize_ptr(arraynode).arraynode_ptr)[position].compare_exchange_weak(oldValue, value);

		return get_node(arraynode, position);
	}

    template <typename Key, typename Value, typename HashFunction>
    void unordered_map<Key, Value, HashFunction>::mark_datanode(node_ptr& node) noexcept
    {
        node.ptr_int |= 0b01UL;
    }

    template <typename Key, typename Value, typename HashFunction>
    void unordered_map<Key, Value, HashFunction>::unmark_datanode(node_ptr& node) noexcept
    {
        node.ptr_int &= ~0b01UL;
    }

    template <typename Key, typename Value, typename HashFunction>
    bool unordered_map<Key, Value, HashFunction>::is_marked(node_ptr node) noexcept
    {
        return static_cast<bool>(node.ptr_int & 0b1UL);
    }

    template <typename Key, typename Value, typename HashFunction>
    void unordered_map<Key, Value, HashFunction>::mark_arraynode(node_ptr& node) noexcept
    {
        node.ptr_int |= 0b10UL;
    }

    template <typename Key, typename Value, typename HashFunction>
    void unordered_map<Key, Value, HashFunction>::unmark_arraynode(node_ptr& node) noexcept
    {
        node.ptr_int &= ~0b10UL;
    }

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::is_array_node(node_ptr node) noexcept
	{
		return static_cast<bool>(node.ptr_int & 0b10UL);
	}

    template <typename Key, typename Value, typename HashFunction>
    auto unordered_map<Key, Value, HashFunction>::get_node(node_ptr arraynode, std::size_t pos) noexcept -> node_ptr
    {
        assert(is_array_node(arraynode));

        node_ptr accessor = sanitize_ptr(arraynode);

        return (*accessor.arraynode_ptr)[pos].load();
    }

    template <typename Key, typename Value, typename HashFunction>
    auto unordered_map<Key, Value, HashFunction>::sanitize_ptr(node_ptr arraynode) noexcept -> node_ptr
    {
        unmark_arraynode(arraynode);
        return arraynode;
    }

} // namespace wf

#endif