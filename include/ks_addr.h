#ifndef KS_ADDR_H
#define KS_ADDR_H

#include "ks_common.h"

/*
 * 判斷字串是 IPv4 還是 IPv6 的數字位址。
 * 回傳 AF_INET / AF_INET6；不是合法數字 IP 則回傳 KS_ERR。
 */
int ks_addr_detect_family(const char *ip);

/*
 * 解析 ip + port 成可用的位址（內部用 getaddrinfo + AI_NUMERICHOST）。
 *   ip      : NULL 或 "" 且 passive=1 時代表「監聽所有介面」
 *   passive : 1 = server bind 用；0 = client connect/sendto 用
 * 成功回傳 KS_OK 並填好 out，失敗回傳 KS_ERR。
 */
int ks_addr_resolve(const char *ip, const char *port,
                    ks_proto_t proto, int passive, ks_addr_t *out);

/*
 * 把位址轉成可顯示字串，例如 "192.168.0.10:8787" 或 "[fe80::1]:8787"。
 * 回傳 buf 方便直接放進 printf。
 */
const char *ks_addr_to_str(const ks_addr_t *addr, char *buf, size_t buflen);

#endif /* KS_ADDR_H */