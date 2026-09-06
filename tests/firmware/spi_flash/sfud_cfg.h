#ifndef TEST_SPI_FLASH_SFUD_CFG_H
#define TEST_SPI_FLASH_SFUD_CFG_H

#define SFUD_USING_FLASH_INFO_TABLE

enum {
    SFUD_GD25Q32E_DEVICE_INDEX = 0,
};

#define SFUD_FLASH_DEVICE_TABLE                                                \
{                                                                              \
    [SFUD_GD25Q32E_DEVICE_INDEX] = {.name = "TEST_FLASH", .spi.name = "SPI2"}, \
}

#endif
