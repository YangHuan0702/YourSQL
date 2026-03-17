//
// Created by huan.yang on 2026-03-17.
//

#include "gtest/gtest.h"
#include "index/b_plus_leaf_page.h"
#include "index/b_plus_internal_page.h"
#include "storage/posix_disk_manager.h"
#include "buffer/buffer_manager.h"
#include "glog/logging.h"

using namespace YourSQL;

class BPlusLeafPageTest : public ::testing::Test {
protected:
    void SetUp() override {
        disk_manager_ = std::make_shared<PosixDiskManager>();
        buffer_manager_ = std::make_shared<BufferManager>(disk_manager_);
        page_ = buffer_manager_->NewPage();
        ASSERT_NE(page_, nullptr);
        leaf_page_ = new BPlusLeafPage<int, int>(page_);
        leaf_page_->Init(page_->id_, INVALID_PAGE_ID, leaf_page_->GetMaxSize());
    }

    void TearDown() override {
        delete leaf_page_;
        buffer_manager_->Release(page_);
        buffer_manager_.reset();
        disk_manager_.reset();
    }

    std::shared_ptr<PosixDiskManager> disk_manager_;
    std::shared_ptr<BufferManager> buffer_manager_;
    Page *page_;
    BPlusLeafPage<int, int> *leaf_page_;
};

TEST_F(BPlusLeafPageTest, InitialState) {
    ASSERT_EQ(leaf_page_->GetSize(), 0);
    ASSERT_TRUE(leaf_page_->IsLeafPage());
    ASSERT_EQ(leaf_page_->GetParentPageId(), INVALID_PAGE_ID);
    ASSERT_EQ(leaf_page_->GetNextPageId(), INVALID_PAGE_ID);
}

TEST_F(BPlusLeafPageTest, InsertSingleElement) {
    ASSERT_TRUE(leaf_page_->Insert(10, 100));
    ASSERT_EQ(leaf_page_->GetSize(), 1);
    ASSERT_EQ(leaf_page_->KeyAt(0), 10);
    ASSERT_EQ(leaf_page_->ValueAt(0), 100);
}

TEST_F(BPlusLeafPageTest, InsertMultipleElements) {
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(leaf_page_->Insert(i, i * 10));
    }

    ASSERT_EQ(leaf_page_->GetSize(), 10);

    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(leaf_page_->KeyAt(i), i);
        ASSERT_EQ(leaf_page_->ValueAt(i), i * 10);
    }
}

TEST_F(BPlusLeafPageTest, InsertInOrder) {
    std::vector<int> keys = {5, 2, 8, 1, 9, 3};

    for (int key : keys) {
        ASSERT_TRUE(leaf_page_->Insert(key, key * 10));
    }

    ASSERT_EQ(leaf_page_->GetSize(), 6);

    for (int i = 0; i < leaf_page_->GetSize() - 1; i++) {
        ASSERT_LT(leaf_page_->KeyAt(i), leaf_page_->KeyAt(i + 1));
    }
}

TEST_F(BPlusLeafPageTest, LookupExistingKey) {
    leaf_page_->Insert(10, 100);
    leaf_page_->Insert(20, 200);
    leaf_page_->Insert(30, 300);

    std::vector<int> result;
    ASSERT_TRUE(leaf_page_->Lookup(20, result));
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0], 200);
}

TEST_F(BPlusLeafPageTest, LookupNonExistingKey) {
    leaf_page_->Insert(10, 100);
    leaf_page_->Insert(20, 200);

    std::vector<int> result;
    ASSERT_FALSE(leaf_page_->Lookup(15, result));
    ASSERT_EQ(result.size(), 0);
}

TEST_F(BPlusLeafPageTest, LookupDuplicateKeys) {
    leaf_page_->Insert(10, 100);
    leaf_page_->Insert(10, 101);
    leaf_page_->Insert(10, 102);

    std::vector<int> result;
    ASSERT_TRUE(leaf_page_->Lookup(10, result));
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 100);
    ASSERT_EQ(result[1], 101);
    ASSERT_EQ(result[2], 102);
}

TEST_F(BPlusLeafPageTest, RemoveExistingKey) {
    leaf_page_->Insert(10, 100);
    leaf_page_->Insert(20, 200);
    leaf_page_->Insert(30, 300);

    ASSERT_EQ(leaf_page_->Remove(20), 1);
    ASSERT_EQ(leaf_page_->GetSize(), 2);

    std::vector<int> result;
    ASSERT_FALSE(leaf_page_->Lookup(20, result));
}

TEST_F(BPlusLeafPageTest, RemoveNonExistingKey) {
    leaf_page_->Insert(10, 100);
    leaf_page_->Insert(20, 200);

    ASSERT_EQ(leaf_page_->Remove(15), 0);
    ASSERT_EQ(leaf_page_->GetSize(), 2);
}

