#ifndef KS_CHAT_H
#define KS_CHAT_H

#include "ks_common.h"
#include "ks_transport.h"

#define KS_QUIT_CMD  "quit"   /* 應用層結束指令（UDP 沒有斷線通知，雙方靠它收尾） */

/*
 * 啟動收/送執行緒並等待結束。與 TCP/UDP、IPv4/IPv6 無關，
 * 只透過 ops 的 send / recv 收發資料。
 * 回傳 KS_OK 或 KS_ERR。
 */
int ks_chat_run(const ks_transport_ops_t *ops, ks_endpoint_t *ep);

#endif /* KS_CHAT_H */