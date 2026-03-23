//
// Created by huan.yang on 2026-03-07.
//
#include "buffer/meta_page.h"
#include "common/constant.h"
#include "glog/logging.h"

using namespace YourSQL;

auto MetaPage::WriteLastPoint() -> void {
    memcpy(meta_page_->data_ + sizeof(size_t) * 2, &last_point_,sizeof(size_t));
    meta_page_->is_dirty_ = true;
    buffer_manager_->Flush(meta_page_->id_);
}

auto MetaPage::Init() -> void {
    LOG(INFO) << "Init - calling Reset and initializing header";
    // meta_page_->Reset();
    size_t table_s = 0;
    memcpy(&table_s,meta_page_->data_ + sizeof(size_t),sizeof(size_t));
    if (buffer_manager_->CurrFileSize() > 0 && table_s > 0) {
        ReadMata();
        return;
    }
    version_ = META_INIT_VERSION;
    table_size_ = META_INIT_TABLE_SIZE;
    last_point_ = META_INIT_LAST_POINT;

    memcpy(meta_page_->data_,&version_,sizeof(size_t));
    memcpy(meta_page_->data_+sizeof(size_t),&table_size_,sizeof(size_t));
    memcpy(meta_page_->data_ + sizeof(size_t)*2,&last_point_,sizeof(size_t));

    LOG(INFO) << "Init - version: " << version_ << ", table_size: " << table_size_ << ", last_point: " << last_point_;
    meta_page_->is_dirty_ = true;
    buffer_manager_->Flush(meta_page_->id_);
}


auto MetaPage::ReadMata() -> void {
    memcpy(&version_,meta_page_->data_,sizeof(size_t));
    memcpy(&table_size_,meta_page_->data_ + sizeof(size_t),sizeof(size_t));
    memcpy(&last_point_,meta_page_->data_ + sizeof(size_t)*2,sizeof(size_t));
    if (table_size_ == 0) {
        return ;
    }

    size_t offset = sizeof(size_t) + sizeof(size_t)+ sizeof(size_t);
    for (size_t i = 0; i < table_size_; i++) {

        size_t cur_offset = offset;

        size_t len = 0;
        memcpy(&len, meta_page_->data_ + offset, sizeof(size_t));
        offset += sizeof(size_t);

        std::string table_name = std::string(meta_page_->data_+offset,len);
        offset += len;
        LOG(INFO) << "Reading table name: " << table_name;

        page_id_t first_id = 0;
        memcpy(&first_id, meta_page_->data_ + offset, sizeof(page_id_t));
        offset += sizeof(page_id_t);

        size_t rows = 0;
        memcpy(&rows, meta_page_->data_ + offset, sizeof(size_t));
        offset += sizeof(size_t);

        entry_id table_id = 0;
        memcpy(&table_id, meta_page_->data_ + offset, sizeof(entry_id));
        offset += sizeof(entry_id);

        page_id_t last_page_id = 0;
        memcpy(&last_page_id, meta_page_->data_ + offset, sizeof(page_id_t));
        offset += sizeof(page_id_t);
        offset += sizeof(size_t);

        MetaItem item;
        item.table_name_ = table_name;
        item.first_page_id = first_id;
        item.num_rows_ = rows;
        item.table_id_ = table_id;
        item.last_page_id = last_page_id;
        item.offset = cur_offset;

        size_t column_size = 0;
        memcpy(&column_size,meta_page_->data_ + offset,sizeof(size_t));
        offset += sizeof(size_t);
        if (column_size > 0) {
            for (size_t index = 0; index<column_size;index++) {
                size_t name_size = 0;
                memcpy(&name_size,meta_page_->data_ + offset, sizeof(size_t));
                offset += sizeof(size_t);

                std::string name(meta_page_->data_+offset, name_size);
                offset += name_size;

                ColumnTypes type;
                memcpy(&type,meta_page_->data_+offset,sizeof(ColumnTypes));
                offset += sizeof(ColumnTypes);

                uint8_t flags;
                memcpy(&flags,meta_page_->data_+offset,sizeof(uint8_t));
                offset += sizeof(uint8_t);

                entry_id c_id;
                memcpy(&c_id,meta_page_->data_ + offset,sizeof(entry_id));
                offset += sizeof(entry_id);

                MetaColumnItem c_item;
                c_item.column_name_ = name;
                c_item.type_ = type;
                c_item.flags_ = flags;
                c_item.column_id_ = c_id;

                item.items_.push_back(c_item);
            }
        }

        name_tables_[table_name] = first_id;
        id_tables_[table_id] = first_id;
        items_[table_id] = item;



    }
    last_point_ = offset;
}



auto MetaPage::GetFirstPageId(const std::string &table_name) -> page_id_t {
    if (name_tables_.find(table_name) == name_tables_.end()) {
        throw std::runtime_error("MetaPage::GetFirstPageId for tableName does not exist");
    }
    return name_tables_[table_name];
}

auto MetaPage::GetFirstPageId(entry_id table_id) -> page_id_t {
    if (id_tables_.find(table_id) == id_tables_.end()) {
        throw std::runtime_error("MetaPage::GetFirstPageId for tableId does not exist");
    }
    return id_tables_[table_id];
}


auto MetaPage::GetNameLen(const MetaItem &item) -> size_t {
    size_t len = 0;
    memcpy(&len,meta_page_->data_ + item.offset, sizeof(size_t));
    return len;
}

