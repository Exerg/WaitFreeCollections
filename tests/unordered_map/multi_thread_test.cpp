#include <gtest/gtest.h>

#include <array>
#include <future>
#include <mutex>
#include <thread>

#include <wfc/unordered_map.hpp>

class WaitFreeHashMapMultiThreadTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		futur = std::shared_future<void>(ready_promise.get_future());

		for (std::size_t i = 0; i < map_size; ++i)
		{
			map.insert(static_cast<unsigned char>(i), i + 1);
		}
	}

	void wait_threads()
	{
		for (auto& t: threads)
		{
			t.join();
		}
	}

	static constexpr std::size_t block_low(std::size_t threadIdx, std::size_t nbrThreads, std::size_t dataSize) noexcept
	{
		return threadIdx * dataSize / nbrThreads;
	}

	static constexpr std::size_t block_high(std::size_t threadIdx,
	                                        std::size_t nbrThreads,
	                                        std::size_t dataSize) noexcept
	{
		return block_low(threadIdx + 1, nbrThreads, dataSize) - 1;
	}

	static constexpr std::size_t block_size(std::size_t threadIdx,
	                                        std::size_t nbrThreads,
	                                        std::size_t dataSize) noexcept
	{
		return block_high(threadIdx, nbrThreads, dataSize) - block_low(threadIdx, nbrThreads, dataSize) + 1;
	}

	static constexpr std::size_t map_size = std::numeric_limits<unsigned char>::max() + 1;
	std::vector<std::thread> threads;
	std::shared_future<void> futur;
	std::promise<void> ready_promise;
	wfc::unordered_map<unsigned char, std::size_t> map{4};
};

TEST_F(WaitFreeHashMapMultiThreadTest, UpdateNoConflict)
{
	constexpr std::size_t nbr_threads = 8;

	for (std::size_t i = 0; i < nbr_threads; ++i)
	{
		threads.emplace_back([this, i]() {
			futur.wait();

			for (std::size_t j = block_low(i, nbr_threads, map_size), n = block_high(i, nbr_threads, map_size); j <= n;
			     ++j)
			{
				ASSERT_EQ(map.update(static_cast<unsigned char>(j), 2 * (j + 1), j + 1),
				          wfc::operation_result::success);
			}
		});
	}

	ready_promise.set_value();
	wait_threads();

	for (std::size_t i = 0; i < map_size; ++i)
	{
		ASSERT_EQ(map.get(static_cast<unsigned char>(i)).value(), 2 * (i + 1));
	}
}

TEST_F(WaitFreeHashMapMultiThreadTest, UpdateConflict)
{
	constexpr std::size_t nbr_threads = 16;
	std::array<std::atomic<int>, nbr_threads / 2> fails{};

	for (std::size_t i = 0; i < nbr_threads; ++i)
	{
		threads.emplace_back([this, i, &fails]() {
			futur.wait();
			std::size_t idx = i % 8;

			for (std::size_t j = block_low(idx, nbr_threads / 2, map_size),
			                 n = block_high(idx, nbr_threads / 2, map_size);
			     j <= n;
			     ++j)
			{
				if (failed(map.update(static_cast<unsigned char>(j), 2 * (j + 1), j + 1)))
				{
					++fails[idx];
				}
			}
		});
	}

	ready_promise.set_value();
	wait_threads();

	for (std::size_t i = 0; i < fails.size(); ++i)
	{
		ASSERT_EQ(fails[i].load(), block_size(i, nbr_threads / 2, map_size));
	}

	for (std::size_t i = 0; i < map_size; ++i)
	{
		ASSERT_EQ(map.get(static_cast<unsigned char>(i)).value(), 2 * (i + 1));
	}
}

