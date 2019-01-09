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
#include "details/unordered_map/nodes.hpp"

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

	/**
	 * Helper function that returns true if the parameter correspond to a success.
	 */
	[[nodiscard]] constexpr bool succeeded(operation_result e) noexcept
	{
		return e == operation_result::success;
	}

	/**
	 * Helper function that returns true if the parameter correspond to a fail state.
	 */
	[[nodiscard]] constexpr bool failed(operation_result e) noexcept
	{
		return !succeeded(e);
	}

	/**
	 * Default hash function. This function is the identity.
	 * @tparam Key - Type of the key
	 */
	template <typename Key>
	class identity_hash
	{
	public:
		static_assert(std::is_fundamental_v<Key>, "Key should be a fundamental type to use the default hash");
		Key operator()(const Key& k) const noexcept
		{
			return k;
		}
	};

	/**
	 * A wait free hash map
	 *
	 * @details This map could be seen as a n-ary tree, (except that the head has 2^n children)
	 * In the case of this map, n is array_length.
	 * Each node of this map could be an array or a datanode. If two datanodes should go in the same place.
	 * Then the existing datanode is transformed into an arraynode to allow the insertion of the two datanodes.
	 * It may be noted that this process of extending the map will be repeted until the hash of the nodes differ.
	 *
	 * @tparam Key type of the key in the map
	 * @tparam Value type of the value in the map
	 * @tparam HashFunction Functor that should give a hash of the key.
	 * This function should hash keys without having any collision.
	 * As a consequence, it is recommended that the input size should be the same as the output size.
	 */
	template <typename Key, typename Value, typename HashFunction = identity_hash<Key>>
	class unordered_map
	{
	public:
		using key_t = Key;
		using hash_t = std::invoke_result_t<HashFunction, Key>;
		using value_t = Value;

		/**
		 * Constructs a wait free hash map.
		 *
		 * @param array_length size of the array containing the elements (head size is 2^array_length)
		 * @param max_fail_count this value should match to the number of threads using this map
		 */
		explicit unordered_map(std::size_t array_length, std::size_t max_fail_count = 8);
		unordered_map(const unordered_map&) = delete;
		~unordered_map() noexcept = default;

		unordered_map& operator=(const unordered_map&) = delete;

		/**
		 * Inserts the key and the value in the hash map.
		 *
		 * @return Returns operation_result::already_present if the key is already in the map.
		 * Otherwise it returns operation_result::success
		 */
		operation_result insert(const Key& key, const Value& value);

		/**
		 * Tries to retrieve the value associated to the key.
		 *
		 * @return an empty optional if the key in not in the map. The associated value otherwise.
		 */
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

		/**
		 * Remove the element associated to the given key if the
		 * expected_value matches the current value.
		 *
		 * @param key
		 * @return @see operation_result
		 */
		operation_result remove(const Key& key, const Value& expected_value);

		/**
		 * Remove the element associated to the given key.
		 *
		 * @param key
		 * @return @see operation_result
		 */
		operation_result remove(const Key& key);

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

		/**
		 * Returns true if the map is empty, false otherwise.
		 */
		bool is_empty() const noexcept;

	private:
		using node_t = details::node_t<hash_t, key_t, value_t>;
		using node_union = details::node_union<node_t>;
		using arraynode_t = details::arraynode_t<node_t>;

		node_union allocate_node(hash_t hash, key_t key, value_t value) const;

		node_union expand_node(node_union arraynode, std::size_t position, std::size_t level) noexcept;

		bool try_node_insertion(node_union arraynode, std::size_t position, node_union datanode);

		template <typename Fun>
		operation_result update_impl(const Key& key, const Value& value, Fun&& compare_expected_value);

		template <typename Fun>
		operation_result remove_impl(const Key& key, Fun&& compare_expected_value);

		template <typename CmpFun, typename AllocFun>
		operation_result update_or_remove_impl(const Key& key,
		                                       CmpFun&& compare_expected_value,
		                                       AllocFun&& replacing_node);

		void ensure_not_replaced(node_union& local, size_t position, size_t r, node_union& node);

		template <typename VisitorFun>
		void visit_array_node(node_union node, VisitorFun&& fun) noexcept(
		    noexcept(std::is_nothrow_invocable_v<VisitorFun, std::pair<key_t, value_t>>));

		std::tuple<std::size_t, hash_t> compute_pos_and_hash(size_t array_pow, hash_t lasthash, size_t level) const;

		arraynode_t m_head;
		std::size_t m_head_size;
		std::size_t m_arrayLength;
		std::size_t m_max_fail_count;
		std::atomic<std::size_t> m_size;
		static constexpr std::size_t hash_size_in_bits = sizeof(hash_t) * std::numeric_limits<unsigned char>::digits;
	};

	template <typename Key, typename Value, typename HashFunction>
	unordered_map<Key, Value, HashFunction>::unordered_map(std::size_t array_length, std::size_t max_fail_count)
	    : m_head(1UL << array_length)
	    , m_head_size(1UL << array_length)
	    , m_arrayLength(array_length)
	    , m_max_fail_count(max_fail_count)
	    , m_size(0UL)
	{
		static_assert(std::atomic<std::size_t>::is_always_lock_free, "Atomic implementation is not lock free");
		static_assert(std::atomic<node_union>::is_always_lock_free, "Atomic implementation is not lock free");

		if (!is_power_of_two(array_length))
		{
			throw std::runtime_error("Array length should be a power of two");
		}
	}

	template <typename Key, typename Value, typename HashFunction>
	operation_result unordered_map<Key, Value, HashFunction>::insert(const Key& key, const Value& value)
	{
		std::size_t nbr_bits_to_shift = log2_of_power_of_two(m_arrayLength);

		std::size_t position;
		std::size_t fail_count;
		node_union local{&m_head};
		mark_arraynode(local);

		hash_t fullhash = HashFunction{}(key);
		hash_t hash = fullhash;

		for (std::size_t r = 0; r < hash_size_in_bits - nbr_bits_to_shift; r += nbr_bits_to_shift)
		{
			fail_count = 0;
			std::tie(position, hash) = compute_pos_and_hash(nbr_bits_to_shift, hash, r);
			node_union node = get_node(local, position);

			while (true)
			{
				if (fail_count > m_max_fail_count)
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
						++fail_count;
						node = node2;
						continue;
					}
					else if (node.datanode_ptr->hash == fullhash)
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
							++fail_count;
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
		std::size_t array_pow = log2_of_power_of_two(m_arrayLength);

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
	operation_result unordered_map<Key, Value, HashFunction>::remove(const Key& key, const Value& expected_value)
	{
		return remove_impl(key, [&expected_value](node_t* node) { return node->value == expected_value; });
	}

	template <typename Key, typename Value, typename HashFunction>
	operation_result unordered_map<Key, Value, HashFunction>::remove(const Key& key)
	{
		return remove_impl(key, [](auto) { return true; });
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
		if (alignof(node_t*) < 4)
		{
			return node_union{new (std::align_val_t{8}) node_t{hash, key, value}};
		}
		else
		{
			return node_union{new node_t{hash, key, value}};
		}
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

		if (value.datanode_ptr != nullptr)
		{
			node_union array_node;

			if (alignof(arraynode_t*) < 4)
			{
				array_node = node_union{new (std::align_val_t{8}) arraynode_t{m_arrayLength}};
			}
			else
			{
				array_node = node_union{new arraynode_t{m_arrayLength}};
			}

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
		return update_or_remove_impl(key, compare_expected_value, [&value, &key, this](hash_t fullhash) {
			return this->allocate_node(fullhash, key, value);
		});
	}

	template <typename Key, typename Value, typename HashFunction>
	template <typename Fun>
	operation_result unordered_map<Key, Value, HashFunction>::remove_impl(const Key& key, Fun&& compare_expected_value)
	{
		return update_or_remove_impl(key, compare_expected_value, [](hash_t) { return node_union{}; });
	}

	template <typename Key, typename Value, typename HashFunction>
	template <typename CmpFun, typename AllocFun>
	operation_result unordered_map<Key, Value, HashFunction>::update_or_remove_impl(const Key& key,
	                                                                                CmpFun&& compare_expected_value,
	                                                                                AllocFun&& replacing_node)
	{
		std::size_t array_pow = log2_of_power_of_two(m_arrayLength);

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
						continue;
					}
					else if (is_marked(node))
					{
						local = expand_node(local, position, r);
						continue;
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

					node_union new_node = replacing_node(fullhash);
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
		std::size_t fail_count = 0;
		do
		{
			node = get_node(local, position);
			++fail_count;

			if (fail_count > m_max_fail_count)
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
} // namespace wfc

#endif // WFC_UNORDERED_MAP_HPP