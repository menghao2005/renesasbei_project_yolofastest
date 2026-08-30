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

/*----------------------------------------------------------------------------------------------------------------------
 * 补充说明 : MIPI DSI 显示对外接口。
 *            lcd_table_setting_t 为面板初始化命令表条目{长度, 数据, 命令ID, 标志};
 *            0xFE/0xFD 为本文件自定义的伪命令 ID,分别表示"延时 size 毫秒"与"命令表结束"。
 *---------------------------------------------------------------------------------------------------------------------*/

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

/* CPKHMI RA8D1 V1 */
#define PIN_DISPLAY_INT                              (BSP_IO_PORT_00_PIN_02)
#define PIN_DISPLAY_RST                              (BSP_IO_PORT_00_PIN_00)
#define PIN_DISPLAY_BACKLIGHT                        (BSP_IO_PORT_09_PIN_07)



typedef struct
{
    uint16_t             size;
    unsigned char        buffer[MIPI_DSI_LCD_CMD_MAX_BYTES];
    mipi_dsi_cmd_id_t    cmd_id;
    mipi_dsi_cmd_flag_t flags;
} lcd_table_setting_t;

extern volatile bool g_vsync_flag, g_message_sent;
extern const lcd_table_setting_t g_lcd_init_ydp430_st7102[];
void mipi_dsi_push_table (const lcd_table_setting_t *table);
void mipi_dsi_start_display ();
void handle_error (fsp_err_t err,  const char *err_str);
void touch_screen_reset(void);
void mipi_dsi_entry(void);
void test_single_pixel(void);
void R_IOPORT_PinToggle(ioport_ctrl_t * p_ctrl, bsp_io_port_pin_t pin);

#endif /* MIPI_DSI_EP_H_ */