auto MetaPage::UpdateTableSize(size_t change_size) -> void {
    table_size_ += change_size;
    LOG(INFO) << "UpdateTableSize - writing table_size: " << table_size_ << " at offset: " << sizeof(size_t);
    memcpy(meta_page_->data_+sizeof(size_t), &table_size_, sizeof(size_t));
    meta_page_->is_dirty_ = true;
    buffer_manager_->Flush(meta_page_->id_);
}


auto MetaPage::UpdateTableRows(entry_id table_id, size_t change_size) -> void {
    if (items_.find(table_id) == items_.end()) {
        throw std::runtime_error("MetaPage::UpdateTableRows don`t find target table_id");
    }
    auto &item = items_[table_id];
    item.num_rows_ += change_size;

    size_t name_len = GetNameLen(item);
    size_t offset = item.offset + sizeof(size_t) + name_len + sizeof(page_id_t);

    memcpy(meta_page_->data_+offset, &item.num_rows_, sizeof(size_t));
    meta_page_->is_dirty_ = true;
    buffer_manager_->Flush(meta_page_->id_);
}

auto MetaPage::UpdateTableFirstId(entry_id table_id, page_id_t first_page_id) -> void {
    if (items_.find(table_id) == items_.end()) {
        throw std::runtime_error("MetaPage::UpdateTableLastId don`t find target table_id");
    }

    auto &item = items_[table_id];
    item.first_page_id = first_page_id;

    size_t name_len = GetNameLen(item);
    size_t offset = item.offset + sizeof(size_t) + name_len ;

    memcpy(meta_page_->data_+offset, &item.first_page_id, sizeof(page_id_t));
    meta_page_->is_dirty_ = true;
    buffer_manager_->Flush(meta_page_->id_);
}


auto MetaPage::UpdateTableLastId(entry_id table_id, page_id_t last_page_id) -> void {
    if (items_.find(table_id) == items_.end()) {
        throw std::runtime_error("MetaPage::UpdateTableLastId don`t find target table_id");
    }
    auto &item = items_[table_id];
    item.last_page_id = last_page_id;

    size_t name_len = GetNameLen(item);
    size_t offset = item.offset + sizeof(size_t) + name_len + sizeof(page_id_t) + sizeof(size_t) + sizeof(entry_id);

    memcpy(meta_page_->data_+offset, &item.last_page_id, sizeof(page_id_t));
    meta_page_->is_dirty_ = true;
    buffer_manager_->Flush(meta_page_->id_);
}

auto MetaPage::AddTable(MetaItem &item) -> void {

    size_t item_offset = last_point_ == 0 ? ITEMS_OFFSET_BEGIN : last_point_;
    item.offset = item_offset;

    size_t need_size = ITEM_FIXED_SIZE + item.table_name_.size() ;

    if (last_point_ + need_size >= PAGE_SIZE) {
        throw std::runtime_error("MetaPage::AddTable: Not enough space for last page");
    }
    size_t name_len = item.table_name_.size();
    LOG(INFO) << "Writing table_name_len: " << name_len << ", table_name: " << item.table_name_;

    if (item.first_page_id == 0) {
        item.first_page_id = item.table_id_;
    }
    if (item.last_page_id == 0) {
        item.last_page_id = item.table_id_;
    }

    memcpy(meta_page_->data_+last_point_,&name_len,sizeof(size_t));
    last_point_ += sizeof(size_t);
    memcpy(meta_page_->data_+last_point_,item.table_name_.c_str(),name_len);
    last_point_ += name_len;
    memcpy(meta_page_->data_+last_point_,&item.first_page_id,sizeof(page_id_t));
    last_point_ += sizeof(page_id_t);
    memcpy(meta_page_->data_+last_point_,&item.num_rows_,sizeof(size_t));
    last_point_ += sizeof(size_t);
    memcpy(meta_page_->data_+last_point_,&item.table_id_,sizeof(entry_id));
    last_point_ += sizeof(entry_id);
    memcpy(meta_page_->data_+last_point_,&item.last_page_id,sizeof(page_id_t));
    last_point_ += sizeof(page_id_t);
    memcpy(meta_page_->data_+last_point_,&item.offset,sizeof(size_t));
    last_point_ += sizeof(size_t);

    size_t column_size = item.items_.size();
    memcpy(meta_page_->data_+last_point_,&column_size,sizeof(size_t));
    last_point_ += sizeof(size_t);

    for (const auto &meta_column_item : item.items_) {
        size_t name_size = meta_column_item.column_name_.size();
        memcpy(meta_page_->data_+last_point_,&name_size,sizeof(size_t));
        last_point_ += sizeof(size_t);

        memcpy(meta_page_->data_ + last_point_, meta_column_item.column_name_.c_str(),name_size);
        last_point_ += name_size;


        memcpy(meta_page_->data_ + last_point_,&meta_column_item.type_,sizeof(ColumnTypes));
        last_point_ += sizeof(ColumnTypes);

        memcpy(meta_page_->data_ + last_point_, &meta_column_item.flags_,sizeof(uint8_t));
        last_point_ += sizeof(uint8_t);

        memcpy(meta_page_->data_ + last_point_, &meta_column_item.column_id_,sizeof(entry_id));
        last_point_ += sizeof(entry_id);
        LOG(INFO) << "After writing column '" << meta_column_item.column_name_ << "', last_point: " << last_point_;
    }
    name_tables_[item.table_name_] = item.first_page_id;
    id_tables_[item.table_id_] = item.first_page_id;
    items_[item.table_id_] = item;

    WriteLastPoint();
    UpdateTableSize(1);
}