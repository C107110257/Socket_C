#include "ken_socket.h"

void print_banner(void)
{
    printf("=====================================\n");
    printf(" %s\n", APP_NAME);
    printf(" Version : %s\n", VERSION);
    printf(" Author : %s\n", AUTHOR);
    printf(" Build : %s %s\n", BUILD_DATE, BUILD_TIME);
    printf("=====================================\n\n");
}

int main(int argc, char *argv[])
{
    print_banner();
    
    if (argc < 2 || strcmp(argv[1], "-h") == 0)
    {
        printf("Usage:\n");
        printf("  %s -s\n", argv[0]);
        printf("  %s -c <server_ip>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-s") == 0)
    {
        run_server();
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        if (argc != 3)
        {
            printf("Usage: %s -c <server_ip>\n", argv[0]);
            return 1;
        }

        run_client(argv[2]);
    }
    else
    {
        printf("Unknown option\n");
        printf("Usage:\n");
        printf("  %s -s\n", argv[0]);
        printf("  %s -c <server_ip>\n", argv[0]);
        return 1;
    }

    return 0;
}