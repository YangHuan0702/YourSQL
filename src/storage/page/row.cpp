//
// Created by huan.yang on 2026-03-04.
//
#include "storage/page/row.h"

#include <cstring>
#include <stdexcept>

using namespace YourSQL;

auto Row::Deserialize(const Tuple &tuple) -> void {
    auto data = tuple.data_;
    this->schema_ = tuple.schema_;

    size_t meta_size = schema_.columns_.size();

    size_t offset = meta_size;
    for (size_t i = 0; i < schema_.columns_.size(); i++) {
        if (data[i] == '0') {
            values_.emplace_back(true);
        } else {
            switch (schema_.columns_[i].column_types) {
                case ColumnTypes::BOOL: {
                    bool val = false;
                    memcpy(&val, data + offset, sizeof(bool));
                    values_.emplace_back(val);
                    offset += sizeof(bool);
                    break;
                }
                case ColumnTypes::INTEGER: {
                    int val = 0;
                    memcpy(&val, data + offset, sizeof(int));
                    offset += sizeof(int);
                    values_.emplace_back(val);
                    break;
                }
                case ColumnTypes::TIMESTAMP: {
                    long long val = 0;
                    memcpy(&val, data + offset, sizeof(long long));
                    offset += sizeof(long long);
                    values_.emplace_back(val);
                    break;
                }
                case ColumnTypes::VARCHAR: {
                    size_t val = 0;
                    memcpy(&val, data + offset, sizeof(size_t));
                    offset += sizeof(size_t);

                    values_.emplace_back(std::string(data + offset, val));
                    offset += val;
                    break;
                }
                default: throw std::invalid_argument("Invalid column type");
            }
        }
    }
}

auto Row::GetValue(size_t index) -> Value {
    if (index >= schema_.columns_.size()) {
        throw std::invalid_argument("index out of range");
    }
    return values_[index];
}

auto Row::GetValue(const std::string &column_name) -> Value {
    for (size_t i = 0; i < schema_.columns_.size(); ++i) {
        if (schema_.columns_[i].name_ == column_name) {
            return GetValue(i);
        }
    }
    throw std::invalid_argument("[Row::GetValue]column not found");
}

auto Row::Serialize() const -> char * {
    char *meta = new char[schema_.columns_.size()];
    size_t size = schema_.columns_.size();
    for (size_t i = 0; i < schema_.columns_.size(); ++i) {
        if (values_[i].IsNull()) {
            meta[i] = '0';
        } else {
            meta[i] = '1';
            switch (schema_.columns_[i].column_types) {
                case ColumnTypes::BOOL: size += 1;
                    break;
                case ColumnTypes::INTEGER: size += sizeof(int);
                    break;
                case ColumnTypes::TIMESTAMP: size += sizeof(long long);
                    break;
                case ColumnTypes::VARCHAR: {
                    size += sizeof(size_t);
                    size += values_[i].GetString().size();
                    break;
                }
                default: {
                    delete [] meta;
                    throw std::invalid_argument("[Row::Serialize()] invalid column type");
                }
            }
        }
    }

    char *data = new char[size];
    memcpy(data, meta, schema_.columns_.size());
    delete [] meta;
    size_t begin_offset = schema_.columns_.size();
    for (size_t i = 0; i < schema_.columns_.size(); ++i) {
        if (!values_[i].IsNull()) {
            switch (schema_.columns_[i].column_types) {
                case ColumnTypes::BOOL: {
                    char bl = values_[i].GetBool() ? '1' : '0';
                    memcpy(data + begin_offset, &bl, sizeof(char));
                    begin_offset += 1;
                    break;
                }
                case ColumnTypes::INTEGER: {
                    int val = values_[i].GetInt();
                    memcpy(data + begin_offset, &val, sizeof(int));
                    begin_offset += sizeof(int);
                    break;
                }
                case ColumnTypes::TIMESTAMP: {
                    long long val = values_[i].GetTimestamp();
                    memcpy(data + begin_offset, &val, sizeof(long long));
                    begin_offset += sizeof(long long);
                    break;
                }
                case ColumnTypes::VARCHAR: {
                    size_t len = values_[i].GetString().size();
                    memcpy(data + begin_offset, &len, sizeof(size_t));
                    begin_offset += sizeof(size_t);
                    memcpy(data + begin_offset, values_[i].GetString().c_str(), len);
                    begin_offset += len;
                    break;
                }
                default: {
                    delete [] meta;
                    delete [] data;
                    throw std::invalid_argument("[Row::Serialize()] invalid column type");
                }
            }
        }
    }
    return data;
}

auto Row::SetValue(size_t index, const Value &value) -> void {
    if (index >= schema_.columns_.size()) {
        throw std::invalid_argument("index out of range");
    }
    values_[index] = value;
}
