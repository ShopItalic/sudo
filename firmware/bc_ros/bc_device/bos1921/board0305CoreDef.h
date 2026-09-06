//
// Description: BSP Configuration for the Boreas PCB#0140
// Created on 29/06/2021
// Copyright (c) 2020 Boreas Technologies All rights reserved.
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

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once


#define NBR_OF_LED (4*3)

#define NBR_OF_I2C (2)
#define NBR_OF_USB (1)
#define NBR_OF_GPIO (34)
#define NBR_OF_SPI (1)
#define NBR_OF_BUTTON (1)
#define NBR_OF_FLASH (1)

#define USBD_PRODUCT_STRING_FS_POSTFIX " (BOS1921)"

#define FEATURE_PERSISTENCE (1)

#define BOS1921_NBR_OF_INSTANCE (2)
#define NBR_OF_IC (BOS1921_NBR_OF_INSTANCE)
#define SENSING_NBR_OF_DETECTION_CONFIG (0)
