#ifndef WFC_UNORDERED_MAP_HPP
#define WFC_UNORDERED_MAP_HPP

#include <atomic>
#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>

#include "utility/math.hpp"

namespace wfc
{
	/**
	 * @brief represents the return status of the operation.
	 */
	enum class operation_result
	{
		success, /**< operation successful */
		expected_value_mismatch, /**< the key's associated value doesn't match the expected value */
		element_not_found, /**< the key is not present in the hash map */
		already_present /**< the key is already present in the hash map */
	};

	[[nodiscard]] constexpr bool succeeded(operation_result e) noexcept
	{
		return e == operation_result::success;
	}

	[[nodiscard]] constexpr bool failed(operation_result e) noexcept
	{
		return !succeeded(e);
	}

	template <typename Key>
	class identity_hash // FIXME: Size doesn't match
	{
	public:
		std::size_t operator()(const Key& k) const noexcept
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

		explicit unordered_map(std::size_t log_bucket_count);
		unordered_map(const unordered_map&) = delete;
		~unordered_map() noexcept = default;

		unordered_map& operator=(const unordered_map&) = delete;

		operation_result insert(const Key& key, const Value& value);

		std::optional<Value> get(const Key& key);

		/**
		 * Update the value associated with the given key if the current
		 * value matches expected_value.
		 *
		 * @param key
		 * @param value
		 * @param expected_value
		 * @return @see operation_result
		 */
		operation_result update(const Key& key, const Value& new_value, const Value& expected_value);

		/**
		 * Update the value associated with the given key.
		 *
		 * @param key
		 * @param value
		 * @return @see operation_result
		 */
		operation_result update(const Key& key, const Value& value);

		operation_result remove(const Key& key); // TODO

		operation_result remove(const Key& key, Value& expected_value); // TODO

		/**
		 * Applies functor on every element in the map.
		 * This function is NOT thread safe.
		 * @tparam VisitorFun The type should be compatible with this prototype void(const std::pair<const hash_t, value_t>&);
		 * @param fun
		 */
		template <typename VisitorFun>
		void visit(VisitorFun&& fun) noexcept(
		    noexcept(std::is_nothrow_invocable_v<VisitorFun, std::pair<key_t, value_t>>)); // FIXME

		/**
		 * Returns the number of elements into the collection
		 */
		std::size_t size() const noexcept;

		bool is_empty() const noexcept;

	private:
		union node_union;

		struct node_t
		{
			hash_t hash;
			key_t key;
			value_t value;
		};

		struct arraynode_t
		{
			using value_t = std::atomic<node_union>;
			using reference_t = value_t&;
			using const_reference_t = const value_t&;

			explicit arraynode_t(std::size_t size);

			arraynode_t(const arraynode_t&) noexcept = delete;
			~arraynode_t() noexcept;

			arraynode_t& operator=(const arraynode_t&) = delete;

			reference_t operator[](std::size_t i) noexcept;

			const_reference_t operator[](std::size_t i) const noexcept;

		private:
			value_t* m_ptr;
			std::size_t m_size;
		};

		union node_union
		{
			node_union() noexcept;
			explicit node_union(node_t* datanode) noexcept;
			explicit node_union(arraynode_t* arraynode) noexcept;

			node_t* datanode_ptr;
			arraynode_t* arraynode_ptr;
			std::uintptr_t ptr_int;
		};

		node_union allocate_node(hash_t hash, key_t key, value_t value) const;

		node_union expand_node(node_union arraynode, std::size_t position, std::size_t level) noexcept;

		bool try_node_insertion(node_union arraynode, std::size_t position, node_union datanode);

		template <typename Fun>
		operation_result update_impl(const Key& key, const Value& value, Fun&& compare_expected_value);

		void ensure_not_replaced(node_union& local, size_t position, size_t r, node_union& node);

		template <typename VisitorFun>
		void visit_array_node(node_union node, VisitorFun&& fun) noexcept(
		    noexcept(std::is_nothrow_invocable_v<VisitorFun, std::pair<key_t, value_t>>));

		std::tuple<std::size_t, hash_t> compute_pos_and_hash(size_t array_pow, hash_t lasthash, size_t level) const;

		static node_union mark_datanode(node_union arraynode, std::size_t position) noexcept;

		static void mark_datanode(node_union& node) noexcept;

		static void unmark_datanode(node_union& node) noexcept;

		static bool is_marked(node_union node) noexcept;

		static void mark_arraynode(node_union& node) noexcept;

		static void unmark_arraynode(node_union& node) noexcept;

		static bool is_array_node(node_union node) noexcept;

