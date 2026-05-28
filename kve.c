#include "kve.h"

static void sync_class_metadata(KVEngine *this_ptr) {
    if (!this_ptr || !this_ptr->db_file) return;
    fseek(this_ptr->db_file, 0, SEEK_SET);
    
    uint32_t sig = 0x4B56454D;
    fwrite(&sig, sizeof(sig), 1, this_ptr->db_file);
    fwrite(&this_ptr->branch_count, sizeof(this_ptr->branch_count), 1, this_ptr->db_file);
    fwrite(&this_ptr->branch_capacity, sizeof(this_ptr->branch_capacity), 1, this_ptr->db_file);
    fwrite(&this_ptr->global_commit_counter, sizeof(this_ptr->global_commit_counter), 1, this_ptr->db_file);
    
    uint16_t active_len = (uint16_t)strlen(this_ptr->active_branch);
    fwrite(&active_len, sizeof(active_len), 1, this_ptr->db_file);
    fwrite(this_ptr->active_branch, 1, active_len, this_ptr->db_file);

    for (int i = 0; i < this_ptr->branch_count; i++) {
        uint16_t b_name_len = (uint16_t)strlen(this_ptr->branch_table[i].name);
        fwrite(&b_name_len, sizeof(b_name_len), 1, this_ptr->db_file);
        fwrite(this_ptr->branch_table[i].name, 1, b_name_len, this_ptr->db_file);
        fwrite(&this_ptr->branch_table[i].latest_commit_id, sizeof(uint32_t), 1, this_ptr->db_file);
    }
    
    fflush(this_ptr->db_file);
}

static void load_class_metadata(KVEngine *this_ptr) {
    if (!this_ptr || !this_ptr->db_file) return;
    fseek(this_ptr->db_file, 0, SEEK_SET);
    uint32_t sig = 0;
    
    if (fread(&sig, sizeof(sig), 1, this_ptr->db_file) != 1 || sig != 0x4B56454D) {
        this_ptr->branch_count = 1;
        this_ptr->branch_capacity = 4;
        this_ptr->branch_table = (BranchMeta *)malloc(sizeof(BranchMeta) * this_ptr->branch_capacity);
        this_ptr->branch_table[0].name = (char *)malloc(strlen("main") + 1);
        strcpy(this_ptr->branch_table[0].name, "main");
        this_ptr->branch_table[0].latest_commit_id = 0;
        
        this_ptr->active_branch = (char *)malloc(strlen("main") + 1);
        strcpy(this_ptr->active_branch, "main");
        this_ptr->global_commit_counter = 1;
        
        sync_class_metadata(this_ptr);
        return;
    }
    
    fread(&this_ptr->branch_count, sizeof(this_ptr->branch_count), 1, this_ptr->db_file);
    fread(&this_ptr->branch_capacity, sizeof(this_ptr->branch_capacity), 1, this_ptr->db_file);
    fread(&this_ptr->global_commit_counter, sizeof(this_ptr->global_commit_counter), 1, this_ptr->db_file);
    
    uint16_t active_len = 0;
    fread(&active_len, sizeof(active_len), 1, this_ptr->db_file);
    this_ptr->active_branch = (char *)malloc(active_len + 1);
    fread(this_ptr->active_branch, 1, active_len, this_ptr->db_file);
    this_ptr->active_branch[active_len] = '\0';

    this_ptr->branch_table = (BranchMeta *)malloc(sizeof(BranchMeta) * this_ptr->branch_capacity);
    for (int i = 0; i < this_ptr->branch_count; i++) {
        uint16_t b_name_len = 0;
        fread(&b_name_len, sizeof(b_name_len), 1, this_ptr->db_file);
        this_ptr->branch_table[i].name = (char *)malloc(b_name_len + 1);
        fread(this_ptr->branch_table[i].name, 1, b_name_len, this_ptr->db_file);
        this_ptr->branch_table[i].name[b_name_len] = '\0';
        fread(&this_ptr->branch_table[i].latest_commit_id, sizeof(uint32_t), 1, this_ptr->db_file);
    }
}

