/*
 * Copyright (c) 2020, HiHope Community.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "httpd.h"
#include "net_params.h"
#include "wifi_connecter.h"
#include "hi_stdlib.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"  // For inet_addr()
#include "cJSON.h"  
#include "dtof.h"
#include "stm32.h" 

static int g_netId = -1;
#define STACK_SIZE  (4096)
#define HTTPC_DEMO_RECV_BUFSIZE 30000
#define SOCK_TARGET_PORT 80

// 替换为你的百度地图API Key
static const char *g_api_key = "NeY5Hx1biFkfoDn9BNhIMpzl50iLrmgn";
static char recv_buf[HTTPC_DEMO_RECV_BUFSIZE];
static char json_buf[2048];
static char instru[1024];
static char stm32buf[1024];

// 函数声明
unsigned int http_client_get(const char *origin, const char *destination, const char *server_ip);
struct hostent *lwip_gethostbyname(const char *name);

void parse_https_recv(char* https_recv, int len){
    cJSON* root = cJSON_Parse(https_recv);
    if(root == NULL){
        printf("解析JSON时发生错误\n");
    }
    cJSON* status = cJSON_GetObjectItem(root, "message");
    if(status == NULL){
        printf("json is not right\n");
        return;
    }
    if(strncmp(cJSON_GetStringValue(status), "ok", 2) != 0){
        printf("the message from baidu map has error\n");
        return;
    }
    cJSON* result = cJSON_GetObjectItem(root, "result");
    if(result == NULL){
        printf("the message does not have result\n");
        return;
    }
    cJSON* routes = cJSON_GetObjectItem(result, "routes");
    if (cJSON_IsArray(routes)) {
        int num_routes = cJSON_GetArraySize(routes);
        for (int i = 0; i < num_routes; i++) {
            cJSON *route = cJSON_GetArrayItem(routes, i);

            cJSON *distance = cJSON_GetObjectItem(route, "distance");
            if (distance != NULL) {
                printf("距离: %d 米\n", distance->valueint);
            }

            cJSON *duration = cJSON_GetObjectItem(route, "duration");
            if (duration != NULL) {
                printf("持续时间: %d 秒\n", duration->valueint);
            }

            cJSON *steps = cJSON_GetObjectItem(route, "steps");
            if (cJSON_IsArray(steps)) {
                int num_steps = cJSON_GetArraySize(steps);
                for (int j = 0; j < num_steps; j++) {
                    cJSON *step = cJSON_GetArrayItem(steps, j);
                    cJSON *dist = cJSON_GetObjectItem(step, "distance");
                    cJSON *dir = cJSON_GetObjectItem(step, "direction");
                    cJSON* dura = cJSON_GetObjectItem(step, "duration");
                    printf("dist: %d , dir:%d, dura:%d\n", dist->valueint, dir->valueint, dura->valueint);
                    int len = snprintf(stm32buf, 1024, "%d,%d,%d", dist->valueint, dir->valueint, dura->valueint);
                    sendtostm32(stm32buf, len);
                }
            }
        }
    }
    printf("complete to parse result\n");
    cJSON_Delete(root); // 清理cJSON对象
}

void HttpClientTask(void *arg) {
    (void)arg;

    char origin[64] = "30.5454,114.4234";
    char destination[64] = "30.5186,114.4234";
    const char *host_name = "api.map.baidu.com";  // 百度地图API服务器的主机名

    // 解析主机名获取IP地址
    struct hostent *host_entry = lwip_gethostbyname(host_name);
    while (host_entry == NULL) {
        printf("Failed to resolve hostname: %s\n", host_name);
        sleep(5);
        host_entry = lwip_gethostbyname(host_name);
    }

    // 获取解析到的第一个IP地址
    char* start_pos = get_start_pos();
    char *server_ip = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
    printf("Resolved IP address: %s\n", server_ip);
    while(1){
        if(start_pos == NULL)
            http_client_get(origin, destination, server_ip);
        else
            http_client_get(start_pos, destination, server_ip);
        sleep(5);
    }
}

unsigned int http_client_get(const char *origin, const char *destination, const char *server_ip) {
    char g_request[512];
    snprintf(g_request, sizeof(g_request), "GET /directionlite/v1/walking?origin=%s&destination=%s&ak=%s HTTP/1.1\r\n\
Host: api.map.baidu.com\r\n\
Connection: close\r\n\
\r\n", origin, destination, g_api_key);

    struct sockaddr_in addr = {0};
    int s, r;

    // 创建 socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        printf("Failed to create socket\n");
        return 1;
    }
    printf("Socket created\n");

    // 设置目标服务器地址
    addr.sin_family = AF_INET;
    addr.sin_port = PP_HTONS(SOCK_TARGET_PORT);
    addr.sin_addr.s_addr = inet_addr(server_ip);

    // 连接到服务器
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("Failed to connect to server\n");
        lwip_close(s);
        return 1;
    }

    // 发送 HTTP 请求
    if (lwip_write(s, g_request, strlen(g_request)) < 0) {
        printf("Failed to send HTTP request\n");
        lwip_close(s);
        return 1;
    }

    // 设置接收超时时间为 5 秒
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
        printf("Failed to set socket receiving timeout\n");
        lwip_close(s);
        return 1;
    }

    // 读取 HTTP 响应并打印
    r = 0;
    char* json_start = NULL;
    char* json_end = NULL;
        // do {
    memset(recv_buf, 0, sizeof(recv_buf));
    r = lwip_read(s, recv_buf, sizeof(recv_buf) - 2);
    printf("%d\n", r);
    json_start = strchr(recv_buf, '{');
    char* tmp = strchr(json_start+1, '}');
    for(int i = 0; i < 5; ++i){
        json_end = tmp;
        tmp = strchr(tmp+1, '}');
    }
    memcpy(json_buf, json_start, json_end-json_start);
    int len = json_end-json_start;
    json_buf[len] = '}';
    json_buf[len+1] = ']';
    json_buf[len+2] = '}';
    json_buf[len+3] = ']';
    json_buf[len+4] = '}';
    json_buf[len+5] = '}';
    len += 5;
    parse_https_recv(json_buf, len);
        // } while (r > 0);

    lwip_close(s);  // 关闭 socket 连接
    return 0;
}

void HttpClientEntry(void) {
    WifiDeviceConfig config = {0};
    osThreadAttr_t attr = {0};
    //wifi名称为ABCD 密码为123456789
    strcpy_s(config.ssid, WIFI_MAX_SSID_LEN, PARAM_HOTSPOT_SSID);
    strcpy_s(config.preSharedKey, WIFI_MAX_KEY_LEN, PARAM_HOTSPOT_PSK);
    config.securityType = PARAM_HOTSPOT_TYPE;
    g_netId = ConnectToHotspot(&config);
    printf("netId = %d\n", g_netId);
    attr.name = "HttpClientTask";
    attr.stack_size = STACK_SIZE;
    attr.priority = osPriorityNormal;
    cJSON_Hooks hook;
    hook.free_fn = free;
    hook.malloc_fn = malloc;
    cJSON_InitHooks(&hook);

    stm32_gpio_init();
    stm_uart_config();
    // dtof_gpio_init();
    // dtof_uart_config();
    if (osThreadNew(HttpClientTask, NULL, &attr) == NULL) {
        printf("[HttpClientEntry] Failed to create HttpClientTask!\n");
    }
    stm32_task_entry();
}

SYS_RUN(HttpClientEntry);
