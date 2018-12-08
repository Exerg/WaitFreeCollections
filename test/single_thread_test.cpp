#include <gtest/gtest.h>

#include <wait_free_unordered_map.hpp>

TEST(WaitFreeHashMap, Construction)
{
	wf::unordered_map<std::size_t, std::size_t> map(8);

	ASSERT_TRUE(map.is_empty());
	ASSERT_EQ(map.size(), 0);
}

TEST(WaitFreeHashMap, Insertion)
{
	wf::unordered_map<std::size_t, std::size_t> map(8);

	map.insert(0, 0);

	ASSERT_FALSE(map.is_empty());
	ASSERT_EQ(map.size(), 1);
}

TEST(WaitFreeHashMap, EmptyGet)
{
	wf::unordered_map<std::size_t, std::size_t> map(8);

	std::optional<std::size_t> r = map.get(0);

	ASSERT_FALSE(r.has_value());
}

TEST(WaitFreeHashMap, Get)
{
	wf::unordered_map<std::size_t, std::size_t> map(8);

	map.insert(0, 1);
	std::optional<std::size_t> r = map.get(0);

	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(*r, 1);
}

TEST(WaitFreeHashMap, FullHashMap)
{
	wf::unordered_map<unsigned char, unsigned char> map(8);

	for (std::size_t i = 0; i < std::numeric_limits<unsigned char>::max(); ++i)
	{
		map.insert(static_cast<unsigned char>(i), static_cast<unsigned char>(i));
	}

	for (std::size_t i = 0; i < std::numeric_limits<unsigned char>::max(); ++i)
	{
		unsigned char value = *map.get(static_cast<unsigned char>(i));
		ASSERT_EQ(value, i);
	}
}
