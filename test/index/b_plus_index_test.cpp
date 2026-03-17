//
// Created by huan.yang on 2026-03-17.
//

#include "gtest/gtest.h"
#include "index/b_plus_index_manager.h"
#include "storage/posix_disk_manager.h"
#include "buffer/buffer_manager.h"
#include "glog/logging.h"

using namespace YourSQL;

class BPlusIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        disk_manager_ = std::make_shared<PosixDiskManager>();
        buffer_manager_ = std::make_shared<BufferManager>(disk_manager_);
        index_manager_ = std::make_unique<BPlusIndexManager<int, int>>(buffer_manager_);
    }

    void TearDown() override {
        index_manager_.reset();
        buffer_manager_.reset();
        disk_manager_.reset();
    }

    std::shared_ptr<PosixDiskManager> disk_manager_;
    std::shared_ptr<BufferManager> buffer_manager_;
    std::unique_ptr<BPlusIndexManager<int, int>> index_manager_;
};

TEST_F(BPlusIndexTest, InsertSingleKey) {
    ASSERT_TRUE(index_manager_->Insert(10, 100));

    std::vector<int> result;
    ASSERT_TRUE(index_manager_->Get(10, result));
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0], 100);
}

TEST_F(BPlusIndexTest, InsertMultipleKeys) {
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));
    }

    for (int i = 0; i < 10; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0], i * 10);
    }
}

TEST_F(BPlusIndexTest, InsertDuplicateKeys) {
    ASSERT_TRUE(index_manager_->Insert(5, 50));
    ASSERT_TRUE(index_manager_->Insert(5, 51));
    ASSERT_TRUE(index_manager_->Insert(5, 52));

    std::vector<int> result;
    ASSERT_TRUE(index_manager_->Get(5, result));
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 50);
    ASSERT_EQ(result[1], 51);
    ASSERT_EQ(result[2], 52);
}

TEST_F(BPlusIndexTest, InsertReverseOrder) {
    for (int i = 99; i >= 0; i--) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 2));
    }

    for (int i = 0; i < 100; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0], i * 2);
    }
}

TEST_F(BPlusIndexTest, GetNonExistentKey) {
    ASSERT_TRUE(index_manager_->Insert(10, 100));

    std::vector<int> result;
    ASSERT_FALSE(index_manager_->Get(20, result));
    ASSERT_EQ(result.size(), 0);
}

TEST_F(BPlusIndexTest, DeleteSingleKey) {
    ASSERT_TRUE(index_manager_->Insert(10, 100));
    ASSERT_TRUE(index_manager_->Delete(10));

    std::vector<int> result;
    ASSERT_FALSE(index_manager_->Get(10, result));
}

TEST_F(BPlusIndexTest, DeleteNonExistentKey) {
    ASSERT_TRUE(index_manager_->Insert(10, 100));
    ASSERT_FALSE(index_manager_->Delete(20));

    std::vector<int> result;
    ASSERT_TRUE(index_manager_->Get(10, result));
    ASSERT_EQ(result.size(), 1);
}

TEST_F(BPlusIndexTest, DeleteAndReinsert) {
    ASSERT_TRUE(index_manager_->Insert(10, 100));
    ASSERT_TRUE(index_manager_->Delete(10));
    ASSERT_TRUE(index_manager_->Insert(10, 200));

    std::vector<int> result;
    ASSERT_TRUE(index_manager_->Get(10, result));
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0], 200);
}

TEST_F(BPlusIndexTest, DeleteMultipleKeys) {
    for (int i = 0; i < 20; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));
    }

    for (int i = 0; i < 20; i += 2) {
        ASSERT_TRUE(index_manager_->Delete(i));
    }

    for (int i = 0; i < 20; i++) {
        std::vector<int> result;
        if (i % 2 == 0) {
            ASSERT_FALSE(index_manager_->Get(i, result));
        } else {
            ASSERT_TRUE(index_manager_->Get(i, result));
            ASSERT_EQ(result[0], i * 10);
        }
    }
}

TEST_F(BPlusIndexTest, LargeScaleInsert) {
    const int count = 1000;
    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 3));
    }

    for (int i = 0; i < count; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0], i * 3);
    }
}

TEST_F(BPlusIndexTest, LargeScaleDelete) {
    const int count = 500;
    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 5));
    }

    for (int i = 0; i < count; i += 3) {
        ASSERT_TRUE(index_manager_->Delete(i));
    }

    for (int i = 0; i < count; i++) {
        std::vector<int> result;
        if (i % 3 == 0) {
            ASSERT_FALSE(index_manager_->Get(i, result));
        } else {
            ASSERT_TRUE(index_manager_->Get(i, result));
        }
    }
}

TEST_F(BPlusIndexTest, RandomInsertDelete) {
    std::vector<int> keys = {50, 20, 80, 10, 30, 70, 90, 5, 15, 25};

    for (int key : keys) {
        ASSERT_TRUE(index_manager_->Insert(key, key * 100));
    }

    ASSERT_TRUE(index_manager_->Delete(20));
    ASSERT_TRUE(index_manager_->Delete(80));

    std::vector<int> result;
    ASSERT_FALSE(index_manager_->Get(20, result));
    ASSERT_FALSE(index_manager_->Get(80, result));

    ASSERT_TRUE(index_manager_->Get(50, result));
    ASSERT_EQ(result[0], 5000);
}

TEST_F(BPlusIndexTest, EmptyTreeDelete) {
    ASSERT_FALSE(index_manager_->Delete(10));
}

TEST_F(BPlusIndexTest, EmptyTreeGet) {
    std::vector<int> result;
    ASSERT_FALSE(index_manager_->Get(10, result));
    ASSERT_EQ(result.size(), 0);
}
