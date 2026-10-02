#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp */

#include "../include/ks_args.h"
#include "../include/ks_addr.h"

void ks_args_usage(const char *prog)
{
    printf("Usage:\n");
    printf("  %s -s [-t tcp|udp] [-p port] [-b bind_ip]\n", prog);
    printf("  %s -c <server_ip> [-t tcp|udp] [-p port]\n", prog);
    printf("\nOptions:\n");
    printf("  -s, --server        Run as server\n");
    printf("  -c, --client <ip>   Run as client, connect to <ip> (IPv4 or IPv6)\n");
    printf("  -t, --proto <p>     tcp (default) or udp\n");
    printf("  -p, --port <port>   Port number (default %s)\n", KS_DEFAULT_PORT);
    printf("  -b, --bind <ip>     Server bind address (default: all interfaces)\n");
    printf("  -h, --help          Show this help\n");
}

static int parse_port(const char *s, char *dst, size_t dstlen)
{
    char *end = NULL;
    long v = strtol(s, &end, 10);

    if (s[0] == '\0' || *end != '\0' || v < 1 || v > 65535) {
        fprintf(stderr, "Invalid port: %s\n", s);
        return KS_ERR;
    }
    snprintf(dst, dstlen, "%ld", v);
    return KS_OK;
}

static int check_ip(const char *s)
{
    if (strlen(s) >= KS_IP_LEN || ks_addr_detect_family(s) == KS_ERR) {
        fprintf(stderr, "Invalid IP address: %s\n", s);
        return KS_ERR;
    }
    return KS_OK;
}

int ks_args_parse(int argc, char *argv[], ks_config_t *cfg)
{
    static const struct option long_opts[] = {
        {"server", no_argument,       0, 's'},
        {"client", required_argument, 0, 'c'},
        {"proto",  required_argument, 0, 't'},
        {"port",   required_argument, 0, 'p'},
        {"bind",   required_argument, 0, 'b'},
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    const char *client_ip = NULL;
    const char *bind_ip   = NULL;
    int have_s = 0;
    int opt;

    memset(cfg, 0, sizeof(*cfg));
    cfg->proto = KS_PROTO_TCP;
    snprintf(cfg->port, sizeof(cfg->port), "%s", KS_DEFAULT_PORT);

    optind = 1;
    while ((opt = getopt_long(argc, argv, "sc:t:p:b:h", long_opts, NULL)) != -1) {
        switch (opt) {
        case 's':
            have_s = 1;
            break;
        case 'c':
            client_ip = optarg;
            break;
        case 't':
            if (strcasecmp(optarg, "tcp") == 0)
                cfg->proto = KS_PROTO_TCP;
            else if (strcasecmp(optarg, "udp") == 0)
                cfg->proto = KS_PROTO_UDP;
            else {
                fprintf(stderr, "Unknown protocol: %s (use tcp or udp)\n", optarg);
                return KS_ERR;
            }
            break;
        case 'p':
            if (parse_port(optarg, cfg->port, sizeof(cfg->port)) != KS_OK)
                return KS_ERR;
            break;
        case 'b':
            bind_ip = optarg;
            break;
        case 'h':
            ks_args_usage(argv[0]);
            return 1;
        default:               /* getopt 已印出錯誤訊息 */
            ks_args_usage(argv[0]);
            return KS_ERR;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        ks_args_usage(argv[0]);
        return KS_ERR;
    }

    if (have_s == (client_ip != NULL)) {   /* 兩個都有或都沒有 */
        fprintf(stderr, "Specify exactly one of -s or -c <ip>\n");
        ks_args_usage(argv[0]);
        return KS_ERR;
    }

    if (have_s) {
        cfg->mode = KS_MODE_SERVER;
        if (bind_ip != NULL) {
            if (check_ip(bind_ip) != KS_OK)
                return KS_ERR;
            snprintf(cfg->ip, sizeof(cfg->ip), "%s", bind_ip);
        }
    } else {
        if (bind_ip != NULL) {
            fprintf(stderr, "-b is only valid in server mode\n");
            return KS_ERR;
        }
        if (check_ip(client_ip) != KS_OK)
            return KS_ERR;
        cfg->mode = KS_MODE_CLIENT;
        snprintf(cfg->ip, sizeof(cfg->ip), "%s", client_ip);
    }

    return KS_OK;
}