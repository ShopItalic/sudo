// Copyright (c) 2022 Boreas Technologies All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//

#include "configuration/coreDef.h"

#if (BSP_BUILD == BSP_PCB0305)

#include <assert.h>

#include "stm32u5xx.h"
#include "bsp/hal/halGpio.h"
#include "bsp/hal/halSpi.h"
#include "bsp/hal/halI2c.h"
#include "bsp/hal/halLedDriver.h"
#include "bsp/hal/halHapticDriver.h"
#include "boreasUtils/data.h"
#include "bsp/hal/halGpioImpl.h"
#include "bsp/hal/halButton.h"
#include "bsp/stm32u5/drivers/timeStm32u5.h"
#include "boreasUtils/logger.h"
#include "bsp/hal/halUsb.h"
#include "bsp/hal/usb/usbService.h"
#include "bsp/hal/halFlash.h"
#include "bsp/hal/flash/flashWinbondW25Q.h"
#include "bsp/hal/flash/fsFlashDiskIo.h"
#include "service/filesystem/fsManagement.h"
#include "bsp/hal/flash/usbFlashDiskIo.h"
#include "service/time/boreasTime.h"


#define DEBOUNCING_DELAY_MS (15)

#define POWER_OFF_DELAY_MS (2000)
#define POWER_ON_DELAY_MS (100)

static void SystemClock_Config(void);

/*
 * Embedded Peripheral Configurations
 */

#define EXTI0_IRQN_PRIORITY (5)
#define EXTI1_IRQN_PRIORITY (5)
#define EXTI2_IRQN_PRIORITY (5)
#define EXTI3_IRQN_PRIORITY (5)
#define EXTI4_IRQN_PRIORITY (5)
#define EXTI5_IRQN_PRIORITY (5)
#define EXTI6_IRQN_PRIORITY (5)
#define EXTI8_IRQN_PRIORITY (5)


typedef enum
{
    GPIO_INDEX_D1_RED_LED = 0,
    GPIO_INDEX_D1_GREEN_LED,
    GPIO_INDEX_D1_BLUE_LED,

    GPIO_INDEX_D2_RED_LED,
    GPIO_INDEX_D2_GREEN_LED,
    GPIO_INDEX_D2_BLUE_LED,

    GPIO_INDEX_BOS1921A_RED_LED,
    GPIO_INDEX_BOS1921A_GREEN_LED,
    GPIO_INDEX_BOS1921A_BLUE_LED,

    GPIO_INDEX_BOS1921B_RED_LED,
    GPIO_INDEX_BOS1921B_GREEN_LED,
    GPIO_INDEX_BOS1921B_BLUE_LED,

    GPIO_INDEX_BOS_SYNC_GPIO,

    GPIO_INDEX_BOS1921A_I2C_SCL,
    GPIO_INDEX_BOS1921A_I2C_SDA,

    GPIO_INDEX_BOS1921B_I2C_SCL,
    GPIO_INDEX_BOS1921B_I2C_SDA,

    GPIO_INDEX_FLASH_SPI_MOSI,
    GPIO_INDEX_FLASH_SPI_MISO,
    GPIO_INDEX_FLASH_SPI_SCLK,
    GPIO_INDEX_FLASH_SPI_CS,
    GPIO_INDEX_FLASH_HOLD,

    GPIO_INDEX_IO0,
    GPIO_INDEX_IO1,
    GPIO_INDEX_IO2,
    GPIO_INDEX_IO3,

    GPIO_INDEX_USER_BUTTON,

    GPIO_INDEX_USB_DM,
    GPIO_INDEX_USB_DP,

    GPIO_INDEX_VDDIO_CONTROL,
    GPIO_INDEX_VBUS_CONTROL,
} GpioIndex;

