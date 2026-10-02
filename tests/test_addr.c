#include <stdio.h>
#include "../include/ks_addr.h"

int main(int argc, char *argv[])
{
    char str[96];

    for (int i = 1; i < argc; i++) {
        int fam = ks_addr_detect_family(argv[i]);
        ks_addr_t a;

        printf("%-22s family=%s", argv[i],
               fam == AF_INET ? "IPv4" : fam == AF_INET6 ? "IPv6" : "invalid");

        if (fam != -1 && ks_addr_resolve(argv[i], "8787", KS_PROTO_TCP, 0, &a) == KS_OK)
            printf("  -> %s", ks_addr_to_str(&a, str, sizeof(str)));
        printf("\n");
    }

    ks_addr_t s;
    if (ks_addr_resolve(NULL, "8787", KS_PROTO_TCP, 1, &s) == KS_OK)
        printf("server default bind    -> %s (family=%s)\n",
               ks_addr_to_str(&s, str, sizeof(str)),
               s.family == AF_INET6 ? "IPv6" : "IPv4");
    return 0;
}