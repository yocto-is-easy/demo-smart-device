#ifndef AHT10_H
#define AHT10_H

#include "driver/i2c.h"
#include "esp_log.h"

typedef enum {
    NOR = 0,
    CYC = 1,
    CMD = 2
} working_mode_t;

typedef struct __attribute__((__packed__)) {
    struct {
        bool busy;
        working_mode_t mode;
        bool calibrated;
    } status;
    float humidity;
    float temperature;
} aht10_data_t;

bool aht10_get_data(aht10_data_t* ret);

bool aht10_is_device_connected();

void aht10_device_init();

void aht10_init_cmd();
void aht10_measure_cmd();
void aht10_soft_reset_cmd();

bool aht10_is_busy(uint8_t status);
bool aht10_is_calibrated(uint8_t status);

static const char working_mode_str[][16] = {
    "Normal",
    "Cycle",
    "Command"
};

working_mode_t aht10_get_working_mode(uint8_t status);

#endif // AHT10_H
