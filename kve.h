#ifndef KVE_H
#define KVE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_WIN32) || defined(_WIN64)
    #define KVE_API __declspec(dllexport)
#else
    #define KVE_API
#endif

typedef enum {
    ARG_END = 0,
    ARG_KEY,
    ARG_VALUE,
    ARG_OUT_VALUE,
    ARG_OUT_VAL_SZ,
    ARG_BRANCH_NAME,
    ARG_TARGET_COMMIT_ID
} ArgTag;

typedef struct {
    char *name;
    uint32_t latest_commit_id;
} BranchMeta;

typedef struct KVEngine {
    FILE *db_file;
    BranchMeta *branch_table;
    int branch_count;
    int branch_capacity;
    char *active_branch;
    uint32_t global_commit_counter;

    void (*set)(struct KVEngine *this_ptr, int dummy, ...);
    int  (*get)(struct KVEngine *this_ptr, int dummy, ...);
    void (*branch)(struct KVEngine *this_ptr, int dummy, ...);
    void (*compact)(struct KVEngine *this_ptr, int dummy, ...);
    void (*close)(struct KVEngine *this_ptr);
} KVEngine;

KVE_API KVEngine* new_kv_engine(const char *filename);

#endif
