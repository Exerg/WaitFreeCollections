#include <gtest/gtest.h>

#include <wfc/unordered_map.hpp>

TEST(WaitFreeHashMap, Construction)
{
	wfc::unordered_map<std::size_t, std::size_t> map(4);

	ASSERT_TRUE(map.is_empty());
	ASSERT_EQ(map.size(), 0);
}

TEST(WaitFreeHashMap, Insertion)
{
	wfc::unordered_map<std::size_t, std::size_t> map(4);

	ASSERT_EQ(map.insert(0, 0), wfc::operation_result::success);
	ASSERT_EQ(map.insert(0, 0), wfc::operation_result::already_present);

	ASSERT_FALSE(map.is_empty());
	ASSERT_EQ(map.size(), 1);

	map.insert(1, 0);
	ASSERT_EQ(map.size(), 2);
}

TEST(WaitFreeHashMap, EmptyGet)
{
	wfc::unordered_map<std::size_t, std::size_t> map(4);

	std::optional<std::size_t> r = map.get(0);

	ASSERT_FALSE(r.has_value());
}

TEST(WaitFreeHashMap, Get)
{
	wfc::unordered_map<std::size_t, std::size_t> map(4);

	map.insert(0, 1);
	std::optional<std::size_t> r = map.get(0);

	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(*r, 1);
}

TEST(WaitFreeHashMap, Update)
{
	wfc::unordered_map<std::size_t, std::size_t> map(4);

	ASSERT_EQ(map.update(0, 5), wfc::operation_result::element_not_found);

	map.insert(0, 1);
	ASSERT_EQ(*map.get(0), 1);
	ASSERT_EQ(map.update(0, 5), wfc::operation_result::success);
	ASSERT_EQ(*map.get(0), 5);

	map.insert(2, 15);
	ASSERT_EQ(map.update(2, 15, 15), wfc::operation_result::success);
	ASSERT_EQ(map.update(2, 5, 15), wfc::operation_result::success);
	ASSERT_EQ(map.update(2, 0, 0), wfc::operation_result::expected_value_mismatch);
	ASSERT_EQ(*map.get(2), 5);

	map.remove(2);
	ASSERT_EQ(map.update(2, 0), wfc::operation_result::element_not_found);
}

TEST(WaitFreeHashMap, Remove)
{
	wfc::unordered_map<std::size_t, std::size_t> map(4);

	ASSERT_EQ(map.remove(0, 5), wfc::operation_result::element_not_found);

	map.insert(0, 3);
	map.insert(1, 2);

	ASSERT_EQ(map.remove(0), wfc::operation_result::success);
	ASSERT_EQ(map.remove(0), wfc::operation_result::element_not_found);
	ASSERT_FALSE(map.get(0).has_value());

	ASSERT_EQ(map.remove(1, 3), wfc::operation_result::expected_value_mismatch);
	ASSERT_TRUE(map.get(1).has_value());
	ASSERT_EQ(map.remove(1, 2), wfc::operation_result::success);
	ASSERT_FALSE(map.get(1).has_value());
}

TEST(WaitFreeHashMap, FullHashMapUpdate)
{
	wfc::unordered_map<unsigned char, std::size_t> map(4);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		map.insert(static_cast<unsigned char>(i), i);
	}

	ASSERT_EQ(map.size(), std::numeric_limits<unsigned char>::max() + 1);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		ASSERT_EQ(map.update(static_cast<unsigned char>(i), i * 2, i), wfc::operation_result::success);
	}

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		ASSERT_EQ(*map.get(static_cast<unsigned char>(i)), i * 2);
	}
}

TEST(WaitFreeHashMap, FullHashMapRemove)
{
	wfc::unordered_map<unsigned char, std::size_t> map(4);

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		map.insert(static_cast<unsigned char>(i), i);
	}

	ASSERT_EQ(map.size(), std::numeric_limits<unsigned char>::max() + 1);

	// remove half of the elements (alternate)
	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		if (i % 2 == 0)
		{
			ASSERT_EQ(map.remove(static_cast<unsigned char>(i), i), wfc::operation_result::success);
		}
	}

	for (std::size_t i = 0; i <= std::numeric_limits<unsigned char>::max(); ++i)
	{
		if (i % 2 == 0)
		{
			ASSERT_FALSE(map.get(static_cast<unsigned char>(i)).has_value());
		}
		else
		{
			ASSERT_TRUE(map.get(static_cast<unsigned char>(i)).has_value());
		}
	}
}

TEST(WaitFreeHashMap, FullHashMapGet)
{
	wfc::unordered_map<unsigned char, unsigned char> map(4);

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
	wfc::unordered_map<unsigned char, unsigned char> map(4);

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
