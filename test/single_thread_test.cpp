#include <gtest/gtest.h>

#include <wait_free_unordered_map.hpp>

TEST(WaitFreeHashMap, Construction) {
    wf::unordered_map<std::size_t, std::size_t> map(8);

    EXPECT_TRUE(map.is_empty());
    ASSERT_EQ(map.size(), 0);
}
