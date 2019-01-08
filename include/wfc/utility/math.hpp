#ifndef WFC_MATH_HPP
#define WFC_MATH_HPP

#include <limits>
#include <type_traits>

#ifndef __has_builtin
	#define WFC__has_builtin(x) 0
#else
#define WFC__has_builtin(x) __has_builtin(x)
#endif

namespace wfc
{
	namespace details
	{
#if WFC__has_builtin(__builtin_clz) && WFC__has_builtin(__builtin_clzl) && WFC__has_builtin(__builtin_clzll)
		template <typename T>
		T clz(T x)
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
#else
		template <typename T>
		T clz(T x)
		{
			static_assert(std::is_unsigned_v<T>, "T should be unsigned");

			T mask = T(1) << T(std::numeric_limits<T>::digits - 1);

			T result = 0;

			for (std::size_t i = 0; i < std::numeric_limits<T>::digits; ++i) {
				if ((mask & x) != 0) {
					return result;
				}
				++result;
				mask >>= 1;
			}

			return result;
		}
#endif
	} // namespace details

	template <typename T>
	constexpr std::size_t log2_of_power_of_two(T x) noexcept;

	template <typename T>
	constexpr bool is_power_of_two(T nbr) noexcept;

	template <typename T>
	constexpr std::size_t log2_of_power_of_two(T x) noexcept
	{
		static_assert(std::is_integral_v<T>, "T should be an integral type");
		assert(is_power_of_two(x));

		return sizeof(T) * std::numeric_limits<unsigned char>::digits - details::clz(x) - 1UL;
	}

	template <typename T>
	constexpr bool is_power_of_two(T nbr) noexcept
	{
		return nbr && !(nbr & (nbr - 1));
	}
} // namespace wfc
#endif // WFC_MATH_HPP
