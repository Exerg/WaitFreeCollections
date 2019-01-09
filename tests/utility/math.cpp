#include <gtest/gtest.h>

#include <wfc/utility/math.hpp>

TEST(Utilty, IsPowerOfTwo)
{
	ASSERT_TRUE(wfc::is_power_of_two(2));
	ASSERT_FALSE(wfc::is_power_of_two(3));
}

TEST(Utilty, Log2OfPowerOfTwo)
{
	ASSERT_EQ(wfc::log2_of_power_of_two(2U), 1U);
	ASSERT_EQ(wfc::log2_of_power_of_two(4U), 2U);
	ASSERT_EQ(wfc::log2_of_power_of_two(8U), 3U);
}

#ifndef NDEBUG
#	define Log2OfPowerOfTwoDeath Log2OfPowerOfTwoDeath
#else
#	define Log2OfPowerOfTwoDeath DISABLED_Log2OfPowerOfTwoDeath
#endif

TEST(Utility, Log2OfPowerOfTwoDeath)
{
	EXPECT_DEATH(wfc::log2_of_power_of_two(3U), "");
}