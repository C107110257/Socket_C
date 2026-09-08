#include "ken_socket.h"

void *recv_msg(void *arg)
{
    // 將執行緒參數轉型成 ConnInfo 結構
    ConnInfo *info = (ConnInfo *)arg;

    // 用來接收資料的緩衝區
    char buffer[1024];

    // 持續接收資料直到連線中斷
    while (1)
    {
        // 清空 buffer，避免殘留舊資料
        memset(buffer, 0, sizeof(buffer));

        // 從 socket 接收資料
        // n 為實際收到的位元組數
        int n = recv(info->sock,
                     buffer,
                     sizeof(buffer) - 1,
                     0);

        // n <= 0 代表：
        // n == 0 : 對方正常關閉連線
        // n < 0  : 接收發生錯誤
        if (n <= 0)
        {
            printf("\n%s disconnected.\n",
                   info->name);
            break;
        }

        // 在接收到的資料最後補上字串結束符號
        // 讓 buffer 可以當作字串使用
        buffer[n] = '\0';

        // 顯示收到的訊息
        // info->name 用來標示訊息來源
        printf("\n%s: %s\n> ",
               info->name,
               buffer);

        // 立即刷新輸出緩衝區
        fflush(stdout);
    }

    // 結束執行緒
    return NULL;
}

void *send_msg(void *arg)
{
    // 取得 socket 描述符
    int sock = *(int *)arg;

    // 輸入訊息暫存區
    char buffer[1024];

    // 持續讀取使用者輸入
    while (1)
    {
        // 顯示輸入提示符號
        printf("> ");
        fflush(stdout);

        // 從鍵盤讀取一行資料
        // stdin 代表標準輸入（鍵盤）
        // fgets 失敗或收到 EOF 時離開
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            break;

        // 移除 fgets 讀進來的換行字元 '\n'
        // 例如：
        // "hello\n" -> "hello"
        buffer[strcspn(buffer, "\n")] = 0;

        // 將訊息透過 socket 傳送給對方
        send(sock, buffer, strlen(buffer), 0);

        // 若輸入 quit 則關閉連線
        if (strcmp(buffer, "quit") == 0)
        {
            // 關閉 socket 的收發功能
            // 通知對方連線結束
            shutdown(sock, SHUT_RDWR);

            // 離開傳送執行緒
            break;
        }
    }

    // 結束執行緒
    return NULL;
}

void run_client(char *server_ip)
{
    // Socket描述符，用來代表一個TCP連線
    int sock;

    // 儲存Server的IP與Port資訊
    struct sockaddr_in server_addr;

    // 建立傳送與接收執行緒ID
    pthread_t tx_thread;
    pthread_t rx_thread;

    // 建立 TCP Socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // 檢查 Socket 是否建立成功
    if (sock < 0)
    {
        perror("socket");
        return;
    }

    // 設定 IPv4
    server_addr.sin_family = AF_INET;

    // 設定連接的 Port
    server_addr.sin_port = htons(PORT);

    // 將字串格式的IP轉成網路位址格式
    if (inet_pton(AF_INET,
                  server_ip,
                  &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock);
        return;
    }

    // 主動連線到指定Server
    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        printf("Failed to connect to %s\n", server_ip);
        perror("connect");

        close(sock);
        return;
    }

    // 顯示連線成功訊息
    printf("Connected to %s , Input 'quit' to close socket.\n",
           server_ip);

    // 配置 ConnInfo 記憶體供接收執行緒使用
    ConnInfo *info = malloc(sizeof(*info));
    // 檢查 malloc 是否成功
    if (info == NULL)
    {
        perror("malloc");
        return;
    }
    // 接收執行緒顯示的訊息來源名稱
    strcpy(info->name, "From Server");
    // 將 Socket 存入 ConnInfo 結構
    info->sock = sock;

    // 建立接收執行緒
    // 持續接收Server傳來的訊息
    if (pthread_create(&rx_thread,
                       NULL,
                       recv_msg,
                       info) != 0)
    {
        perror("pthread_create");
        close(sock);
        free(info);
        return;
    }

    // 建立傳送執行緒
    // 持續讀取鍵盤輸入並送出訊息
    if (pthread_create(&tx_thread,
                       NULL,
                       send_msg,
                       &sock) != 0)
    {
        perror("pthread_create");

        pthread_cancel(rx_thread);
        pthread_join(rx_thread, NULL);

        close(sock);
        free(info);
        return;
    }

    // 等待傳送執行緒結束
    pthread_join(tx_thread, NULL);

    // 等待接收執行緒結束
    pthread_join(rx_thread, NULL);

    free(info);
    close(sock);
}

void run_server()
{
    // Server Socket 描述符
    int server_fd;

    // Server 位址資訊
    struct sockaddr_in addr;

    // 建立 TCP Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 檢查 Socket 是否建立成功
    if (server_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    // Socket 選項設定值
    int opt = 1;

    // 設定 SO_REUSEADDR
    // 避免 Server 重啟時出現
    // "Address already in use"
    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(server_fd);
        exit(1);
    }

    // 使用 IPv4
    addr.sin_family = AF_INET;

    // 指定監聽 Port
    addr.sin_port = htons(PORT);

    // 監聽所有可用網路介面
    // 等同於 0.0.0.0
    addr.sin_addr.s_addr = INADDR_ANY;

    // 將 Socket 綁定到指定 IP 與 Port
    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    // 開始監聽 Client 連線
    // backlog = 5
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    printf("Listening on port %d...\n", PORT);

    // 持續等待 Client 連線
    while (1)
    {
        // Client Socket 描述符
        int client_fd;

        // Client 位址資訊
        struct sockaddr_in client_addr;

        // 位址結構大小
        socklen_t len = sizeof(client_addr);

        // 收發執行緒 ID
        pthread_t tx_thread;
        pthread_t rx_thread;

        // 等待 Client 連線
        // accept() 會阻塞直到有 Client 連入
        client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_addr,
            &len);

        // 檢查 accept 是否成功
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        // 配置 ConnInfo 記憶體
        // 提供 recv_msg() 使用
        ConnInfo *info = malloc(sizeof(*info));

        // 檢查 malloc 是否成功
        if (info == NULL)
        {
            perror("malloc");
            close(client_fd);
            continue;
        }

        // 顯示訊息來源名稱
        strcpy(info->name, "From Client");

        // 儲存 Client Socket
        info->sock = client_fd;

        // 顯示連線進來的 Client IP 與 Port
        printf("Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // 建立接收執行緒
        // 負責接收 Client 訊息
        if (pthread_create(&rx_thread,
                           NULL,
                           recv_msg,
                           info) != 0)
        {
            perror("pthread_create");
            free(info);
            close(client_fd);
            continue;
        }

        // 建立傳送執行緒
        // 負責讀取鍵盤輸入並送給 Client
        if (pthread_create(&tx_thread,
                           NULL,
                           send_msg,
                           &client_fd) != 0)
        {
            perror("pthread_create");

            // 取消接收執行緒
            pthread_cancel(rx_thread);

            // 等待接收執行緒結束
            pthread_join(rx_thread, NULL);

            free(info);
            close(client_fd);
            continue;
        }

        // 等待傳送執行緒結束
        pthread_join(tx_thread, NULL);

        // 等待接收執行緒結束
        pthread_join(rx_thread, NULL);

        close(client_fd);
        free(info);
    }
}