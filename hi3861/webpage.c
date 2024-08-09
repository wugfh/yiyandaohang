#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifi_hotspot.h"
#include "lwip/netifapi.h"
#include "lwip/sockets.h"
#include "stm32.h"
#include "cJSON.h"
 
#define ATTR_STACK_SIZE 10240
#define ZERO 0
#define ONE 1
#define TWO 2
#define THREE 3
#define FOUR 4
#define FIVE 5
#define SEVEN 7
#define TEN 10
#define ONE_HUNDRED 100

extern char webinstru[256];
 
static volatile int g_hotspotStarted = 0;        
static const char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nCache-Control: no-cache\r\n\r\n";
 
static char* g_targetssid[30];
static char* g_targetpassword[30]; 

extern char myip[20];
char page_addr[50];
 
#define HTML_FORMAT                  "<!DOCTYPE html>\n" \
                                      "<html lang=\"zh-CN\">\n"\
                                      "<head>\n"\
                                      "<meta charset=\"UTF-8\">\n"\
                                      "</head>\n"\
                                      "<body>\n"\
                                      "<h1>当前位置：%s</h1>\n"\
                                      "<h1>目标位置：%s</h1>\n"\
                                      "<h1>路径指示：%s</h1>\n"\
                                      "</body>\n"\
                                      "</html>"
#define NAV_PAGE_HTML \
"<html lang=\"zh\">\n" \
"<head>\n" \
"    <meta charset=\"UTF-8\">\n" \
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"    <title>导航页面</title>\n" \
"    <style>\n" \
"        body {\n" \
"            font-family: Arial, sans-serif;\n" \
"            margin: 20px;\n" \
"        }\n" \
"        .container {\n" \
"            max-width: 600px;\n" \
"            margin: auto;\n" \
"        }\n" \
"        .input-group {\n" \
"            margin-bottom: 15px;\n" \
"        }\n" \
"        label {\n" \
"            display: block;\n" \
"            margin-bottom: 5px;\n" \
"        }\n" \
"        input[type=\"text\"] {\n" \
"            width: 100%%;\n" \
"            padding: 8px;\n" \
"            box-sizing: border-box;\n" \
"        }\n" \
"        .output-group {\n" \
"            margin-top: 20px;\n" \
"        }\n" \
"        .output-group div {\n" \
"            margin-bottom: 10px;\n" \
"        }\n" \
"    </style>\n" \
"</head>\n" \
"<body>\n" \
"    <div class=\"container\">\n" \
"        <h1>导航页面</h1>\n" \
"        <div class=\"input-group\">\n" \
"            <label for=\"target-lat\">目标纬度：</label>\n" \
"            <input type=\"text\" id=\"target-lat\" placeholder=\"输入目标纬度\">\n" \
"        </div>\n" \
"        <div class=\"input-group\">\n" \
"            <label for=\"target-lon\">目标经度：</label>\n" \
"            <input type=\"text\" id=\"target-lon\" placeholder=\"输入目标经度\">\n" \
"        </div>\n" \
"        <div class=\"input-group\">\n" \
"            <label for=\"target-lon\">位置序号：</label>\n" \
"            <input type=\"text\" id=\"target-index\" placeholder=\"输入位置序号\">\n" \
"        </div>\n" \
"        <button onclick=\"sendCoordinates()\">发送经纬度</button>\n" \
"        <div class=\"output-group\">\n" \
"            <div>本地经纬度：<span id=\"local-coords\">%s</span></div>\n" \
"            <div>目标经纬度：<span id=\"target-coords\">%s</span></div>\n" \
"            <div>导航指令：<span id=\"navigation-instructions\">%s</span></div>\n" \
"        </div>\n" \
"    </div>\n" \
"    <script>\n" \
"        function sendCoordinates() {\n" \
"            const targetLat = document.getElementById('target-lat').value;\n" \
"            const targetLon = document.getElementById('target-lon').value;\n" \
"            const targetIndex = document.getElementById('target-index').value;\n" \
"const data = { lat: targetLat, lon: targetLon, index: targetIndex};" \
"fetch('%s', {" \
"method: 'POST'," \
"headers: { 'Content-Type': 'application/json' }," \
"body: JSON.stringify(data)" \
"})" \
".then(response => response.json())" \
".then(data => { console.log('Success:', data); })" \
".catch((error) => { console.error('Error:', error); });" \
"  }\n" \
"    </script>\n" \
"</body>\n" \
"</html>"

 
char http_index_html[4096];
 
 
static int DisposeNetMessageType(char* buf)
{
    int buflen = strlen(buf);
    /* 判断是不是HTTP的GET命令*/
    if (    buf[0]=='G' &&
            buf[1]=='E' &&
            buf[2]=='T' &&
            buf[3]==' ' &&
            buf[4]=='/' )
    {
        return 0;
    }
    return 1;
}

