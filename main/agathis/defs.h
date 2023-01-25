// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AGATHIS_98RXG9U8BUUHY401
#define AGATHIS_98RXG9U8BUUHY401

//#define AG_I2C_OFFSET  0x20

#define AG_STORAGE_SIZE     128

#define AG_CAP_SW_TMC       0x01

typedef enum {
    AG_ERR_NONE = 0,
    AG_ERR_MULTI_MASTER,
} AGError_t;

typedef enum {
    AG_LED_OFF = 0,
    AG_LED_ON,
    AG_LED_ERROR,
    AG_LED_BLINK,
} AGLEDState_t;

#endif /* AGATHIS_98RXG9U8BUUHY401 */
