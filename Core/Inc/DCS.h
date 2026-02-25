#ifndef DCS_H
#define DCS_H

/* MIPI DCS standard commands */
#define DCS_SOFT_RESET          0x01
#define DCS_EXIT_SLEEP_MODE     0x11
#define DCS_ENTER_SLEEP_MODE    0x10
#define DCS_SET_DISPLAY_ON      0x29
#define DCS_SET_DISPLAY_OFF     0x28
#define DCS_SET_COLUMN_ADDRESS  0x2A
#define DCS_SET_PAGE_ADDRESS    0x2B
#define DCS_WRITE_MEMORY_START  0x2C
#define DCS_SET_ADDRESS_MODE    0x36
#define DCS_SET_PIXEL_FORMAT    0x3A
#define DCS_SET_TEAR_ON         0x35
#define DCS_ENTER_INVERT_MODE   0x21
#define DCS_EXIT_INVERT_MODE    0x20

/* ST7789 specific commands */
#define DCS_PORCH_SETTING       0xB2
#define DCS_GATE_CONTROL        0xB7
#define DCS_VCOMS_SETTING       0xBB
#define DCS_LCM_CONTROL         0xC0
#define DCS_VRH_COMMAND_ENABLE  0xC2
#define DCS_VRH_SET             0xC3
#define DCS_FRAME_RATE_CONTROL  0xC6
#define DCS_POWER_CONTROL_1     0xD0
#define DCS_POSITIVE_GAMMA      0xE0
#define DCS_NEGATIVE_GAMMA      0xE1

/* Display dimensions */
#define DCS_DISPLAY_WIDTH       240
#define DCS_DISPLAY_HEIGHT      240

#endif /* DCS_H */