static int kv_raw_read_internal(KVEngine *this_ptr, const char *target_key, char **out_value, uint32_t *out_val_sz, uint32_t target_commit_id) {
    if (!this_ptr || !this_ptr->db_file || !target_key || !out_value || !out_val_sz) return 0;

    if (target_commit_id == 0) {
        for (int i = 0; i < this_ptr->branch_count; i++) {
            if (strcmp(this_ptr->branch_table[i].name, this_ptr->active_branch) == 0) {
                target_commit_id = this_ptr->branch_table[i].latest_commit_id;
                break;
            }
        }
    }

    long data_start_offset = sizeof(uint32_t) + (sizeof(int) * 2) + sizeof(uint32_t) + sizeof(uint16_t) + strlen(this_ptr->active_branch);
    for(int i=0; i<this_ptr->branch_count; i++) {
        data_start_offset += sizeof(uint16_t) + strlen(this_ptr->branch_table[i].name) + sizeof(uint32_t);
    }
    fseek(this_ptr->db_file, data_start_offset, SEEK_SET);

    uint16_t key_sz; uint32_t val_sz; uint32_t block_commit_id; int found = 0;
    while (fread(&key_sz, sizeof(key_sz), 1, this_ptr->db_file) == 1) {
        fread(&val_sz, sizeof(val_sz), 1, this_ptr->db_file);
        fread(&block_commit_id, sizeof(block_commit_id), 1, this_ptr->db_file);

        char *current_key = (char *)malloc(key_sz + 1);
        fread(current_key, 1, key_sz, this_ptr->db_file);
        current_key[key_sz] = '\0';

        if (strcmp(current_key, target_key) == 0 && block_commit_id <= target_commit_id) {
            if (found && *out_value) free(*out_value);
            *out_value = (char *)malloc(val_sz + 1);
            fread(*out_value, 1, val_sz, this_ptr->db_file);
            (*out_value)[val_sz] = '\0';
            *out_val_sz = val_sz;
            found = 1;
        } else {
            fseek(this_ptr->db_file, val_sz, SEEK_CUR);
        }
        free(current_key);
    }
    return found;
}

static void kve_method_set(KVEngine *this_ptr, int dummy, ...) {
    if (!this_ptr || !this_ptr->db_file) return;
    va_list args; va_start(args, dummy);
    const char *key = NULL; const char *value = NULL;
    ArgTag tag;
    while ((tag = va_arg(args, ArgTag)) != ARG_END) {
        if (tag == ARG_KEY) key = va_arg(args, const char*);
        else if (tag == ARG_VALUE) value = va_arg(args, const char*);
    }
    va_end(args);
    if (!key || !value) return;

    uint16_t key_sz = (uint16_t)strlen(key);
    uint32_t val_sz = (uint32_t)strlen(value);
    uint32_t current_commit = this_ptr->global_commit_counter++;

    fseek(this_ptr->db_file, 0, SEEK_END);
    fwrite(&key_sz, sizeof(key_sz), 1, this_ptr->db_file);
    fwrite(&val_sz, sizeof(val_sz), 1, this_ptr->db_file);
    fwrite(&current_commit, sizeof(current_commit), 1, this_ptr->db_file);
    fwrite(key, 1, key_sz, this_ptr->db_file);
    fwrite(value, 1, val_sz, this_ptr->db_file);

    for (int i = 0; i < this_ptr->branch_count; i++) {
        if (strcmp(this_ptr->branch_table[i].name, this_ptr->active_branch) == 0) {
            this_ptr->branch_table[i].latest_commit_id = current_commit;
            break;
        }
    }
    sync_class_metadata(this_ptr);
}

static int kve_method_get(KVEngine *this_ptr, int dummy, ...) {
    if (!this_ptr || !this_ptr->db_file) return 0;
    va_list args; va_start(args, dummy);
    const char *key = NULL; char **out_value = NULL; uint32_t *out_val_sz = NULL; uint32_t target_commit = 0;
    ArgTag tag;
    while ((tag = va_arg(args, ArgTag)) != ARG_END) {
        switch (tag) {
            case ARG_KEY:              key = va_arg(args, const char*); break;
            case ARG_OUT_VALUE:        out_value = va_arg(args, char**); break;
            case ARG_OUT_VAL_SZ:       out_val_sz = va_arg(args, uint32_t*); break;
            case ARG_TARGET_COMMIT_ID: target_commit = va_arg(args, uint32_t); break;
            default: break;
        }
    }
    va_end(args);
    return kv_raw_read_internal(this_ptr, key, out_value, out_val_sz, target_commit);
}