static GpioHalDriver gpioDriverArray[] = {
    {
        //D1_RED_LED
        .gpioHal = {
            .ioPort = GPIOD,
            .ioDef = {
                .Pin = GPIO_PIN_12,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //D1_GREEN_LED
        .gpioHal = {
            .ioPort = GPIOD,
            .ioDef = {
                .Pin = GPIO_PIN_13,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //D1_BLUE_LED
        .gpioHal = {
            .ioPort = GPIOD,
            .ioDef = {
                .Pin = GPIO_PIN_15,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
    //D2_RED_LED
        .gpioHal = {
            .ioPort = GPIOG,
            .ioDef = {
                .Pin = GPIO_PIN_5,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
        .initState = GPIO_PIN_SET
        }
    },
    {
        //D2_GREEN_LED
        .gpioHal = {
            .ioPort = GPIOG,
            .ioDef = {
                .Pin = GPIO_PIN_8,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },

    {
        //D2_BLUE_LED
        .gpioHal = {
            .ioPort = GPIOG,
            .ioDef = {
                .Pin = GPIO_PIN_3,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //BOS1921A_RED_LED
        .gpioHal = {
            .ioPort = GPIOC,
            .ioDef = {
                .Pin = GPIO_PIN_4,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //BOS1921A_GREEN_LED
        .gpioHal = {
            .ioPort = GPIOA,
            .ioDef = {
                .Pin = GPIO_PIN_6,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },

    {
        //BOS1921A_BLUE_LED
        .gpioHal = {
            .ioPort = GPIOA,
            .ioDef = {
                .Pin = GPIO_PIN_5,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //BOS1921B_RED_LED
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_5,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //BOS1921B_GREEN_LED
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_3,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },

    {
        //BOS1921B_BLUE_LED
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_1,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //BOS_SYNC_GPIO
        .gpioHal = {
            .ioPort = GPIOA,
            .ioDef = {
                .Pin = GPIO_PIN_2,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_RESET,
        }
    },
    {
        //BOS1921A_I2C_SCL
        .gpioHal = {
            .ioPort = GPIOC,
            .ioDef = {
                .Pin = GPIO_PIN_0,
                .Mode = GPIO_MODE_AF_OD,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF4_I2C3
            }
        }
    },
    {
        //BOS1921A_I2C_SDA
        .gpioHal = {
            .ioPort = GPIOC,
            .ioDef = {
                .Pin = GPIO_PIN_1,
                .Mode = GPIO_MODE_AF_OD,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF4_I2C3
            }
        }
    },
    {
        //BOS1921B_I2C_SCL,
        .gpioHal = {
            .ioPort = GPIOF,
            .ioDef = {
                .Pin = GPIO_PIN_1,
                .Mode = GPIO_MODE_AF_OD,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF4_I2C2
            }
        }
    },
    {
        //BOS1921B_I2C_SDA
        .gpioHal = {
            .ioPort = GPIOF,
            .ioDef = {
                .Pin = GPIO_PIN_0,
                .Mode = GPIO_MODE_AF_OD,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF4_I2C2
            }
        }
    },
    {
        //FLASH_SPI_MOSI
        .gpioHal = {
            .ioPort = GPIOB,
            .ioDef = {
                .Pin = GPIO_PIN_15,
                .Mode = GPIO_MODE_AF_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF5_SPI2
            }
        }
    },
    {
        //FLASH_SPI_MISO
        .gpioHal = {
            .ioPort = GPIOB,
            .ioDef = {
                .Pin = GPIO_PIN_14,
                .Mode = GPIO_MODE_AF_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF5_SPI2
            }
        }
    },
    {
        //FLASH_SPI_SCLK
        .gpioHal = {
            .ioPort = GPIOB,
            .ioDef = {
                .Pin = GPIO_PIN_13,
                .Mode = GPIO_MODE_AF_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF5_SPI2
            }
        }
    },
    {
        //FLASH_SPI_CS
        .gpioHal = {
            .ioPort = GPIOB,
            .ioDef = {
                .Pin = GPIO_PIN_12,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,

            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //FLASH_HOLD
        .gpioHal = {
            .ioPort = GPIOD,
            .ioDef = {
                .Pin = GPIO_PIN_10,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            },
            .initState = GPIO_PIN_SET
        }
    },
    {
        //IO0
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_7,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_PULLUP,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            },
            .initState = GPIO_PIN_RESET,

        }
    },
    {
        //IO1
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_9,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_PULLUP,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            },
            .initState = GPIO_PIN_RESET
        }
    },
    {
        //IO2
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_10,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_PULLUP,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            },
            .initState = GPIO_PIN_RESET
        }
    },
    {
        //IO3
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_8,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_PULLUP,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            },
            .initState = GPIO_PIN_RESET
        }
    },
    {
        //USER_BUTTON
        .gpioHal = {
            .ioPort = GPIOB,
            .ioDef = {
                .Pin = GPIO_PIN_6,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            },
            .ioIsr = {
                .irqNumber = EXTI6_IRQn,
                .irqPriority = EXTI6_IRQN_PRIORITY,
                .irqSubPriority = 0
            },
        }
    },

    {
        //USB_DM
        .gpioHal = {
            .ioPort = GPIOA,
            .ioDef = {
                .Pin = GPIO_PIN_11,
                .Mode = GPIO_MODE_AF_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF10_USB,
            },
        }
    },
    {
        //USB_DP
        .gpioHal = {
            .ioPort = GPIOA,
            .ioDef = {
                .Pin = GPIO_PIN_12,
                .Mode = GPIO_MODE_AF_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
                .Alternate = GPIO_AF10_USB,
            },
        }
    },
    {
        // VDDIO Control
        .gpioHal = {
            .ioPort = GPIOF,
            .ioDef = {
                .Pin = GPIO_PIN_2,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_LOW
            },
            .initState = GPIO_PIN_RESET
        }
    },
    {
        // VBUS Control
        .gpioHal = {
            .ioPort = GPIOC,
            .ioDef = {
                .Pin = GPIO_PIN_5,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_LOW
            },
            .initState = GPIO_PIN_RESET
        }
    }
};

static GpioHalDriver icGpioDriverArrayOutput[] = {
    {
        //BOS1921A_GPIO
        .gpioHal = {
            .ioPort = GPIOC,
            .ioDef = {
                .Pin = GPIO_PIN_3,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET,
        }
    },
    {
        //BOS1921B_GPIO
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_4,
                .Mode = GPIO_MODE_OUTPUT_PP,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_SET,
        }
    }
};

static GpioHalDriver icGpioDriverArrayInput[2] = {
    {
        //BOS1921A_GPIO
        .gpioHal = {
            .ioPort = GPIOC,
            .ioDef = {
                .Pin = GPIO_PIN_3,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_RESET,
            .ioIsr = {
                .irqNumber = EXTI3_IRQn,
                .irqPriority = EXTI3_IRQN_PRIORITY,
                .irqSubPriority = 0
            }
        }
    },
    {
        //BOS1921B_GPIO
        .gpioHal = {
            .ioPort = GPIOE,
            .ioDef = {
                .Pin = GPIO_PIN_4,
                .Mode = GPIO_MODE_INPUT,
                .Pull = GPIO_NOPULL,
                .Speed = GPIO_SPEED_FREQ_LOW,
            },
            .initState = GPIO_PIN_RESET,
            .ioIsr = {
                .irqNumber = EXTI4_IRQn,
                .irqPriority = EXTI4_IRQN_PRIORITY,
                .irqSubPriority = 0
            }
        }
    }
};

typedef enum
{
    SPI_INDEX_FLASH = 0,
} SpiIndex;

static SpiHalDriver spiDrivers[NBR_OF_SPI] = {
    //SPI_INDEX_FLASH
    {
        .spiHal = {
            .instance = SPI2,
            .configuration = {
                .Mode = SPI_MODE_MASTER,
                .Direction = SPI_DIRECTION_2LINES,
                .DataSize = SPI_DATASIZE_8BIT,
                .CLKPolarity = SPI_POLARITY_LOW,
                .CLKPhase = SPI_PHASE_1EDGE,
                .NSS = SPI_NSS_SOFT,
                .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2,
                .FirstBit = SPI_FIRSTBIT_MSB,
                .TIMode = SPI_TIMODE_DISABLE,
                .CRCCalculation = SPI_CRCCALCULATION_DISABLE,
                .CRCPolynomial = 10,
                .NSSPMode = SPI_NSS_PULSE_DISABLE,
                .NSSPolarity = SPI_NSS_POLARITY_LOW,
                .FifoThreshold = SPI_FIFO_THRESHOLD_01DATA,
                .MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE,
                .MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE,
                .MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE,
                .MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE,
                .IOSwap = SPI_IO_SWAP_DISABLE,
                .ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY,
                .ReadyPolarity = SPI_RDY_POLARITY_HIGH,
            },
            .mosi = &gpioDriverArray[GPIO_INDEX_FLASH_SPI_MOSI].gpio,
            .miso = &gpioDriverArray[GPIO_INDEX_FLASH_SPI_MISO].gpio,
            .sclk = &gpioDriverArray[GPIO_INDEX_FLASH_SPI_SCLK].gpio,
            .cs = &gpioDriverArray[GPIO_INDEX_FLASH_SPI_CS].gpio,

            /**
             * NOTE
             * Even if NSS is configured to software managed (.NSS = SPI_NSS_SOFT,), we have to disable
             * the CS management from the SPI driver. This SPI port is used by the Flash driver which is
             * responsible to manage the CS.
             */
            .isCsManaged = false
        },
    }
};

typedef enum
{
    I2C_INDEX_BOS1921A = 0,
    I2C_INDEX_BOS1921B
} I2cIndex;

static I2cHalDriver i2cDrivers[NBR_OF_I2C] = {
    //BOS1921A
    {
        .type = I2C_TYPE_MASTER,
        .i2cHal = {
            .mode = I2C_MODE_FAST_MODE_PLUS,
            .type = HAL_I2C_MODE_MASTER,
            .instance.i2c = I2C3,
            .configuration.i2c = {
                /*
                 * Timing register value calculated using the reference manual:
                 *  - Ti2cclk = 20.83ns (48MHz)
                 *  - PRESC = 0
                 *  - SCLDEL = 2 (3 * 20.83ns = ~62.5ns)
                 *  - SDADEL = 0 (0 ns)
                 *  - SCLH = 12 (13 * 20.83ns = 270ns)
                 *  - SCLL = 23 (24 * 20.83ns = 499ns)
                 */
                .Timing = 0x00200C17,
                .OwnAddress1 = 0,
                .AddressingMode = I2C_ADDRESSINGMODE_7BIT,
                .DualAddressMode = I2C_DUALADDRESS_DISABLE,
                .OwnAddress2 = 0,
                .OwnAddress2Masks = 0,
                .GeneralCallMode = I2C_GENERALCALL_DISABLE,
                .NoStretchMode = I2C_NOSTRETCH_DISABLE,
            },
            .sda = &gpioDriverArray[GPIO_INDEX_BOS1921A_I2C_SDA].gpio,
            .scl = &gpioDriverArray[GPIO_INDEX_BOS1921A_I2C_SCL].gpio,
            .timeout = 10000,
        }
    },
    //BOS1921B
    {
        .type = I2C_TYPE_MASTER,
        .i2cHal = {
            .mode = I2C_MODE_FAST_MODE_PLUS,
            .type = HAL_I2C_MODE_MASTER,
            .instance.i2c = I2C2,
            .configuration.i2c = {
                /*
                 * Timing register value calculated using the reference manual:
                 *  - Ti2cclk = 20.83ns (48MHz)
                 *  - PRESC = 0
                 *  - SCLDEL = 2 (3 * 20.83ns = ~62.5ns)
                 *  - SDADEL = 0 (0 ns)
                 *  - SCLH = 12 (13 * 20.83ns = 270ns)
                 *  - SCLL = 23 (24 * 20.83ns = 499ns)
                 */
                .Timing = 0x00200C17,
                .OwnAddress1 = 0,
                .AddressingMode = I2C_ADDRESSINGMODE_7BIT,
                .DualAddressMode = I2C_DUALADDRESS_DISABLE,
                .OwnAddress2 = 0,
                .OwnAddress2Masks = 0,
                .GeneralCallMode = I2C_GENERALCALL_DISABLE,
                .NoStretchMode = I2C_NOSTRETCH_DISABLE,
            },
            .sda = &gpioDriverArray[GPIO_INDEX_BOS1921B_I2C_SDA].gpio,
            .scl = &gpioDriverArray[GPIO_INDEX_BOS1921B_I2C_SCL].gpio,
            .timeout = 10000,
        }
    }
};


static UsbHalDriver usbDrivers[NBR_OF_USB] = {
    {
        .usbHal = {
            .dmGpio = &gpioDriverArray[GPIO_INDEX_USB_DM].gpio,
            .dpGpio = &gpioDriverArray[GPIO_INDEX_USB_DP].gpio,
            .sofGpio = NULL,
            .pcdhandle = {
                .Instance = USB_OTG_FS,
                .Init = {
                    .dev_endpoints = 6,
                    .ep0_mps = 64,
                    .speed = PCD_SPEED_FULL,
                    .dma_enable = DISABLE,
                    .phy_itface = PCD_PHY_EMBEDDED,
                    .Sof_enable = ENABLE,
                    .low_power_enable = DISABLE,
                    .lpm_enable = DISABLE,
                    .battery_charging_enable = DISABLE,
                    .vbus_sensing_enable = DISABLE,
                    .use_dedicated_ep1 = DISABLE,
                }
            },
            .usbHandle = {
                .id = DEVICE_FS,
                .pDesc = &usbStringDescriptor,
                .pClass = &usbDeviceClass,
            }
        }
    }
};

static LedDriver ledDrivers[NBR_OF_LED] = {
    {.gpio = &gpioDriverArray[GPIO_INDEX_D1_RED_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_D1_RED},
    {.gpio = &gpioDriverArray[GPIO_INDEX_D1_GREEN_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_D1_GREEN},
    {.gpio = &gpioDriverArray[GPIO_INDEX_D1_BLUE_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_D1_BLUE},

    {.gpio = &gpioDriverArray[GPIO_INDEX_D2_RED_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_D2_RED},
    {.gpio = &gpioDriverArray[GPIO_INDEX_D2_GREEN_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_D2_GREEN},
    {.gpio = &gpioDriverArray[GPIO_INDEX_D2_BLUE_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_D2_BLUE},

    {.gpio = &gpioDriverArray[GPIO_INDEX_BOS1921A_RED_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_DA_RED},
    {.gpio = &gpioDriverArray[GPIO_INDEX_BOS1921A_GREEN_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_DA_GREEN},
    {.gpio = &gpioDriverArray[GPIO_INDEX_BOS1921A_BLUE_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_DA_BLUE},

    {.gpio = &gpioDriverArray[GPIO_INDEX_BOS1921B_RED_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_DB_RED},
    {.gpio = &gpioDriverArray[GPIO_INDEX_BOS1921B_GREEN_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_DB_GREEN},
    {.gpio = &gpioDriverArray[GPIO_INDEX_BOS1921B_BLUE_LED].gpio, .polarity = LED_POLARITY_ACTIVE_LOW, .id = LedID_DB_BLUE},
};

typedef enum
{
    HAPTIC_DRIVER_INDEX_BOS1921A = 0,
    HAPTIC_DRIVER_INDEX_BOS1921B,
} HapticDriverIndex;

static HapticSystDriver halHapticDrivers[NBR_OF_IC] = {
    {
        .slotId = 0,
        .type = HapticDriverType_BOS1921,
        .conf = {
            .bos1921 = {
                .i2c = &i2cDrivers[I2C_INDEX_BOS1921A].master,
                .gpio = &icGpioDriverArrayInput[0].gpio,
                .i2cAddress = BOS1921_I2C_DEFAULT_ADDRESS
            },
        },
    },
    {
        .slotId = 1,
        .type = HapticDriverType_BOS1921,
        .conf = {
            .bos1921 = {
                .i2c = &i2cDrivers[I2C_INDEX_BOS1921B].master,
                .gpio = &icGpioDriverArrayInput[1].gpio,
                .i2cAddress = BOS1921_I2C_DEFAULT_ADDRESS
            },
        },
    }
};

static ButtonDriver buttonDrivers[NBR_OF_BUTTON] = {
    {.gpio = &gpioDriverArray[GPIO_INDEX_USER_BUTTON].gpio, .debouncingDelayMs = DEBOUNCING_DELAY_MS}
};

static FlashHalDriver flashDriverArray[NBR_OF_FLASH] = {
    {
        .type = FLASH_W25Q_TYPE,
        .peripheral = FLASH_W25Q_PERIPHERAL,
        .flashHal = {
            .spi = &spiDrivers[SPI_INDEX_FLASH].spi,
            .info = FLASH_W25Q_HAL_INFO,
            .config = FLASH_W25Q_HAL_CONFIG,
            .commands = FLASH_W25Q_HAL_COMMANDS,
        }
    }
};

void Error_Handler(void);

static bool bos1921TryInitHapticDriver()
{
    bool result = false;

    for (uint8_t i = 0; i < DATA_ARRAY_LENGTH(halHapticDrivers); i++)
    {
        HapticSystDriver* halHapticDriver = &halHapticDrivers[i];

        uint8_t icAddr = halHapticDriver->conf.bos1921.i2cAddress;
        if (icAddr != BOS1921_I2C_DEFAULT_ADDRESS)
        {
            halGpiosInit(&icGpioDriverArrayOutput[i], 1);
            const Gpio* outGpio = icGpioDriverArrayOutput[i].gpio;
            halGpiosUnit(outGpio);
        }

        result = halGpiosInit(&icGpioDriverArrayInput[i], 1);
        result = result && halHapticDriverInitSingle(halHapticDriver);

        if (!result)
        {
            if(i == 0)
            {
                const Led* led = halGetLedById(LedID_DA_RED);
                if (led)
                {
                    led->set(led, LED_STATE_ON);
                }
            }
            else if (i == 1)
            {
                const Led* led = halGetLedById(LedID_DB_RED);
                if (led)
                {
                    led->set(led, LED_STATE_ON);
                }
            }
        }
    }

    return result;
}

static bool bos1921HapticDriverInit()
{
    bool result = true;

    if (!bos1921TryInitHapticDriver())
    {
        for (uint8_t i = 0; i < DATA_ARRAY_LENGTH(halHapticDrivers); i++)
        {
            HapticSystDriver* halHapticDriver = &halHapticDrivers[i];
            halHapticDriverUnint(halHapticDriver->driver);
            halGpiosUnit(icGpioDriverArrayInput[i].gpio);
        }

        gpioDriverArray[GPIO_INDEX_VDDIO_CONTROL].gpio->set(gpioDriverArray[GPIO_INDEX_VDDIO_CONTROL].gpio, GPIO_STATE_HIGH);
        gpioDriverArray[GPIO_INDEX_VBUS_CONTROL].gpio->set(gpioDriverArray[GPIO_INDEX_VBUS_CONTROL].gpio, GPIO_STATE_HIGH);

        timeWaitMs(POWER_OFF_DELAY_MS);

        gpioDriverArray[GPIO_INDEX_VDDIO_CONTROL].gpio->set(gpioDriverArray[GPIO_INDEX_VDDIO_CONTROL].gpio, GPIO_STATE_LOW);
        gpioDriverArray[GPIO_INDEX_VBUS_CONTROL].gpio->set(gpioDriverArray[GPIO_INDEX_VBUS_CONTROL].gpio, GPIO_STATE_LOW);

        timeWaitMs(POWER_ON_DELAY_MS);

        bos1921TryInitHapticDriver();
    }

    return result;
}

/**
 * @brief This function checks if the SDA line is low and if so, it toggles the SCL line to try to release the SDA line.
 *
 * When the MCU reset during an i2c transaction, the BOS1921 sometimes holds the SDA line low and the MCU
 * is not able to generate a start condition on the bus.
 *
 * @return
 */
static bool checkI2CBus()
{
    bool res = true;

    GpioHalDriver driversTemp[] = {
        {
            .gpio = NULL,
            .gpioHal = {
                .ioPort = GPIOC,
                .ioDef = {
                    .Pin = GPIO_PIN_1,
                    .Mode = GPIO_MODE_INPUT,
                    .Pull = GPIO_NOPULL,
                    .Speed = GPIO_SPEED_LOW
                },
                .initState = GPIO_PIN_RESET
            }
        },
        {
            .gpio = NULL,
            .gpioHal = {
                .ioPort = GPIOC,
                .ioDef = {
                    .Pin = GPIO_PIN_0,
                    .Mode = GPIO_MODE_OUTPUT_PP,
                    .Pull = GPIO_NOPULL,
                    .Speed = GPIO_SPEED_LOW
                },
                .initState = GPIO_PIN_SET
            }
        },
        {
            .gpio = NULL,
            .gpioHal = {
                .ioPort = GPIOF,
                .ioDef = {
                    .Pin = GPIO_PIN_1,
                    .Mode = GPIO_MODE_INPUT,
                    .Pull = GPIO_NOPULL,
                    .Speed = GPIO_SPEED_LOW
                },
                .initState = GPIO_PIN_RESET
            }
        },
        {
            .gpio = NULL,
            .gpioHal = {
                .ioPort = GPIOF,
                .ioDef = {
                    .Pin = GPIO_PIN_0,
                    .Mode = GPIO_MODE_OUTPUT_PP,
                    .Pull = GPIO_NOPULL,
                    .Speed = GPIO_SPEED_LOW
                },
                .initState = GPIO_PIN_SET
            }
        }
    };

    res = halGpiosInit(driversTemp, DATA_ARRAY_LENGTH(driversTemp));

    for (size_t i = 0; res && i < DATA_ARRAY_LENGTH(driversTemp); i += 2)
    {
        uint8_t nbAttempts = 0;
        uint8_t maxAttempts = 20;
        const Gpio* sda = driversTemp[i].gpio;
        const Gpio* slc = driversTemp[i + 1].gpio;
        while (res && sda->get(sda) == GPIO_STATE_LOW && ((++nbAttempts) <= maxAttempts))
        {
            res = slc->set(slc, GPIO_STATE_LOW);
            timeWaitUs(1);
            res = res && slc->set(slc, GPIO_STATE_HIGH);
            timeWaitUs(1);
        }
        if (nbAttempts > maxAttempts)
        {
            res = false;
        }
        if (nbAttempts > 0)
        {
            logDebug("I2C bus %d fixed after %d attempts", i / 2, nbAttempts);
        }
    }

    for (size_t i = 0; i < DATA_ARRAY_LENGTH(driversTemp); ++i)
    {
        if (driversTemp[i].gpio != NULL)
        {
            res = halGpiosUnit(driversTemp[i].gpio) && res;
        }
    }

    return res;
}

void Error_Handler(void)
{
    const Led* d1Green = halGetLedById(LedID_D1_GREEN);
    const Led* d1Red = halGetLedById(LedID_D1_RED);
    const Led* daGreen = halGetLedById(LedID_DA_GREEN);

    if (d1Green != NULL)
    {
        d1Green->set(d1Green, LED_STATE_OFF);
    }
    if (daGreen != NULL)
    {
        daGreen->set(daGreen, LED_STATE_OFF);
    }
    if (d1Red != NULL)
    {
        d1Red->set(d1Red, LED_STATE_ON);
    }
    while (true)
    {
        //Do nothing
    }
}

bool board0305Init()
{
    logSetLevel(LogLevel_Debug);

    HAL_PWREx_DisableUCPDDeadBattery();

    __HAL_RCC_PWR_CLK_ENABLE();

    HAL_Init();

    SystemClock_Config();

    bool res = halTimeInit();

    res = res && checkI2CBus();

    res = res && halGpiosInit(gpioDriverArray, DATA_ARRAY_LENGTH(gpioDriverArray));

    res = res && halI2csInit(i2cDrivers, DATA_ARRAY_LENGTH(i2cDrivers));
    res = res && halSpisInit(spiDrivers, DATA_ARRAY_LENGTH(spiDrivers));

    res = res && halUsbsInit(usbDrivers, DATA_ARRAY_LENGTH(usbDrivers));

    res = res && halFlashInit(flashDriverArray, DATA_ARRAY_LENGTH(flashDriverArray));
    res = res && halButtonInit(buttonDrivers, DATA_ARRAY_LENGTH(buttonDrivers));

    res = res && halLedDriverInit(ledDrivers, DATA_ARRAY_LENGTH(ledDrivers));
    res = res && bos1921HapticDriverInit();

#if (FEATURE_USB_FS == 1 || FEATURE_FILE_SYSTEM_INIT == 1)
    const Flash* flash = halFlashGet(FlashPeripharal_ExternalFs);
#endif

#if (FEATURE_USB_FS == 1)
    if (flash != NULL)
    {
        res = res && usbFlashDiskIoInit(flash);
        const Usb* usb = halGetUsb(0);
        usb->setDeviceType(usb, USB_DEVICE_TYPE_MSC);
        res = res && usb->start(usb);
    }
#endif

#if (FEATURE_FILE_SYSTEM_INIT == 1)
    if (flash != NULL)
    {

        res = res && fsFlashDiskIoInit(flash);
        res = res && fsManagementInit();
    }
#endif
    return res;
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

    /** Configure the main internal regulator output voltage
    */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_MSIK;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
    RCC_OscInitStruct.MSIKClockRange = RCC_MSIKRANGE_0;
    RCC_OscInitStruct.MSIKState = RCC_MSIK_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 96;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 6;
    RCC_OscInitStruct.PLL.PLLR = 4;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_0;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                  | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
    {
        Error_Handler();
    }


    __HAL_RCC_CRS_CLK_ENABLE();

    /** Configures CRS
    */
    RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
    RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
    RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
    RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
    RCC_CRSInitStruct.ErrorLimitValue = 34;
    RCC_CRSInitStruct.HSI48CalibrationValue = 64;

    HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
    PeriphClkInit.I2c3ClockSelection = RCC_I2C3CLKSOURCE_MSIK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
    PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_MSIK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
    PeriphClkInit.Spi1ClockSelection = RCC_SPI1CLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    PeriphClkInit.Spi2ClockSelection = RCC_SPI2CLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    /* Timers Peripheral Clock Enable */
    __HAL_RCC_TIM7_CLK_ENABLE();

    SystemCoreClockUpdate();
}

#endif
