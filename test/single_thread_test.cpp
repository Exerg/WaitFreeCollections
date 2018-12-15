#include <gtest/gtest.h>

#include <wait_free_unordered_map.hpp>

TEST(WaitFreeHashMap, Construction)
{
	wf::unordered_map<std::size_t, std::size_t> map(4);

	ASSERT_TRUE(map.is_empty());
	ASSERT_EQ(map.size(), 0);
}

TEST(WaitFreeHashMap, Insertion)
{
	wf::unordered_map<std::size_t, std::size_t> map(4);

	ASSERT_EQ(map.insert(0, 0), wf::operation_result::success);
	ASSERT_EQ(map.insert(0, 0), wf::operation_result::already_present);

	ASSERT_FALSE(map.is_empty());
	ASSERT_EQ(map.size(), 1);

	map.insert(1, 0);
	ASSERT_EQ(map.size(), 2);
}

TEST(WaitFreeHashMap, EmptyGet)
{
	wf::unordered_map<std::size_t, std::size_t> map(4);

	std::optional<std::size_t> r = map.get(0);

	ASSERT_FALSE(r.has_value());
}

TEST(WaitFreeHashMap, Get)
{
	wf::unordered_map<std::size_t, std::size_t> map(4);

	map.insert(0, 1);
	std::optional<std::size_t> r = map.get(0);

	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(*r, 1);
}

TEST(WaitFreeHashMap, Update)
{
	wf::unordered_map<std::size_t, std::size_t> map(4);

	ASSERT_EQ(map.update(0, 5), wf::operation_result::element_not_found);

	map.insert(0, 1);
	ASSERT_EQ(*map.get(0), 1);
	ASSERT_EQ(map.update(0, 5), wf::operation_result::success);
	ASSERT_EQ(*map.get(0), 5);

	map.insert(2, 15);
	ASSERT_EQ(map.update(2, 15, 15), wf::operation_result::success);
	ASSERT_EQ(map.update(2, 5, 15), wf::operation_result::success);
	ASSERT_EQ(map.update(2, 0, 0), wf::operation_result::expected_value_mismatch);
	ASSERT_EQ(*map.get(2), 5);

	/*map.remove(2); FIXME: uncomment when remove is implemented.
	ASSERT_EQ(map.update(2, 0), wf::operation_result::element_not_found);*/
}

TEST(WaitFreeHashMap, FullHashMapUpdate)
{
	wf::unordered_map<unsigned char, std::size_t> map(4);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		map.insert(static_cast<unsigned char>(i), i);
	}

	ASSERT_EQ(map.size(), std::numeric_limits<unsigned char>::max() + 1);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		ASSERT_EQ(map.update(static_cast<unsigned char>(i), i * 2, i), wf::operation_result::success);
	}

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		ASSERT_EQ(*map.get(static_cast<unsigned char>(i)), i * 2);
	}
}

TEST(WaitFreeHashMap, FullHashMapGet)
{
	wf::unordered_map<unsigned char, unsigned char> map(4);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		map.insert(static_cast<unsigned char>(i), static_cast<unsigned char>(i));
	}

	ASSERT_EQ(map.size(), std::numeric_limits<unsigned char>::max() + 1);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		unsigned char value = *map.get(static_cast<unsigned char>(i));
		ASSERT_EQ(value, i);
	}
}

TEST(WaitFreeHashMap, FullHashMapVisist)
{
	wf::unordered_map<unsigned char, unsigned char> map(4);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		map.insert(static_cast<unsigned char>(i), static_cast<unsigned char>(i));
	}

	ASSERT_EQ(map.size(), std::numeric_limits<unsigned char>::max() + 1);

	int nbr_value = 0;
	map.visit([&nbr_value](std::pair<unsigned char, unsigned char> p) {
		++nbr_value;
		ASSERT_EQ(p.first, p.second);
	});

	ASSERT_EQ(nbr_value, std::numeric_limits<unsigned char>::max() + 1);
}
