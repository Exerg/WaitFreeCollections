#ifndef WFC_NODES_HPP
#define WFC_NODES_HPP

#include <atomic>
#include <cstdint>

namespace wfc
{
	namespace details
	{
		template <typename NodeT>
		union node_union;

		template <typename Hash, typename Key, typename Value>
		struct node_t
		{
			Hash hash;
			Key key;
			Value value;
		};

		template <typename NodeT>
		struct arraynode_t
		{
			using value_t = std::atomic<node_union<NodeT>>;
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

		template <typename NodeT>
		union node_union
		{
			node_union() noexcept;
			explicit node_union(NodeT* datanode) noexcept;
			explicit node_union(arraynode_t<NodeT>* arraynode) noexcept;

			NodeT* datanode_ptr;
			arraynode_t<NodeT>* arraynode_ptr;
			std::uintptr_t ptr_int;
		};

		template <typename NodeT>
		bool is_array_node(node_union<NodeT> node) noexcept;

		template <typename NodeT>
		node_union<NodeT> mark_datanode(node_union<NodeT> arraynode, std::size_t position) noexcept;

		template <typename NodeT>
		void mark_datanode(node_union<NodeT>& node) noexcept;

		template <typename NodeT>
		void unmark_datanode(node_union<NodeT>& node) noexcept;

		template <typename NodeT>
		bool is_marked(node_union<NodeT> node) noexcept;

		template <typename NodeT>
		void mark_arraynode(node_union<NodeT>& node) noexcept;

		template <typename NodeT>
		void unmark_arraynode(node_union<NodeT>& node) noexcept;

		template <typename NodeT>
		node_union<NodeT> get_node(node_union<NodeT> arraynode, std::size_t pos) noexcept;

		template <typename NodeT>
		node_union<NodeT> sanitize_ptr(node_union<NodeT> arraynode) noexcept;

		template <typename NodeT>
		bool is_array_node(node_union<NodeT> node) noexcept
		{
			return static_cast<bool>(node.ptr_int & 0b10UL);
		}

		template <typename NodeT>
		auto mark_datanode(node_union<NodeT> arraynode, std::size_t position) noexcept -> node_union<NodeT>
		{
			node_union oldValue = get_node(arraynode, position);
			node_union value = oldValue;
			mark_datanode(value);

			(*sanitize_ptr(arraynode).arraynode_ptr)[position].compare_exchange_weak(oldValue, value);

			return get_node(arraynode, position);
		}

		template <typename NodeT>
		void mark_datanode(node_union<NodeT>& node) noexcept
		{
			node.ptr_int |= 0b01UL;
		}

		template <typename NodeT>
		void unmark_datanode(node_union<NodeT>& node) noexcept
		{
			node.ptr_int &= ~0b01UL;
		}

		template <typename NodeT>
		bool is_marked(node_union<NodeT> node) noexcept
		{
			return static_cast<bool>(node.ptr_int & 0b1UL);
		}

		template <typename NodeT>
		void mark_arraynode(node_union<NodeT>& node) noexcept
		{
			node.ptr_int |= 0b10UL;
		}

		template <typename NodeT>
		void unmark_arraynode(node_union<NodeT>& node) noexcept
		{
			node.ptr_int &= ~0b10UL;
		}

		template <typename NodeT>
		auto get_node(node_union<NodeT> arraynode, std::size_t pos) noexcept -> node_union<NodeT>
		{
			assert(is_array_node(arraynode));

			node_union accessor = sanitize_ptr(arraynode);

			return (*accessor.arraynode_ptr)[pos].load();
		}

		template <typename NodeT>
		auto sanitize_ptr(node_union<NodeT> arraynode) noexcept -> node_union<NodeT>
		{
			unmark_arraynode(arraynode);
			return arraynode;
		}

		template <typename NodeT>
		node_union<NodeT>::node_union() noexcept : datanode_ptr{nullptr}
		{
		}

		template <typename NodeT>
		node_union<NodeT>::node_union(NodeT* datanode) noexcept : datanode_ptr{datanode}
		{
		}

		template <typename NodeT>
		node_union<NodeT>::node_union(arraynode_t<NodeT>* arraynode) noexcept : arraynode_ptr{arraynode}
		{
		}

		template <typename NodeT>
		arraynode_t<NodeT>::arraynode_t(std::size_t size) : m_ptr{new value_t[size]}, m_size(size)
		{
			for (std::size_t i = 0; i < size; ++i)
			{
				new (&m_ptr[i]) value_t();
			}
		}

		template <typename NodeT>
		arraynode_t<NodeT>::~arraynode_t() noexcept
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
				m_ptr[i].store({});
			}

			delete[] m_ptr;
		}

		template <typename NodeT>
		auto arraynode_t<NodeT>::operator[](std::size_t i) noexcept -> reference_t
		{
			return m_ptr[i];
		}

		template <typename NodeT>
		auto arraynode_t<NodeT>::operator[](std::size_t i) const noexcept -> const_reference_t
		{
			return m_ptr[i];
		}
	} // namespace details
} // namespace wfc
#endif // WFC_NODES_HPP
