#include "kve.h"

int main() {
    KVEngine *db = new_kv_engine("pure_macro_free.db");
    if (!db) return 1;

    printf("--- [안정화 완성형] 매크로 배제 C 클래스 엔진 기동 ---\n");

    db->set(db, 0, ARG_KEY, "macro_status", ARG_VALUE, "None_Zero_Macro_Defined", ARG_END);
    db->set(db, 0, ARG_VALUE, "C_Dynamic_Typedef_Struct", ARG_KEY, "type_system", ARG_END);

    db->branch(db, 0, ARG_BRANCH_NAME, "debug", ARG_END);
    db->set(db, 0, ARG_VALUE, "Debug_Branch_Runtime_Active", ARG_KEY, "macro_status", ARG_END);

    char *res_val = NULL; uint32_t res_sz = 0;
    db->get(db, 0, ARG_OUT_VAL_SZ, &res_sz, ARG_OUT_VALUE, &res_val, ARG_KEY, "macro_status", ARG_END);
    printf(">> 현재 [debug] 브랜치 데이터: %s (크기: %u)\n", res_val, res_sz);
    free(res_val);

    db->branch(db, 0, ARG_BRANCH_NAME, "main", ARG_END);
    db->get(db, 0, ARG_KEY, "macro_status", ARG_OUT_VALUE, &res_val, ARG_OUT_VAL_SZ, &res_sz, ARG_END);
    printf(">> 복귀한 [main] 브랜치 데이터: %s\n", res_val);
    free(res_val);

    db->get(db, 0, ARG_TARGET_COMMIT_ID, 2, ARG_KEY, "type_system", ARG_OUT_VALUE, &res_val, ARG_OUT_VAL_SZ, &res_sz, ARG_END);
    printf(">> [타임머신 롤백 복구] 시점 #2 데이터: %s\n", res_val);
    free(res_val);

    db->compact(db, 0, ARG_END);
    printf("\n>> 매크로 없는 디스크 가변 압축 완료.\n");

    db->close(db);
    return 0;
}