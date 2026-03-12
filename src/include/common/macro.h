//
// Created by huan.yang on 2026-03-03.
//
#pragma once

namespace YourSQL {

#define PAGE_SIZE 4096
#define BUFFER_MAX_PAGE 128

#define PAGE_HEADER_MAGIC 0xDEADBEEF
#define MAX_OPEN_FILES 512

#define INVALID_PAGE_ID 0

#define DATA_PATH "data"
#define DATA_FILE_NAME "ydb.sdb"

}