#ifndef WAITFREEHASHMAP_THREAD_MANIPULATION_HPP
#define WAITFREEHASHMAP_THREAD_MANIPULATION_HPP

#include <atomic>

namespace wfc
{
	namespace details
	{
		inline std::size_t get_thread_id() {
			static std::atomic<std::size_t> i = 0;
			thread_local static std::size_t id = i++;

			return id;
		}
	}
} // namespace wfc
#endif //WAITFREEHASHMAP_THREAD_MANIPULATION_HPP
