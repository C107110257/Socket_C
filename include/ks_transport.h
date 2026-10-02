#ifndef KS_TRANSPORT_H
#define KS_TRANSPORT_H

#include "ks_common.h"

/*
 * 傳輸層抽象介面：TCP / UDP 各自實作一組。
 * 回傳慣例：
 *   open_* / wait_peer : KS_OK(0) 成功，KS_ERR(-1) 失敗
 *   send / recv        : 同 POSIX，>0 位元組數，0 對方關閉(TCP)，<0 錯誤
 */
typedef struct {
    int     (*open_client)(ks_endpoint_t *ep, const ks_config_t *cfg);
    int     (*open_server)(ks_endpoint_t *ep, const ks_config_t *cfg);
    int     (*wait_peer)  (ks_endpoint_t *ep);   /* TCP: accept；UDP: 收第一個封包記下 peer */
    ssize_t (*send)       (ks_endpoint_t *ep, const void *buf, size_t len);
    ssize_t (*recv)       (ks_endpoint_t *ep, void *buf, size_t len);
    void    (*shutdown)   (ks_endpoint_t *ep);   /* 喚醒阻塞中的 recv，用於結束收發 */
    void    (*close)      (ks_endpoint_t *ep);
} ks_transport_ops_t;

/* 依協定取得對應的 ops，不支援時回傳 NULL */
const ks_transport_ops_t *ks_get_transport(ks_proto_t proto);

/* 各協定的 ops 實體（定義在 ks_tcp.c / ks_udp.c，ks_transport.c 會用到） */
extern const ks_transport_ops_t ks_tcp_ops;
extern const ks_transport_ops_t ks_udp_ops;

#endif /* KS_TRANSPORT_H */