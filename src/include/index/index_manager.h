//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include <vector>

namespace YourSQL {

    template<class KeyType,class ValType>
    class IndexManager {
    public:
        virtual ~IndexManager() = default;

        virtual auto Insert(const KeyType &key, const ValType &val) -> bool = 0;
        virtual auto Delete(const KeyType &key) -> bool = 0;
        virtual auto Get(const KeyType &key, std::vector<ValType> &re) -> bool = 0;
    };

}
