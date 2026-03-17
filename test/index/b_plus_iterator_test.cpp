//
// Created by huan.yang on 2026-03-17.
//

#include "gtest/gtest.h"
#include "index/b_plus_index_manager.h"
#include "index/b_plus_index_iterator.h"
#include "storage/posix_disk_manager.h"
#include "buffer/buffer_manager.h"
#include "glog/logging.h"

using namespace YourSQL;

class BPlusIteratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        disk_manager_ = std::make_shared<PosixDiskManager>();
        buffer_manager_ = std::make_shared<BufferManager>(disk_manager_);
    }

    void TearDown() override {
        buffer_manager_.reset();
        disk_manager_.reset();
    }

    std::shared_ptr<PosixDiskManager> disk_manager_;
    std::shared_ptr<BufferManager> buffer_manager_;
};

TEST_F(BPlusIteratorTest, IterateEmptyTree) {
    BPlusIndexIterator<int, int> iter;
    ASSERT_TRUE(iter.IsEnd());
}

TEST_F(BPlusIteratorTest, IterateSingleElement) {
    Page *page = buffer_manager_->NewPage();
    ASSERT_NE(page, nullptr);

    auto leaf = reinterpret_cast<BPlusLeafPage<int, int> *>(page->data_);
    leaf->Init(page->id_, INVALID_PAGE_ID, leaf->GetMaxSize());
    leaf->Insert(10, 100);

    BPlusIndexIterator<int, int> iter(buffer_manager_, page->id_, 0);
    ASSERT_FALSE(iter.IsEnd());

    auto pair = *iter;
    ASSERT_EQ(pair.first, 10);
    ASSERT_EQ(pair.second, 100);

    ++iter;
    ASSERT_TRUE(iter.IsEnd());
}

TEST_F(BPlusIteratorTest, IterateMultipleElements) {
    Page *page = buffer_manager_->NewPage();
    ASSERT_NE(page, nullptr);

    auto leaf = reinterpret_cast<BPlusLeafPage<int, int> *>(page->data_);
    leaf->Init(page->id_, INVALID_PAGE_ID, leaf->GetMaxSize());

    for (int i = 0; i < 10; i++) {
        leaf->Insert(i, i * 10);
    }

    BPlusIndexIterator<int, int> iter(buffer_manager_, page->id_, 0);
    int count = 0;

    while (!iter.IsEnd()) {
        auto pair = *iter;
        ASSERT_EQ(pair.first, count);
        ASSERT_EQ(pair.second, count * 10);
        ++iter;
        count++;
    }

    ASSERT_EQ(count, 10);
}

TEST_F(BPlusIteratorTest, IterateAcrossPages) {
    Page *page1 = buffer_manager_->NewPage();
    Page *page2 = buffer_manager_->NewPage();
    ASSERT_NE(page1, nullptr);
    ASSERT_NE(page2, nullptr);

    auto leaf1 = reinterpret_cast<BPlusLeafPage<int, int> *>(page1->data_);
    auto leaf2 = reinterpret_cast<BPlusLeafPage<int, int> *>(page2->data_);

    leaf1->Init(page1->id_, INVALID_PAGE_ID, leaf1->GetMaxSize());
    leaf2->Init(page2->id_, INVALID_PAGE_ID, leaf2->GetMaxSize());

    for (int i = 0; i < 5; i++) {
        leaf1->Insert(i, i * 10);
    }

    for (int i = 5; i < 10; i++) {
        leaf2->Insert(i, i * 10);
    }

    leaf1->SetNextPageId(page2->id_);

    BPlusIndexIterator<int, int> iter(buffer_manager_, page1->id_, 0);
    int count = 0;

    while (!iter.IsEnd()) {
        auto pair = *iter;
        ASSERT_EQ(pair.first, count);
        ASSERT_EQ(pair.second, count * 10);
        ++iter;
        count++;
    }

    ASSERT_EQ(count, 10);
}

TEST_F(BPlusIteratorTest, IteratorEquality) {
    Page *page = buffer_manager_->NewPage();
    ASSERT_NE(page, nullptr);

    auto leaf = reinterpret_cast<BPlusLeafPage<int, int> *>(page->data_);
    leaf->Init(page->id_, INVALID_PAGE_ID, leaf->GetMaxSize());
    leaf->Insert(10, 100);
    leaf->Insert(20, 200);

    BPlusIndexIterator<int, int> iter1(buffer_manager_, page->id_, 0);
    BPlusIndexIterator<int, int> iter2(buffer_manager_, page->id_, 0);

    ASSERT_TRUE(iter1 == iter2);
    ASSERT_FALSE(iter1 != iter2);

    ++iter1;
    ASSERT_FALSE(iter1 == iter2);
    ASSERT_TRUE(iter1 != iter2);

    ++iter2;
    ASSERT_TRUE(iter1 == iter2);
}

TEST_F(BPlusIteratorTest, IteratorStartFromMiddle) {
    Page *page = buffer_manager_->NewPage();
    ASSERT_NE(page, nullptr);

    auto leaf = reinterpret_cast<BPlusLeafPage<int, int> *>(page->data_);
    leaf->Init(page->id_, INVALID_PAGE_ID, leaf->GetMaxSize());

    for (int i = 0; i < 10; i++) {
        leaf->Insert(i, i * 10);
    }

    BPlusIndexIterator<int, int> iter(buffer_manager_, page->id_, 5);
    ASSERT_FALSE(iter.IsEnd());

    auto pair = *iter;
    ASSERT_EQ(pair.first, 5);
    ASSERT_EQ(pair.second, 50);

    int count = 5;
    while (!iter.IsEnd()) {
        auto p = *iter;
        ASSERT_EQ(p.first, count);
        ++iter;
        count++;
    }

    ASSERT_EQ(count, 10);
}

TEST_F(BPlusIteratorTest, IteratorIncrementAtEnd) {
    Page *page = buffer_manager_->NewPage();
    ASSERT_NE(page, nullptr);

    auto leaf = reinterpret_cast<BPlusLeafPage<int, int> *>(page->data_);
    leaf->Init(page->id_, INVALID_PAGE_ID, leaf->GetMaxSize());
    leaf->Insert(10, 100);

    BPlusIndexIterator<int, int> iter(buffer_manager_, page->id_, 0);
    ++iter;
    ASSERT_TRUE(iter.IsEnd());

    ++iter;
    ASSERT_TRUE(iter.IsEnd());
}
