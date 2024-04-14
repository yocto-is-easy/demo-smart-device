#include <memory.h>

#include "aht10.h"

#define AHT10_ADDR 0x38

#define AHT10_INIT_CMD 0xE1
#define AHT10_MEASURE_CMD 0xAC
#define AHT10_SOFT_RESET_CMD 0xBA

#define AHT10_AFTER_MEASURE_DELAY 100

#define AHT10_READ_DATA_SIZE 6

#define TWO_IN_20 1048576

bool aht10_get_data(aht10_data_t* ret) {
    aht10_measure_cmd();
    vTaskDelay(AHT10_AFTER_MEASURE_DELAY / portTICK_PERIOD_MS);
    
    esp_err_t err;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    err = i2c_master_start(cmd);
    err = i2c_master_write_byte(cmd, (AHT10_ADDR << 1) | I2C_MASTER_READ, true);

    uint8_t read_data[AHT10_READ_DATA_SIZE] = {0};
    err = i2c_master_read(cmd, read_data, AHT10_READ_DATA_SIZE, I2C_MASTER_ACK);
    err = i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    if(err != ESP_OK) {
        //ESP_LOGE("aht10", "get_data: i2c_master_cmd_begin failed\n");
        return false;
    }

    i2c_cmd_link_delete(cmd);

    //memcpy(&ret->status, read_data, 1);
    ret->status.busy = aht10_is_busy(read_data[0]);
    ret->status.mode = aht10_get_working_mode(read_data[0]);
    ret->status.calibrated = aht10_is_calibrated(read_data[0]);


    uint32_t humidity = 0;
    uint32_t temperature = 0;

    humidity = read_data[1];
    humidity <<= 8;
    humidity |= read_data[2];
    humidity <<= 4;
    humidity |= read_data[3] >> 4;

    temperature = read_data[3] & 0x0F;
    temperature <<= 8;
    temperature |= read_data[4];
    temperature <<= 8;
    temperature |= read_data[5];

    // convert to real values
    ret->humidity = (float)humidity * 100 / TWO_IN_20;
    ret->temperature = (float)temperature * 200 / TWO_IN_20 - 50;

    return true;
}

void aht10_device_init() {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    aht10_init_cmd();
}

bool aht10_is_device_connected() {
    esp_err_t err;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AHT10_ADDR << 1) | I2C_MASTER_WRITE, true);

    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);

    i2c_cmd_link_delete(cmd);

    return err == ESP_OK;
}

void write_cmd(uint8_t device_cmd) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AHT10_ADDR << 1) | I2C_MASTER_WRITE, true);

    uint8_t data[3] = {device_cmd, 0x33, 0x00};
    i2c_master_write(cmd, data, 3, true);

    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    if(err != ESP_OK) {
        ESP_LOGE("aht10", "write_cmd: i2c_master_cmd_begin failed\n");
    }

    i2c_cmd_link_delete(cmd);
}

void aht10_init_cmd() {
    write_cmd(AHT10_INIT_CMD);
}

void aht10_measure_cmd() {
    write_cmd(AHT10_MEASURE_CMD);
}

void aht10_soft_reset_cmd() {
    write_cmd(AHT10_SOFT_RESET_CMD);
}

bool aht10_is_busy(uint8_t status) {
    return status & 0x80; // the 7th bit
}

bool aht10_is_calibrated(uint8_t status) {
    return status & 0x08; // the 3rd bit
}

working_mode_t aht10_get_working_mode(uint8_t status) {
    // 0011 0000 = 0x30
    uint8_t mode = (status & 0x30); // 5th and 6th bits

    if(mode == 0x00) {
        return NOR;
    } else if(mode & 0x10) {
        return CYC;
    } else if(mode & 0x20) {
        return CMD;
    } else {
        return -1;
    }
}
