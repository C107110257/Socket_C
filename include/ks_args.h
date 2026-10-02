#ifndef KS_ARGS_H
#define KS_ARGS_H

#include "ks_common.h"

/*
 * 解析命令列，結果存進 cfg（會先套用預設值：TCP、KS_DEFAULT_PORT）。
 * 回傳：
 *   KS_OK : 解析成功，可繼續執行
 *   1     : 使用者要求說明（-h），已印出 usage，呼叫端直接結束即可
 *   KS_ERR: 參數錯誤，已印出錯誤與 usage
 */
int  ks_args_parse(int argc, char *argv[], ks_config_t *cfg);

void ks_args_usage(const char *prog);

#endif /* KS_ARGS_H */