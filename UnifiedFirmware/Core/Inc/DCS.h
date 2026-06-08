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

/* ILI9488 specific commands */
#define ILI9488_INTERFACE_MODE  0xB0
#define ILI9488_FRAME_RATE      0xB1
#define ILI9488_DISPLAY_FUNC    0xB6
#define ILI9488_POWER_CTRL1     0xC0
#define ILI9488_POWER_CTRL2     0xC1
#define ILI9488_VCOM_CTRL       0xC5
#define ILI9488_IMAGE_FUNC      0xE9
#define ILI9488_ADJUST_CTRL3    0xF7
#define ILI9488_POSITIVE_GAMMA  0xE0
#define ILI9488_NEGATIVE_GAMMA  0xE1

/* Display dimensions */
#define DCS_DISPLAY_WIDTH       320
#define DCS_DISPLAY_HEIGHT      480

#endif /* DCS_H */
