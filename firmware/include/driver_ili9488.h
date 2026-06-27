#ifndef FIRMWARE_INCLUDE_DRIVER_ILI9488_H
#define FIRMWARE_INCLUDE_DRIVER_ILI9488_H

#define ILI9488_RESOLUTION_X     480
#define ILI9488_RESOLUTION_Y     320
#define ILI9488_COLOR_DEPTH      (16U)
#define ILI9488_PIXEL_FORMAT     0x55

#define ILI9488_CMD_SWRESET      0x01
#define ILI9488_CMD_SLPOUT       0x11
#define ILI9488_CMD_DISPON       0x29
#define ILI9488_CMD_MADCTL       0x36
#define ILI9488_CMD_COLMOD       0x3A
#define ILI9488_CMD_CASET        0x2A
#define ILI9488_CMD_RASET        0x2B
#define ILI9488_CMD_RAMWR        0x2C

#define ILI9488_MADCTL_BGR       (1 << 3)
#define ILI9488_MADCTL_MY        (1 << 7)
#define ILI9488_MADCTL_MX        (1 << 6)
#define ILI9488_MADCTL_MV        (1 << 5)
#define ILI9488_MADCTL_ML        (1 << 4)
#define ILI9488_MADCTL_MH        (1 << 2)

#define ILI9488_SPI_CLOCK_HZ     20000000

#endif
