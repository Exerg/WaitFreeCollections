#ifndef WFC_MATH_HPP
#define WFC_MATH_HPP

namespace wfc
{
	namespace details
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
	} // namespace details

	template <std::size_t HashSizeInBit, typename T>
	constexpr std::size_t log2_of_power_of_two(T x) noexcept;

	template <typename T>
	constexpr bool is_power_of_two(T nbr) noexcept;

	template <std::size_t HashSizeInBit, typename T>
	constexpr std::size_t log2_of_power_of_two(T x) noexcept
	{
		return HashSizeInBit - details::clz(x) - 1UL;
	}

	template <typename T>
	constexpr bool is_power_of_two(T nbr) noexcept
	{
		return nbr && !(nbr & (nbr - 1));
	}
} // namespace wfc
#endif // WFC_MATH_HPP
