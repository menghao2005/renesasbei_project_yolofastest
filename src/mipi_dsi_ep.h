/***********************************************************************************************************************
 * File Name    : mipi_dsi_ep.h
 * Description  : Contains data structures and functions used in hal_entry.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

#ifndef MIPI_DSI_EP_H_
#define MIPI_DSI_EP_H_

#include <stdint.h>
#include "r_mipi_dsi_api.h"

#define MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG      ((mipi_dsi_cmd_id_t) 0xFE)
#define MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE    ((mipi_dsi_cmd_id_t) 0xFD)
#define MIPI_DSI_LCD_CMD_MAX_BYTES                   (64U)
#define BYTES_PER_PIXEL                              (2)
#define COLOR_BAND_COUNT                             (8)
#define RESET_FLAG                                   (false)
#define SET_FLAG                                     (true)
#define INITIAL_VALUE                                ('\0')
#define RTT_SELECT_5S                                (1U)
#define RTT_SELECT_15S                               (2U)
#define RTT_SELECT_30S                               (3U)
#define RTT_SELECT_DISABLE_ULPS                      (4U)
#define GPT_UNITS_SECONDS                            (1)
#define GPT_DESIRED_PERIOD_5SEC                      (5)
#define GPT_DESIRED_PERIOD_15SEC                     (15)
#define GPT_DESIRED_PERIOD_30SEC                     (30)

/* CPKHMI RA8D1 V1 */
#define PIN_DISPLAY_INT                              (BSP_IO_PORT_00_PIN_02)
#define PIN_DISPLAY_RST                              (BSP_IO_PORT_00_PIN_00)
#define PIN_DISPLAY_BACKLIGHT                        (BSP_IO_PORT_09_PIN_07)


#define ENTER_ULPS_DELAY                             (8000000)
#define MIPI_DSI_MENU               "\r\nSet time to enter Ultra-Low Power State (ULPS):"\
                                    "\r\n1. Input '1' to enter ULPS after 5s."\
                                    "\r\n2. Input '2' to enter ULPS after 15s."\
                                    "\r\n3. Input '3' to enter ULPS after 30s."\
                                    "\r\n4. Input '4' to always display(Default).\r\n"\
                                    "\r\nNote: After entering Ultra Low Power State (ULPS), please touch the screen to exit this mode."\
                                    "\r\nUser Input:  "
#define MIPI_DSI_INFO_5S            "Enter ULPS after 5 seconds of display\r\n"
#define MIPI_DSI_INFO_15S           "Enter ULPS after 15 seconds of display\r\n"
#define MIPI_DSI_INFO_30S           "Enter ULPS after 30 seconds of display\r\n"
#define MIPI_DSI_INFO_DISABLE_ULPS  "Always display\r\n"

typedef struct
{
    uint16_t             size;
    unsigned char        buffer[MIPI_DSI_LCD_CMD_MAX_BYTES];
    mipi_dsi_cmd_id_t    cmd_id;
    mipi_dsi_cmd_flag_t flags;
} lcd_table_setting_t;

extern volatile bool g_vsync_flag, g_message_sent, g_ulps_flag, g_irq_state, g_timer_overflow;
extern const lcd_table_setting_t g_lcd_init_ydp430_st7102[];
void mipi_dsi_push_table (const lcd_table_setting_t *table);
void mipi_dsi_start_display ();
void handle_error (fsp_err_t err,  const char *err_str);
void touch_screen_reset(void);
void mipi_dsi_entry(void);
void test_single_pixel(void);

#endif /* MIPI_DSI_EP_H_ */