char json_buf[256];
char page2stm_buf[100];
void handle_post(char* recv_buf, int len){
    char* json_start = NULL;
    char* json_end = NULL;
    json_start = strchr(recv_buf, '{');
    json_end = strchr(json_start+1, '}');
    if(json_end-json_start > sizeof(json_buf)-10){
        printf("json buffer overflow\n");
        return;
    }
    memcpy(json_buf, json_start, json_end-json_start+1);
    json_buf[json_end-json_start+2] = '\0';
    cJSON* root = cJSON_Parse(json_buf);
    cJSON* lal_obj = cJSON_GetObjectItem(root, "lat");
    cJSON* lng_obj = cJSON_GetObjectItem(root, "lon");
    cJSON* index_obj = cJSON_GetObjectItem(root, "index");
    char* lal = cJSON_GetStringValue(lal_obj);
    char* lng = cJSON_GetStringValue(lng_obj);
    char* index = cJSON_GetStringValue(index_obj);
    printf("%s:%s:%s\r\n", lal, lng, index);
    int stm_len = snprintf(page2stm_buf, 100, "page:%s:%s:%s", lal, lng, index);
    sendtostm32(page2stm_buf, stm_len);
    set_target_pos(lal, lng);

}

char request[10000];
void webpage_accept(){
    //建立tcp连接
    ssize_t retval = 0;
    int backlog = 1;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    int connfd = -1;
    
 
    struct sockaddr_in clientAddr = {0};
    socklen_t clientAddrLen = sizeof(clientAddr);
    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);  // 端口号，从主机字节序转为网络字节序
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 允许任意主机接入， 0.0.0.0
 
    retval = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); // 绑定端口    
    if (retval < 0) {
        printf("bind failed, %ld!\r\n", retval);
        close(sockfd);
        return;
    }
    printf("bind to port %u success!\r\n", 80);
 
    retval = listen(sockfd, backlog); // 开始监听
    if (retval < 0) {
        printf("listen failed!\r\n");
        printf("do_cleanup...\r\n");
        close(sockfd);
        return;
    }
    printf("listen with %d backlog success!\r\n", backlog);
 
    // 接受客户端连接，成功会返回一个表示连接的 socket ， clientAddr 参数将会携带客户端主机和端口信息 ；失败返回 -1
    // 此后的 收、发 都在 表示连接的 socket 上进行；之后 sockfd 依然可以继续接受其他客户端的连接，
    //  UNIX系统上经典的并发模型是“每个连接一个进程”——创建子进程处理连接，父进程继续接受其他客户端的连接
    //  鸿蒙liteos-a内核之上，可以使用UNIX的“每个连接一个进程”的并发模型
    //     liteos-m内核之上，可以使用“每个连接一个线程”的并发模型
 
    while (1) {
        static int i=1;
 
        connfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (connfd < 0) {
            printf("accept failed, %d, %d\r\n", connfd, errno);
            printf("do_cleanup...\r\n");
            close(sockfd);
            return;
        }
       
        // 后续 收、发 都在 表示连接的 socket 上进行；
        int recv_len;
        recv_len = recv(connfd, request, sizeof(request), 0 );
        if (recv_len < 0) {
            printf("recv request failed, %ld!\r\n", retval);
            close(sockfd);
            return;
        }

        int messagetype = DisposeNetMessageType(request);
        if(messagetype == 0)//get请求
        {
                retval = send(connfd, http_html_hdr, strlen(http_html_hdr), 0);
                if (retval <= 0) {
                    printf("send response header failed, %ld!\r\n", retval);
                    close(connfd);
                    close(sockfd);
                    return;
                }
                printf("recv buffer:%s\r\n", request);
                snprintf(page_addr, 50, "http://%s:%d/endpoint", myip,80);
                int len = snprintf(http_index_html, sizeof(http_index_html), NAV_PAGE_HTML, get_start_pos(), get_target_pos(), webinstru, page_addr);
                retval = send(connfd, http_index_html, len, 0);
                if (retval <= 0) { 
                    printf("send response body failed, %ld!\r\n", retval);
                    close(connfd);
                    close(sockfd);
                    return;
                }
        }
        else {
            printf("recv len:%d\r\n", recv_len);
            handle_post(request, recv_len);
            retval = send(connfd, http_html_hdr, strlen(http_html_hdr), 0);
            if (retval <= 0) {
                printf("send response header failed, %ld!\r\n", retval);
                close(connfd);
                close(sockfd);
                return;
            }
            snprintf(page_addr, 50, "http://%s:%d/endpoint", myip,80);
            int len = snprintf(http_index_html, sizeof(http_index_html), NAV_PAGE_HTML, get_start_pos(), get_target_pos(), webinstru, page_addr);
            retval = send(connfd, http_index_html, len, 0);
            if (retval <= 0) { 
                printf("send response body failed, %ld!\r\n", retval);
                close(connfd);
                close(sockfd);
                return;
            }
        }
 
        /* 关闭 */
        close(connfd);

    }
}

static void WifiHotspotTask(int *arg)
{
    while(1){
        webpage_accept();
        sleep(2);
    }

}
 
void WifiHotspotDemo(void)
{
    osThreadAttr_t attr;
    // 初始化相关配置
    attr.name = "WifiHotspotTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = ATTR_STACK_SIZE;
    attr.priority = osPriorityNormal;
 
    if (osThreadNew(WifiHotspotTask, NULL, &attr) == NULL) {
        printf("[WifiHotspotDemo] Failed to create WifiHotspotTask!\n");
    }
}
