#include <gtest/gtest.h>

#include "arena_pool.hpp"

TEST(ArenaPool, Construct) {
    const auto capacity = 256;
    arena_pool<int> pool(capacity);
    EXPECT_EQ(pool.capacity(), capacity);
    EXPECT_EQ(pool.used(), 0);
    EXPECT_FALSE(pool.empty());
    EXPECT_TRUE(pool.full());
}

TEST(ArenaPool, Allocate) {
    const auto capacity = 256;
    arena_pool<int> pool(capacity);

    for (auto i = 1; i <= capacity; ++i) {
        EXPECT_TRUE(pool.allocate());
        EXPECT_EQ(pool.capacity(), capacity);
        EXPECT_EQ(pool.used(), i);
        EXPECT_EQ(pool.empty(), i == capacity);
        EXPECT_FALSE(pool.full());
    }

    // allocate from empty pool
    const auto ptr = pool.allocate();
    EXPECT_FALSE(ptr);
    EXPECT_EQ(pool.capacity(), capacity);
    EXPECT_EQ(pool.used(), capacity);
    EXPECT_TRUE(pool.empty());
    EXPECT_FALSE(pool.full());
}

TEST(ArenaPool, AllocateEmpty) {
    const auto capacity = 1;
    arena_pool<int> pool(capacity);
    ASSERT_TRUE(pool.allocate());
    ASSERT_TRUE(pool.empty());

    EXPECT_FALSE(pool.allocate());
    EXPECT_TRUE(pool.empty());
    EXPECT_FALSE(pool.full());
}

TEST(ArenaPool, Deallocate) {
    const auto capacity = 256;
    arena_pool<int> pool(capacity);

    for (auto i = 1; i <= capacity * 2; ++i) {
        const auto ptr = pool.allocate();
        ASSERT_TRUE(ptr);
        ASSERT_FALSE(pool.full());

        pool.deallocate(ptr);
        EXPECT_EQ(pool.capacity(), capacity);
        EXPECT_EQ(pool.used(), 0);
        EXPECT_FALSE(pool.empty());
        EXPECT_TRUE(pool.full());
    }
}

TEST(ArenaPool, DeallocateNullptr) {
    const auto capacity = 256;
    arena_pool<int> pool(capacity);
    ASSERT_TRUE(pool.allocate());
    ASSERT_EQ(pool.used(), 1);
    ASSERT_FALSE(pool.full());

    int* n_ptr = nullptr;
    EXPECT_DEATH(pool.deallocate(n_ptr), "attempted to deallocate nullptr");
    EXPECT_EQ(pool.used(), 1);
    EXPECT_FALSE(pool.full());
}

TEST(ArenaPool, DeallocateForeignPtr) {
    const auto capacity = 256;
    arena_pool<int> pool(capacity);
    ASSERT_TRUE(pool.allocate());
    ASSERT_EQ(pool.used(), 1);
    ASSERT_FALSE(pool.full());

    auto f_ptr = new int;
    EXPECT_DEATH(pool.deallocate(f_ptr), "attempted to deallocate foreign ptr");
    EXPECT_EQ(pool.used(), 1);
    EXPECT_FALSE(pool.full());
}

TEST(ArenaPool, DoubleDeallocate) {
    const auto capacity = 256;
    arena_pool<int> pool(capacity);
    const auto p1 = pool.allocate();
    ASSERT_TRUE(p1);
    const auto p2 = pool.allocate();
    ASSERT_TRUE(p2);
    ASSERT_EQ(pool.used(), 2);
    ASSERT_FALSE(pool.full());

    pool.deallocate(p1);
    EXPECT_EQ(pool.used(), 1);
    EXPECT_FALSE(pool.full());
    EXPECT_DEATH(pool.deallocate(p1), "attempted double deallocate");
    EXPECT_EQ(pool.used(), 1);
    EXPECT_FALSE(pool.full());

    pool.deallocate(p2);
    EXPECT_EQ(pool.used(), 0);
    EXPECT_TRUE(pool.full());
}