TEST_F(WaitFreeHashMapMultiThreadTest, RemoveNoConflict)
{
	constexpr std::size_t nbr_threads = 8;

	for (std::size_t i = 0; i < nbr_threads; ++i)
	{
		threads.emplace_back([this, i]() {
			futur.wait();

			for (std::size_t j = block_low(i, nbr_threads, map_size), n = block_high(i, nbr_threads, map_size); j <= n;
			     ++j)
			{
				ASSERT_EQ(map.remove(static_cast<unsigned char>(j), j + 1), wfc::operation_result::success);
			}
		});
	}

	ready_promise.set_value();
	wait_threads();

	for (std::size_t i = 0; i < map_size; ++i)
	{
		ASSERT_FALSE(map.get(static_cast<unsigned char>(i)).has_value());
	}
}

TEST_F(WaitFreeHashMapMultiThreadTest, RemoveConflict)
{
	constexpr std::size_t nbr_threads = 16;
	std::array<std::atomic<int>, nbr_threads / 2> fails{};

	for (std::size_t i = 0; i < nbr_threads; ++i)
	{
		threads.emplace_back([this, i, &fails]() {
			futur.wait();
			std::size_t idx = i % 8;

			for (std::size_t j = block_low(idx, nbr_threads / 2, map_size),
			                 n = block_high(idx, nbr_threads / 2, map_size);
			     j <= n;
			     ++j)
			{
				if (failed(map.remove(static_cast<unsigned char>(j), j + 1)))
				{
					++fails[idx];
				}
			}
		});
	}

	ready_promise.set_value();
	wait_threads();

	for (std::size_t i = 0; i < fails.size(); ++i)
	{
		ASSERT_EQ(fails[i].load(), block_size(i, nbr_threads / 2, map_size));
	}

	for (std::size_t i = 0; i < map_size; ++i)
	{
		ASSERT_FALSE(map.get(static_cast<unsigned char>(i)).has_value());
	}
}

TEST_F(WaitFreeHashMapMultiThreadTest, MixedOperation)
{
	wfc::unordered_map<unsigned char, std::size_t> custom_map{4};
	constexpr std::size_t nbr_threads = 3;
	std::array<std::atomic_bool, map_size - 1> updated{};
	std::array<std::atomic_bool, map_size - 1> removed{};

	for (std::size_t i = 0; i < nbr_threads; ++i)
	{
		threads.emplace_back([this, &custom_map, i]() {
			futur.wait();
			std::size_t idx = i;

			for (std::size_t j = block_low(idx, nbr_threads, map_size - 1),
			                 n = block_high(idx, nbr_threads, map_size - 1);
			     j <= n;
			     ++j)
			{
				ASSERT_EQ(custom_map.insert(static_cast<unsigned char>(j), 2 * j), wfc::operation_result::success);
			}

			//printf("j: %zu, n: %zu\n", block_low(i, nbr_threads, map_size), block_high(i, nbr_threads, map_size));
		});

		threads.emplace_back([this, &custom_map, i, &updated]() {
			futur.wait();
			std::size_t idx = i;

			for (std::size_t j = block_low(idx, nbr_threads, map_size - 1),
			                 n = block_high(idx, nbr_threads, map_size - 1);
			     j <= n;
			     ++j)
			{
				if (succeeded(custom_map.update(static_cast<unsigned char>(j), 4 * j)))
				{
					updated[j] = true;
				}
			}
		});

		threads.emplace_back([this, &custom_map, i, &removed]() {
			futur.wait();
			std::size_t idx = i;

			for (std::size_t j = block_low(idx, nbr_threads, map_size - 1),
			                 n = block_high(idx, nbr_threads, map_size - 1);
			     j <= n;
			     ++j)
			{
				if (succeeded(custom_map.remove(static_cast<unsigned char>(j))))
				{
					removed[j] = true;
				}
			}
		});
	}

	ready_promise.set_value();
	wait_threads();

	for (std::size_t i = 0; i < map_size - 1; ++i) {
		if (updated[i] && !removed[i]) {
			ASSERT_EQ(custom_map.get(static_cast<unsigned char>(i)).value(), 4 * i);
		} else if (removed[i]) {
			ASSERT_FALSE(custom_map.get(static_cast<unsigned char>(i)).has_value());
		} else {
			ASSERT_EQ(custom_map.get(static_cast<unsigned char>(i)).value(), 2 * i);
		}
	}
}
