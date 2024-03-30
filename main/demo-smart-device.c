#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mocto-smart-device.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

void app_main(void)
{
    msd_configure();

    while(1) {
        // broadcast a UDP packet
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

        struct sockaddr_in broadcast_addr;
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        broadcast_addr.sin_port = htons(1234);

        const char* data = "Hello, world!";
        int err = sendto(sock, data, strlen(data), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        if(err < 0) {
            printf("Error occurred during sending: errno %d\n", errno);
        }

        close(sock);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
