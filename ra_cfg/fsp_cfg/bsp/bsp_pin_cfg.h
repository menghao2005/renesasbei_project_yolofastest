/* generated configuration header file - do not edit */
#ifndef BSP_PIN_CFG_H_
#define BSP_PIN_CFG_H_
#include "r_ioport.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

#define FLASH (BSP_IO_PORT_01_PIN_09)
#define USER_LED (BSP_IO_PORT_01_PIN_10)
#define D0 (BSP_IO_PORT_04_PIN_00)
#define D1 (BSP_IO_PORT_04_PIN_01)
#define D2 (BSP_IO_PORT_04_PIN_05)
#define D3 (BSP_IO_PORT_04_PIN_06)
#define DIR_R (BSP_IO_PORT_05_PIN_13)
#define D4 (BSP_IO_PORT_07_PIN_00)
#define D5 (BSP_IO_PORT_07_PIN_01)
#define D6 (BSP_IO_PORT_07_PIN_02)
#define D7 (BSP_IO_PORT_07_PIN_03)
#define ov_RST (BSP_IO_PORT_07_PIN_09)
#define PWDN (BSP_IO_PORT_07_PIN_10)
#define DIR_L (BSP_IO_PORT_09_PIN_06)

extern const ioport_cfg_t g_bsp_pin_cfg; /* RA8T2_CPKNET.pincfg */

extern const ioport_cfg_t g_bsp_pin_cfg1; /* RA8P1_CPKHMI.pincfg */

extern const ioport_cfg_t g_bsp_pin_cfg0; /* New Configuration */

void BSP_PinConfigSecurityInit();

/* Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER
#endif /* BSP_PIN_CFG_H_ */