TEST_F(BPlusLeafPageTest, RemoveAllElements) {
    leaf_page_->Insert(10, 100);
    leaf_page_->Insert(20, 200);
    leaf_page_->Insert(30, 300);

    ASSERT_EQ(leaf_page_->Remove(10), 1);
    ASSERT_EQ(leaf_page_->Remove(20), 1);
    ASSERT_EQ(leaf_page_->Remove(30), 1);

    ASSERT_EQ(leaf_page_->GetSize(), 0);
}

TEST_F(BPlusLeafPageTest, LowerBound) {
    leaf_page_->Insert(10, 100);
    leaf_page_->Insert(20, 200);
    leaf_page_->Insert(30, 300);
    leaf_page_->Insert(40, 400);

    ASSERT_EQ(leaf_page_->LowerBound(15), 1);
    ASSERT_EQ(leaf_page_->LowerBound(20), 1);
    ASSERT_EQ(leaf_page_->LowerBound(5), 0);
    ASSERT_EQ(leaf_page_->LowerBound(50), 4);
}

TEST_F(BPlusLeafPageTest, NextPageId) {
    ASSERT_EQ(leaf_page_->GetNextPageId(), INVALID_PAGE_ID);

    leaf_page_->SetNextPageId(123);
    ASSERT_EQ(leaf_page_->GetNextPageId(), 123);

    leaf_page_->SetNextPageId(456);
    ASSERT_EQ(leaf_page_->GetNextPageId(), 456);
}

class BPlusInternalPageTest : public ::testing::Test {
protected:
    void SetUp() override {
        disk_manager_ = std::make_shared<PosixDiskManager>();
        buffer_manager_ = std::make_shared<BufferManager>(disk_manager_);
        page_ = buffer_manager_->NewPage();
        ASSERT_NE(page_, nullptr);
        internal_page_ = new BPlusInternalPage<int>(page_);
        internal_page_->Init(page_->id_, INVALID_PAGE_ID, 100);
    }

    void TearDown() override {
        delete internal_page_;
        buffer_manager_->Release(page_);
        buffer_manager_.reset();
        disk_manager_.reset();
    }

    std::shared_ptr<PosixDiskManager> disk_manager_;
    std::shared_ptr<BufferManager> buffer_manager_;
    Page *page_;
    BPlusInternalPage<int> *internal_page_;
};

TEST_F(BPlusInternalPageTest, InitialState) {
    ASSERT_EQ(internal_page_->GetSize(), 0);
    ASSERT_FALSE(internal_page_->IsLeafPage());
    ASSERT_EQ(internal_page_->GetParentPageId(), INVALID_PAGE_ID);
}

TEST_F(BPlusInternalPageTest, InsertAfter) {
    internal_page_->InsertAfter(0, 10, 1);
    ASSERT_EQ(internal_page_->GetSize(), 1);

    internal_page_->InsertAfter(1, 20, 2);
    ASSERT_EQ(internal_page_->GetSize(), 2);

    ASSERT_EQ(internal_page_->KeyAt(0), 10);
    ASSERT_EQ(internal_page_->ValueAt(0), 1);
    ASSERT_EQ(internal_page_->KeyAt(1), 20);
    ASSERT_EQ(internal_page_->ValueAt(1), 2);
}

TEST_F(BPlusInternalPageTest, SetKeyAndValue) {
    internal_page_->InsertAfter(0, 10, 1);
    internal_page_->InsertAfter(1, 20, 2);

    internal_page_->SetKeyAt(0, 15);
    internal_page_->SetValueAt(1, 5);

    ASSERT_EQ(internal_page_->KeyAt(0), 15);
    ASSERT_EQ(internal_page_->ValueAt(1), 5);
}

TEST_F(BPlusInternalPageTest, Remove) {
    internal_page_->InsertAfter(0, 10, 1);
    internal_page_->InsertAfter(1, 20, 2);
    internal_page_->InsertAfter(2, 30, 3);

    internal_page_->Remove(1);
    ASSERT_EQ(internal_page_->GetSize(), 2);

    ASSERT_EQ(internal_page_->KeyAt(0), 10);
    ASSERT_EQ(internal_page_->KeyAt(1), 30);
}

TEST_F(BPlusInternalPageTest, LookupChild) {
    internal_page_->InsertAfter(0, 10, 1);
    internal_page_->InsertAfter(1, 20, 2);
    internal_page_->InsertAfter(2, 30, 3);

    ASSERT_EQ(internal_page_->LookupChild(5), 1);
    ASSERT_EQ(internal_page_->LookupChild(15), 2);
    ASSERT_EQ(internal_page_->LookupChild(25), 3);
}
