//
// Created by huan.yang on 2026-03-03.
//
#pragma once

namespace YourSQL {

#define PAGE_SIZE 4096
#define BUFFER_MAX_PAGE 128

#define PAGE_HEADER_MAGIC 0xDEADBEEF
#define MAX_OPEN_FILES 512


#define DATA_PATH "/data/YourSQL"
#define DATA_FILE_NAME "ydb.s"

}