static void kve_method_branch(KVEngine *this_ptr, int dummy, ...) {
    if (!this_ptr || !this_ptr->db_file) return;
    va_list args; va_start(args, dummy);
    const char *b_name = NULL;
    ArgTag tag;
    while ((tag = va_arg(args, ArgTag)) != ARG_END) {
        if (tag == ARG_BRANCH_NAME) b_name = va_arg(args, const char*);
    }
    va_end(args);
    if (!b_name) return;

    for (int i = 0; i < this_ptr->branch_count; i++) {
        if (strcmp(this_ptr->branch_table[i].name, b_name) == 0) {
            free(this_ptr->active_branch);
            this_ptr->active_branch = (char *)malloc(strlen(b_name) + 1);
            strcpy(this_ptr->active_branch, b_name);
            sync_class_metadata(this_ptr);
            return;
        }
    }

    if (this_ptr->branch_count >= this_ptr->branch_capacity) {
        this_ptr->branch_capacity *= 2;
        this_ptr->branch_table = (BranchMeta *)realloc(this_ptr->branch_table, sizeof(BranchMeta) * this_ptr->branch_capacity);
    }

    uint32_t parent_commit = 0;
    for (int i = 0; i < this_ptr->branch_count; i++) {
        if (strcmp(this_ptr->branch_table[i].name, this_ptr->active_branch) == 0) {
            parent_commit = this_ptr->branch_table[i].latest_commit_id;
            break;
        }
    }

    this_ptr->branch_table[this_ptr->branch_count].name = (char *)malloc(strlen(b_name) + 1);
    strcpy(this_ptr->branch_table[this_ptr->branch_count].name, b_name);
    this_ptr->branch_table[this_ptr->branch_count].latest_commit_id = parent_commit;
    this_ptr->branch_count++;

    free(this_ptr->active_branch);
    this_ptr->active_branch = (char *)malloc(strlen(b_name) + 1);
    strcpy(this_ptr->active_branch, b_name);
    
    sync_class_metadata(this_ptr);
}

static void kve_method_compact(KVEngine *this_ptr, int dummy, ...) {
    if (!this_ptr || !this_ptr->db_file) return;

    FILE *temp_fp = fopen("pure_macro_free.tmp", "wb+");
    if (!temp_fp) return;
    
    uint32_t placeholder = 0;
    fwrite(&placeholder, 4, 1, temp_fp);

    long data_start_offset = sizeof(uint32_t) + (sizeof(int) * 2) + sizeof(uint32_t) + sizeof(uint16_t) + strlen(this_ptr->active_branch);
    for(int i=0; i<this_ptr->branch_count; i++) {
        data_start_offset += sizeof(uint16_t) + strlen(this_ptr->branch_table[i].name) + sizeof(uint32_t);
    }
    fseek(this_ptr->db_file, data_start_offset, SEEK_SET);
    
    uint16_t key_sz; uint32_t val_sz; uint32_t b_id;
    while (fread(&key_sz, sizeof(key_sz), 1, this_ptr->db_file) == 1) {
        fread(&val_sz, sizeof(val_sz), 1, this_ptr->db_file);
        fread(&b_id, sizeof(b_id), 1, this_ptr->db_file);
        char *k = (char *)malloc(key_sz + 1);
        fread(k, 1, key_sz, this_ptr->db_file); k[key_sz] = '\0';
        
        char *v = NULL; uint32_t v_sz = 0;
        if (kv_raw_read_internal(this_ptr, k, &v, &v_sz, 0)) {
            uint32_t new_c = this_ptr->global_commit_counter++;
            fwrite(&key_sz, sizeof(key_sz), 1, temp_fp);
            fwrite(&v_sz, sizeof(v_sz), 1, temp_fp);
            fwrite(&new_c, sizeof(new_c), 1, temp_fp);
            fwrite(k, 1, key_sz, temp_fp);
            fwrite(v, 1, v_sz, temp_fp);
            free(v);
        }
        fseek(this_ptr->db_file, val_sz, SEEK_CUR);
        free(k);
    }
    fclose(this_ptr->db_file);
    fclose(temp_fp);
    remove("pure_macro_free.db");
    rename("pure_macro_free.tmp", "pure_macro_free.db");
    this_ptr->db_file = fopen("pure_macro_free.db", "rb+");
    sync_class_metadata(this_ptr);
}

static void kve_method_close(KVEngine *this_ptr) {
    if (!this_ptr) return;
    if (this_ptr->db_file) fclose(this_ptr->db_file);
    if (this_ptr->active_branch) free(this_ptr->active_branch);
    if (this_ptr->branch_table) {
        for (int i = 0; i < this_ptr->branch_count; i++) {
            if (this_ptr->branch_table[i].name) free(this_ptr->branch_table[i].name);
        }
        free(this_ptr->branch_table);
    }
    free(this_ptr);
}

KVEngine* new_kv_engine(const char *filename) {
    KVEngine *engine = (KVEngine *)malloc(sizeof(KVEngine));
    if (!engine) return NULL;

    engine->db_file = fopen(filename, "rb+");
    if (!engine->db_file) {
        engine->db_file = fopen(filename, "wb+");
        if (!engine->db_file) {
            free(engine);
            return NULL;
        }
    }

    load_class_metadata(engine);

    engine->set = kve_method_set;
    engine->get = kve_method_get;
    engine->branch = kve_method_branch;
    engine->compact = kve_method_compact;
    engine->close = kve_method_close;

    return engine;
}