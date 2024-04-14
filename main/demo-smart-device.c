#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "mocto-smart-device.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#include "aht10.h"

void init_i2c() {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    i2c_param_config(I2C_NUM_0, &i2c_config);

    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

QueueHandle_t aht10_upd_reporter_queue;
void aht10_task(void* pvParameter) {
    aht10_device_init();

    while(1) {
        aht10_data_t data;
        if(aht10_get_data(&data)) {
            // printf("busy: %d, mode %s, calibrated: %d\n", 
            //     data.status.busy,
            //     working_mode_str[data.status.mode],
            //     data.status.calibrated);

            printf("humidity: %f, temperature: %f\n",
                data.humidity,
                data.temperature);

            xQueueSend(aht10_upd_reporter_queue, &data, 0);

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

void udp_reporter(void *param) {
    aht10_upd_reporter_queue = xQueueCreate(2, sizeof(aht10_data_t));

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct sockaddr_in broadcast_addr;
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcast_addr.sin_port = htons(1234);

    aht10_data_t data;
    while(1) {
        memset(&data, 0, sizeof(data));
        if(xQueueReceive(aht10_upd_reporter_queue, &data, 0)) {
            ssize_t err = sendto(sock, (void*)&data, sizeof(data), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
            if(err < 0) {
                printf("Error occurred during sending: errno %d\n", errno);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    close(sock);
} 

void app_main(void)
{
    init_i2c();

    msd_configure();

    xTaskCreate(aht10_task, "aht10_task", 2048, NULL, 10, NULL);
    xTaskCreate(udp_reporter, "udp_reporter", 2048, NULL, 10, NULL);
}
