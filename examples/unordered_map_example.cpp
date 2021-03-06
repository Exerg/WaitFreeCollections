#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <sstream>

#include <wfc/unordered_map.hpp>

constexpr int nbr_threads = 64;

int main()
{
	std::promise<void> ready_promise;
	std::shared_future<void> f = {ready_promise.get_future()};

	wfc::unordered_map<std::size_t, std::size_t> m(8, nbr_threads, nbr_threads);

	std::array<std::thread, nbr_threads> threads;
	std::array<double, nbr_threads> insertion_times = {};
	for (std::size_t i = 0; i < nbr_threads; ++i)
	{
		threads[i] = std::thread([&m, i, &f, &insertion_times]() {
			f.wait();

			clock_t t_start = std::clock();
			wfc::operation_result result = m.insert(i, i);
			clock_t t_end = std::clock();
			insertion_times[i] = 1000.0 * static_cast<double>(t_end - t_start) / CLOCKS_PER_SEC;
			if (failed(result))
			{
				std::stringstream output;
				output << "Not inserted: " << i << '\n';
				std::cout << output.str();
			}
		});
	}

	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1s);
	}

	ready_promise.set_value();

	for (auto& t: threads)
	{
		t.join();
	}

	m.visit(
	    [](std::pair<const std::size_t, std::size_t> p) { std::cout << '[' << p.first << '-' << p.second << "]\n"; });

	double max = *std::max_element(insertion_times.begin(), insertion_times.end());
	std::cout << "Max:  " << max << "ms\n";
	std::cout << "Mean: "
	          << std::accumulate(insertion_times.begin(), insertion_times.end(), 0.0) / insertion_times.size()
	          << "ms\n";
	std::cout << "Min:  " << *std::min_element(insertion_times.begin(), insertion_times.end()) << "ms\n";

	return 0;
}
