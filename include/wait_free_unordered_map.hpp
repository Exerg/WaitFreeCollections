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
	T clz(T x)
	{
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
		explicit unordered_map(std::size_t initialSize);

		unordered_map(const unordered_map&) = delete;

		unordered_map& operator=(const unordered_map&) = delete;

		bool insert(const Key& key, const Value& value);

		std::optional<Value> get(const Key& key);

		template <typename VisitorFun>
		void visit(VisitorFun&& fun);

		std::size_t size() const;

		bool is_empty() const;

	private:
		struct node__;
		struct arraynode__;
		union node_ptr;

		using hash_t = std::invoke_result_t<HashFunction, Key>;
		using value_t = Value;
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

			explicit arraynode__(std::size_t size) : m_ptr{new value_t[size]}, m_size(size)
			{
				for (std::size_t i = 0; i < size; ++i)
				{
					new (&m_ptr[i]) value_t();
				}
			}

			arraynode__(const arraynode__&) = delete;

			arraynode__& operator=(const arraynode__&) = delete;

			~arraynode__()
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

			reference_t operator[](std::size_t i)
			{
				return m_ptr[i];
			}

		private:
			value_t* m_ptr;

			std::size_t m_size;
		};

		union node_ptr
		{
			node_ptr() noexcept : datanode_ptr{nullptr}
			{
			}

			node_t* datanode_ptr;

			arraynode_t* arraynode_ptr;

			std::uintptr_t ptr_int;
		};

		static bool is_power_of_two(std::size_t nbr) noexcept;

		constexpr static std::size_t log2_power_two(std::size_t x) noexcept;

		static node_ptr mark_datanode(node_ptr arraynode, std::size_t position);

		static bool is_marked(node_ptr node);

		static bool is_array_node(node_ptr node);

		static void mark_datanode(node_ptr& node);

		static void unmark_datanode(node_ptr& node);

		static void mark_arraynode(node_ptr& node);

		static void unmark_arraynode(node_ptr& node);

		node_ptr expandNode(node_ptr arraynode, std::size_t position, std::size_t level);

		template <typename VisitorFun>
		void visit_array_node(node_ptr node, std::size_t level, VisitorFun&& fun);

		node_ptr allocate_node(hash_t hash, value_t value) const;

		static node_ptr get_node(node_ptr arraynode, std::size_t pos);

		static node_ptr sanitize_ptr(node_ptr arraynode);

		arraynode_t m_head;

		std::size_t m_head_size;

		std::size_t m_arrayLength;

		std::size_t m_size;

		static constexpr std::size_t hash_size_in_bits = sizeof(hash_t) * std::numeric_limits<unsigned char>::digits;
	};

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::unordered_map(std::size_t initialSize)
	  : m_head(2UL << initialSize), m_head_size(2UL << initialSize), m_arrayLength(initialSize), m_size(0UL)
	{
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
		node_ptr local;
		local.arraynode_ptr = &m_head;
		mark_arraynode(local);

		hash_t fullhash = HashFunction{}(key);
		hash_t hash = fullhash;

		for (std::size_t r = 0; r < hash_size_in_bits - array_pow; r += array_pow)
		{
			if (r == 0)
			{
				position = hash & (m_head_size - 1);
				hash >>= m_arrayLength;
			}
			else
			{
				position = hash & (m_arrayLength - 1);
				hash >>= array_pow;
			}

			failCount = 0;

			node_ptr node = get_node(local, position);

			while (true)
			{
				if (failCount > 8)
				{ // FIXME
					node = mark_datanode(local, position);
				}
				if (node.datanode_ptr == nullptr)
				{
					node_ptr node_to_insert = allocate_node(fullhash, value);

					node_ptr null;
					null.datanode_ptr = nullptr;

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

			node_ptr null;
			null.datanode_ptr = nullptr;

			return (*sanitize_ptr(local).arraynode_ptr)[position].compare_exchange_weak(null, node_to_insert);
		}
		else
		{
			return false;
		}
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::allocate_node(hash_t hash, value_t value) const
	  -> unordered_map::node_ptr
	{
		node_ptr node_to_insert;

		node_to_insert.datanode_ptr = new (std::align_val_t{8}) node_t{};
		node_to_insert.datanode_ptr->hash = hash;
		node_to_insert.datanode_ptr->value = value;

		return node_to_insert;
	}

	template <typename Key, typename Value, typename HashFunction>
	std::optional<Value> unordered_map<Key, Value, HashFunction>::get(const Key& key)
	{
		std::size_t array_pow = log2_power_two(m_arrayLength);

		std::size_t position;
		std::size_t failCount;
		node_ptr local;
		local.arraynode_ptr = &m_head;
		mark_arraynode(local);

		hash_t fullhash = HashFunction{}(key);
		hash_t hash = fullhash;

		for (std::size_t r = 0; r < hash_size_in_bits - array_pow; r += array_pow)
		{
			if (r == 0)
			{
				position = hash & (m_head_size - 1);
				hash >>= m_arrayLength;
			}
			else
			{
				position = hash & (m_arrayLength - 1);
				hash >>= array_pow;
			}

			failCount = 0;

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
	bool unordered_map<Key, Value, HashFunction>::is_power_of_two(std::size_t nbr) noexcept
	{
		return nbr && !(nbr & (nbr - 1));
	}

	template <typename Key, typename Value, typename HashFunction>
	constexpr std::size_t unordered_map<Key, Value, HashFunction>::log2_power_two(std::size_t x) noexcept
	{
		return hash_size_in_bits - clz(x) - 1UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::mark_datanode(node_ptr arraynode, std::size_t position)
	  -> unordered_map::node_ptr
	{
		node_ptr oldValue = get_node(arraynode, position);
		node_ptr value = oldValue;
		mark_datanode(value);

		(*sanitize_ptr(arraynode).arraynode_ptr)[position].compare_exchange_weak(oldValue, value);

		return get_node(arraynode, position);
	}

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::is_marked(node_ptr node)
	{
		return static_cast<bool>(node.ptr_int & 0b1UL);
	}

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::is_array_node(node_ptr node)
	{
		return static_cast<bool>(node.ptr_int & 0b10UL);
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::expandNode(node_ptr arraynode,
	                                                         std::size_t position,
	                                                         std::size_t level) -> unordered_map::node_ptr
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

		node_ptr new_value;
		new_value.arraynode_ptr = new (std::align_val_t{8}) arraynode_t{m_arrayLength};

		std::size_t new_pos = value.datanode_ptr->hash >> (m_arrayLength + level) & (m_arrayLength - 1);
		unmark_datanode(value);

		(*new_value.arraynode_ptr)[new_pos] = value;
		mark_arraynode(new_value);

		if (!node_atomic.compare_exchange_weak(old_value, new_value))
		{
			new_value = sanitize_ptr(new_value);
			(*new_value.arraynode_ptr)[new_pos] = node_ptr{};
			delete new_value.arraynode_ptr;
		}

		return new_value;
	}

	template <typename Key, typename Value, typename HashFunction>
	template <typename VisitorFun>
	void unordered_map<Key, Value, HashFunction>::visit(VisitorFun&& fun)
	{
		for (std::size_t i = 0; i < m_head_size; ++i)
		{
			node_ptr node = m_head[i].load();
			if (node.datanode_ptr != nullptr)
			{
				if (is_array_node(node))
				{
					visit_array_node(node, 1, fun);
				}
				else
				{
					std::invoke(
					  fun, std::pair<const hash_t&, const value_t&>(node.datanode_ptr->hash, node.datanode_ptr->value));
				}
			}
		}
	}

	template <typename Key, typename Value, typename HashFunction>
	template <typename VisitorFun>
	void unordered_map<Key, Value, HashFunction>::visit_array_node(node_ptr node, std::size_t level, VisitorFun&& fun)
	{
		for (std::size_t i = 0; i < m_arrayLength; ++i)
		{
			node_ptr child = get_node(node, i);
			if (child.datanode_ptr != nullptr)
			{
				if (is_array_node(child))
				{
					visit_array_node(child, level + 1);
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
	auto unordered_map<Key, Value, HashFunction>::get_node(node_ptr arraynode, std::size_t pos)
	  -> unordered_map::node_ptr
	{
		assert(is_array_node(arraynode));

		node_ptr accessor = sanitize_ptr(arraynode);

		return (*accessor.arraynode_ptr)[pos].load();
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::sanitize_ptr(node_ptr arraynode) -> node_ptr
	{
		unmark_arraynode(arraynode);
		return arraynode;
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::mark_datanode(node_ptr& node)
	{
		node.ptr_int |= 0b01UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::unmark_datanode(node_ptr& node)
	{
		node.ptr_int &= ~0b01UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::mark_arraynode(node_ptr& node)
	{
		node.ptr_int |= 0b10UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::unmark_arraynode(node_ptr& node)
	{
		node.ptr_int &= ~0b10UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	std::size_t unordered_map<Key, Value, HashFunction>::size() const
	{
		return m_size;
	}

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::is_empty() const
	{
		return size() == 0;
	}

} // namespace wf

#undef clz

#endif