		static node_union get_node(node_union arraynode, std::size_t pos) noexcept;

		static node_union sanitize_ptr(node_union arraynode) noexcept;

		arraynode_t m_head;
		std::size_t m_head_size;
		std::size_t m_arrayLength;
		std::atomic<std::size_t> m_size;
		static constexpr std::size_t hash_size_in_bits = sizeof(hash_t) * std::numeric_limits<unsigned char>::digits;
	};

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::node_union::node_union() noexcept : datanode_ptr{nullptr}
	{
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::node_union::node_union(node_t* datanode) noexcept : datanode_ptr{datanode}
	{
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::node_union::node_union(arraynode_t* arraynode) noexcept
	    : arraynode_ptr{arraynode}
	{
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::arraynode_t::arraynode_t(std::size_t size)
	    : m_ptr{new value_t[size]}, m_size(size)
	{
		for (std::size_t i = 0; i < size; ++i)
		{
			new (&m_ptr[i]) value_t();
		}
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::arraynode_t::~arraynode_t() noexcept
	{
		for (std::size_t i = 0; i < m_size; ++i)
		{
			node_union child = m_ptr[i].load();
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
	auto unordered_map<Key, Value, HashFunction>::arraynode_t::operator[](std::size_t i) noexcept -> reference_t
	{
		return m_ptr[i];
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::arraynode_t::operator[](std::size_t i) const noexcept
	    -> const_reference_t
	{
		return m_ptr[i];
	}

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::unordered_map(std::size_t log_bucket_count)
	    : m_head(1UL << log_bucket_count)
	    , m_head_size(1UL << log_bucket_count)
	    , m_arrayLength(log_bucket_count)
	    , m_size(0UL)
	{
		static_assert(std::atomic<std::size_t>::is_always_lock_free, "Atomic implementation is not lock free");
		static_assert(std::atomic<node_union>::is_always_lock_free, "Atomic implementation is not lock free");

		if (!is_power_of_two(log_bucket_count))
		{
			throw std::runtime_error("Size should be a power of four");
		}
	}

	template <typename Key, typename Value, typename HashFunction>
	operation_result unordered_map<Key, Value, HashFunction>::insert(const Key& key, const Value& value)
	{
		std::size_t nbr_bits_to_shift = log2_of_power_of_two<hash_size_in_bits>(m_arrayLength);

		std::size_t position;
		std::size_t failCount;
		node_union local{&m_head};
		mark_arraynode(local);

		hash_t fullhash = HashFunction{}(key);
		hash_t hash = fullhash;

		for (std::size_t r = 0; r < hash_size_in_bits - nbr_bits_to_shift; r += nbr_bits_to_shift)
		{
			failCount = 0;
			std::tie(position, hash) = compute_pos_and_hash(nbr_bits_to_shift, hash, r);
			node_union node = get_node(local, position);

			while (true)
			{
				if (failCount > 8) // FIXME
				{
					node = mark_datanode(local, position);
				}

				if (node.datanode_ptr == nullptr
				    && try_node_insertion(local, position, allocate_node(fullhash, key, value)))
				{
					return operation_result::success;
				}

				if (is_marked(node))
				{
					node = expand_node(local, position, r);
				}

				if (is_array_node(node))
				{
					local = node;
					break;
				}
				else
				{
					node_union node2 = get_node(local, position);
					if (node.ptr_int != node2.ptr_int)
					{
						++failCount;
						node = node2;
					}
					else
					{
						if (node.datanode_ptr->hash == fullhash)
						{
							return operation_result::already_present;
						}
						else
						{
							node = expand_node(local, position, r);
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

		std::tie(position, std::ignore) = compute_pos_and_hash(position, hash, hash_size_in_bits - nbr_bits_to_shift);
		node_union node = get_node(local, position);

		if (node.datanode_ptr == nullptr && try_node_insertion(local, position, allocate_node(fullhash, key, value)))
		{
			return operation_result::success;
		}

		return operation_result::already_present;
	}

	template <typename Key, typename Value, typename HashFunction>
	std::optional<Value> unordered_map<Key, Value, HashFunction>::get(const Key& key)
	{
		std::size_t array_pow = log2_of_power_of_two<hash_size_in_bits>(m_arrayLength);

		std::size_t position;
		node_union local{&m_head};
		mark_arraynode(local);

		hash_t fullhash = HashFunction{}(key);
		hash_t hash = fullhash;

		for (std::size_t r = 0; r < hash_size_in_bits - array_pow; r += array_pow)
		{
			std::tie(position, hash) = compute_pos_and_hash(array_pow, hash, r);
			node_union node = get_node(local, position);

			if (is_array_node(node))
			{
				local = node;
			}
			else if (node.datanode_ptr == nullptr)
			{
				return std::nullopt;
			}
			else if (node.ptr_int != get_node(local, position).ptr_int)
			{
				ensure_not_replaced(local, position, r, node);

				if (is_array_node(node))
				{
					local = node;
				}
				else if (is_marked(node))
				{
					local = expand_node(local, position, r);
				}
				else if (node.datanode_ptr == nullptr)
				{
					return std::nullopt;
				}
			}
			else if (node.datanode_ptr->hash == fullhash)
			{
				return {node.datanode_ptr->value};
			}
			else
			{
				break;
			}
		}

		return std::nullopt;
	}

	template <typename Key, typename Value, typename HashFunction>
	operation_result unordered_map<Key, Value, HashFunction>::update(const Key& key,
	                                                                 const Value& new_value,
	                                                                 const Value& expected_value)
	{
		return update_impl(key, new_value, [&expected_value](node_t* node) { return node->value == expected_value; });
	}

	template <typename Key, typename Value, typename HashFunction>
	operation_result unordered_map<Key, Value, HashFunction>::update(const Key& key, const Value& value)
	{
		return update_impl(key, value, [](auto) { return true; });
	}

	template <typename Key, typename Value, typename HashFunction>
	template <typename VisitorFun>
	void unordered_map<Key, Value, HashFunction>::visit(VisitorFun&& fun) noexcept(
	    noexcept(std::is_nothrow_invocable_v<VisitorFun, std::pair<key_t, value_t>>))
	{
		static_assert(std::is_invocable_v<VisitorFun, std::pair<key_t, value_t>>,
		              "Visitor doesn't respect the concept");

		for (std::size_t i = 0; i < m_head_size; ++i)
		{
			node_union node = m_head[i].load();
			if (node.datanode_ptr != nullptr)
			{
				if (is_array_node(node))
				{
					visit_array_node(node, fun);
				}
				else
				{
					std::invoke(fun, std::pair<key_t, value_t>(node.datanode_ptr->key, node.datanode_ptr->value));
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
	auto unordered_map<Key, Value, HashFunction>::allocate_node(hash_t hash, key_t key, value_t value) const
	    -> node_union
	{
		return node_union{new (std::align_val_t{8}) node_t{hash, key, value}};
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::expand_node(node_union arraynode,
	                                                          std::size_t position,
	                                                          std::size_t level) noexcept -> node_union
	{
		std::atomic<node_union>& node_atomic = (*sanitize_ptr(arraynode).arraynode_ptr)[position];
		node_union old_value = node_atomic.load();

		if (is_array_node(old_value))
		{
			return old_value;
		}
		node_union value = node_atomic.load();

		if (value.ptr_int != old_value.ptr_int)
		{
			return value;
		}

		node_union array_node{new (std::align_val_t{8}) arraynode_t{m_arrayLength}};

		std::size_t new_pos = value.datanode_ptr->hash >> (m_arrayLength + level) & (m_arrayLength - 1);
		unmark_datanode(value);

		(*array_node.arraynode_ptr)[new_pos] = value;
		mark_arraynode(array_node);

		if (!node_atomic.compare_exchange_weak(old_value, array_node))
		{
			array_node = sanitize_ptr(array_node);
			(*array_node.arraynode_ptr)[new_pos] = node_union{};
			delete array_node.arraynode_ptr;
		}

		return node_atomic.load();
	}

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::try_node_insertion(node_union arraynode,
	                                                                 std::size_t position,
	                                                                 node_union datanode)
	{
		node_union null{static_cast<node_t*>(nullptr)};

		if ((*sanitize_ptr(arraynode).arraynode_ptr)[position].compare_exchange_weak(null, datanode))
		{
			++m_size;
			return true;
		}

		delete datanode.datanode_ptr;
		return false;
	}

	template <typename Key, typename Value, typename HashFunction>
	template <typename Fun>
	operation_result unordered_map<Key, Value, HashFunction>::update_impl(const Key& key,
	                                                                      const Value& value,
	                                                                      Fun&& compare_expected_value)
	{
		std::size_t array_pow = log2_of_power_of_two<hash_size_in_bits>(m_arrayLength);

		std::size_t position;
		node_union local{&m_head};
		mark_arraynode(local);

		hash_t fullhash = HashFunction{}(key);
		hash_t hash = fullhash;

		for (std::size_t r = 0; r < hash_size_in_bits - array_pow; r += array_pow)
		{
			std::tie(position, hash) = compute_pos_and_hash(array_pow, hash, r);
			node_union node = get_node(local, position);

			if (is_array_node(node))
			{
				local = node;
			}
			else if (is_marked(node))
			{
				local = expand_node(local, position, r);
			}
			else if (node.datanode_ptr == nullptr)
			{
				return operation_result::element_not_found;
			}
			else
			{
				if (node.ptr_int != get_node(local, position).ptr_int)
				{
					ensure_not_replaced(local, position, r, node);

					if (is_array_node(node))
					{
						local = node;
					}
					else if (is_marked(node))
					{
						local = expand_node(local, position, r);
					}
					else if (node.datanode_ptr == nullptr)
					{
						return operation_result::element_not_found;
					}
				}

				if (node.datanode_ptr->hash == fullhash)
				{
					if (!compare_expected_value(node.datanode_ptr))
					{
						return operation_result::expected_value_mismatch;
					}

					node_union new_node = allocate_node(fullhash, key, value);
					if ((*sanitize_ptr(local).arraynode_ptr)[position].compare_exchange_weak(node, new_node))
					{
						delete node.datanode_ptr;

						return operation_result::success;
					}
					else
					{
						delete new_node.datanode_ptr;

						node = get_node(local, position);
						if (is_array_node(node))
						{
							local = node;
						}
						else if (is_marked(node))
						{
							local = expand_node(local, position, r);
						}
						else
						{
							return operation_result::element_not_found;
						}
					}
				}
				else
				{
					return operation_result::element_not_found;
				}
			}
		}

		return operation_result::element_not_found;
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::ensure_not_replaced(node_union& local,
	                                                                  size_t position,
	                                                                  size_t r,
	                                                                  node_union& node)
	{
		std::size_t failCount = 0;
		do
		{
			node = get_node(local, position);
			++failCount;

			if (failCount > 8) // FIXME
			{
				mark_datanode(node);
				local = expand_node(local, position, r);
				break;
			}
		} while (node.ptr_int != get_node(local, position).ptr_int);
	}

	template <typename Key, typename Value, typename HashFunction>
	template <typename VisitorFun>
	void unordered_map<Key, Value, HashFunction>::visit_array_node(node_union node, VisitorFun&& fun) noexcept(
	    noexcept(std::is_nothrow_invocable_v<VisitorFun, std::pair<key_t, value_t>>))
	{
		for (std::size_t i = 0; i < m_arrayLength; ++i)
		{
			node_union child = get_node(node, i);
			if (child.datanode_ptr != nullptr)
			{
				if (is_array_node(child))
				{
					visit_array_node(child, fun);
				}
				else
				{
					std::invoke(fun, std::pair<key_t, value_t>(child.datanode_ptr->key, child.datanode_ptr->value));
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
	auto unordered_map<Key, Value, HashFunction>::mark_datanode(node_union arraynode, std::size_t position) noexcept
	    -> node_union
	{
		node_union oldValue = get_node(arraynode, position);
		node_union value = oldValue;
		mark_datanode(value);

		(*sanitize_ptr(arraynode).arraynode_ptr)[position].compare_exchange_weak(oldValue, value);

		return get_node(arraynode, position);
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::mark_datanode(node_union& node) noexcept
	{
		node.ptr_int |= 0b01UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::unmark_datanode(node_union& node) noexcept
	{
		node.ptr_int &= ~0b01UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::is_marked(node_union node) noexcept
	{
		return static_cast<bool>(node.ptr_int & 0b1UL);
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::mark_arraynode(node_union& node) noexcept
	{
		node.ptr_int |= 0b10UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	void unordered_map<Key, Value, HashFunction>::unmark_arraynode(node_union& node) noexcept
	{
		node.ptr_int &= ~0b10UL;
	}

	template <typename Key, typename Value, typename HashFunction>
	bool unordered_map<Key, Value, HashFunction>::is_array_node(node_union node) noexcept
	{
		return static_cast<bool>(node.ptr_int & 0b10UL);
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::get_node(node_union arraynode, std::size_t pos) noexcept -> node_union
	{
		assert(is_array_node(arraynode));

		node_union accessor = sanitize_ptr(arraynode);

		return (*accessor.arraynode_ptr)[pos].load();
	}

	template <typename Key, typename Value, typename HashFunction>
	auto unordered_map<Key, Value, HashFunction>::sanitize_ptr(node_union arraynode) noexcept -> node_union
	{
		unmark_arraynode(arraynode);
		return arraynode;
	}

} // namespace wfc

#endif // WFC_UNORDERED_MAP_HPP