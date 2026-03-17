//
// Created by huan.yang on 2026-03-17.
//

#include "gtest/gtest.h"
#include "index/b_plus_index_manager.h"
#include "storage/posix_disk_manager.h"
#include "buffer/buffer_manager.h"
#include "glog/logging.h"

using namespace YourSQL;

class BPlusSplitCoalesceTest : public ::testing::Test {
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

TEST_F(BPlusSplitCoalesceTest, TriggerLeafSplit) {
    const int count = 100;
    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));
    }

    for (int i = 0; i < count; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0], i * 10);
    }
}

TEST_F(BPlusSplitCoalesceTest, TriggerInternalSplit) {
    const int count = 500;
    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 5));
    }

    for (int i = 0; i < count; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0], i * 5);
    }
}

TEST_F(BPlusSplitCoalesceTest, TriggerCoalesce) {
    const int count = 100;
    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));
    }

    for (int i = 0; i < count / 2; i++) {
        ASSERT_TRUE(index_manager_->Delete(i));
    }

    for (int i = 0; i < count / 2; i++) {
        std::vector<int> result;
        ASSERT_FALSE(index_manager_->Get(i, result));
    }

    for (int i = count / 2; i < count; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        ASSERT_EQ(result[0], i * 10);
    }
}

TEST_F(BPlusSplitCoalesceTest, TriggerRedistribute) {
    for (int i = 0; i < 50; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));
    }

    for (int i = 10; i < 20; i++) {
        ASSERT_TRUE(index_manager_->Delete(i));
    }

    for (int i = 0; i < 10; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
    }

    for (int i = 20; i < 50; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
    }
}

TEST_F(BPlusSplitCoalesceTest, InsertDeleteInsert) {
    for (int i = 0; i < 100; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));
    }

    for (int i = 0; i < 100; i += 2) {
        ASSERT_TRUE(index_manager_->Delete(i));
    }

    for (int i = 0; i < 100; i += 2) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 20));
    }

    for (int i = 0; i < 100; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        if (i % 2 == 0) {
            ASSERT_EQ(result[0], i * 20);
        } else {
            ASSERT_EQ(result[0], i * 10);
        }
    }
}

TEST_F(BPlusSplitCoalesceTest, StressTestMixed) {
    const int operations = 1000;
    std::set<int> inserted_keys;

    for (int i = 0; i < operations; i++) {
        int key = rand() % 500;

        if (inserted_keys.find(key) == inserted_keys.end()) {
            ASSERT_TRUE(index_manager_->Insert(key, key * 100));
            inserted_keys.insert(key);
        } else {
            if (rand() % 2 == 0) {
                ASSERT_TRUE(index_manager_->Delete(key));
                inserted_keys.erase(key);
            }
        }
    }

    for (int key : inserted_keys) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(key, result));
    }
}

TEST_F(BPlusSplitCoalesceTest, DeleteAllAndReinsert) {
    for (int i = 0; i < 50; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));
    }

    for (int i = 0; i < 50; i++) {
        ASSERT_TRUE(index_manager_->Delete(i));
    }

    for (int i = 0; i < 50; i++) {
        std::vector<int> result;
        ASSERT_FALSE(index_manager_->Get(i, result));
    }

    for (int i = 0; i < 50; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 20));
    }

    for (int i = 0; i < 50; i++) {
        std::vector<int> result;
        ASSERT_TRUE(index_manager_->Get(i, result));
        ASSERT_EQ(result[0], i * 20);
    }
}

TEST_F(BPlusSplitCoalesceTest, SequentialInsertReverseDelete) {
    const int count = 200;

    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 5));
    }

    for (int i = count - 1; i >= 0; i--) {
        ASSERT_TRUE(index_manager_->Delete(i));

        for (int j = 0; j < i; j++) {
            std::vector<int> result;
            ASSERT_TRUE(index_manager_->Get(j, result));
        }
    }
}

TEST_F(BPlusSplitCoalesceTest, AlternatingInsertDelete) {
    for (int i = 0; i < 100; i++) {
        ASSERT_TRUE(index_manager_->Insert(i, i * 10));

        if (i > 0 && i % 10 == 0) {
            for (int j = i - 5; j < i; j++) {
                ASSERT_TRUE(index_manager_->Delete(j));
            }
        }
    }

    for (int i = 0; i < 100; i++) {
        std::vector<int> result;
        bool found = index_manager_->Get(i, result);

        if (i >= 95 || (i % 10 < 5 && i >= 10)) {
            ASSERT_TRUE(found);
        }
    }
}
