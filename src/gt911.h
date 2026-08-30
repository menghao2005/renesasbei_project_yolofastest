/***********************************************************************************************************************
 * File Name    : gt911.h
 * Description  : Contains data structures and functions used in mipi_dsi_ep.c.
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

#ifndef GT911_H_
#define GT911_H_

/*----------------------------------------------------------------------------------------------------------------------
 * 补充说明 : 触摸控制器对外接口(保留 gt911 文件名,面板实际为 Sitronix ST7123)。
 *            TP_INT 外部中断只置标志,主循环调用 st7123_touch_irq_print_task() 读取并解析
 *            报告表(最多 5 点、每点 8 字节),结果存入 g_st7123_touch_* 供 UI 使用;
 *            下方 gt911_* / g_touch_* 宏为兼容旧代码的别名。
 *---------------------------------------------------------------------------------------------------------------------*/

#include "hal_data.h"

/* GT911 驱动遗留宏(本面板实际使用下方 ST7123 寄存器定义),保留兼容 */
#define BUFFER_LENGTH                                (10)
#define GTP_READ_COOR_ADDR                           (0x814E)
#define ST7123_BUFFER_STATUS_READY                   (0x80)
#define GT911_BUFFER_STATUS_READY                    ST7123_BUFFER_STATUS_READY
#define GTP_ADDR_LENGTH                              (2)

/* Keep the original gt911 file name, but this panel uses a Sitronix ST7123 touch controller. */
#define ST7123_I2C_SLAVE_ADDR                        (0x55U)
#define ST7123_REG_STATUS                            (0x0001U)
#define ST7123_REG_REPORT_TABLE                      (0x0010U)
#define ST7123_MAX_TOUCH_POINTS                      (5U)
#define ST7123_POINT_STRIDE_BYTES                    (8U)
#define ST7123_REPORT_HEADER_BYTES                   (4U)
#define ST7123_PANEL_WIDTH                           (480U)
#define ST7123_PANEL_HEIGHT                          (800U)

typedef struct st_coord
{
    uint16_t x;
    uint16_t y;
}coord_t;

typedef __PACKED_STRUCT st_st7123_point_payload
{
    uint8_t track_id;
    uint16_t x;
    uint16_t y;
    uint16_t point_size;
    uint8_t reserved;
}st7123_point_payload_t;

typedef st7123_point_payload_t gt911_point_payload_t;

fsp_err_t st7123_get_status(uint8_t* status, coord_t * points, uint32_t num_points);
fsp_err_t st7123_read_touch_controller_status(uint8_t * status);
fsp_err_t st7123_read_raw_report(uint8_t * report, uint32_t report_length);
bool st7123_touch_irq_pending_get_and_clear(void);
fsp_err_t st7123_touch_irq_init(void);
void st7123_touch_irq_print_task(void);
bool st7123_touch_get_latest(coord_t * points, uint32_t num_points, uint8_t * touch_count);

/* Latest valid touch coordinates. Updated by st7123_touch_irq_print_task(), not inside the IRQ ISR. */
extern coord_t g_st7123_touch_points[ST7123_MAX_TOUCH_POINTS];
extern volatile uint8_t g_st7123_touch_count;
extern volatile bool g_st7123_touch_updated;
extern volatile uint16_t g_st7123_touch_x;
extern volatile uint16_t g_st7123_touch_y;

/* Compatibility aliases for older code that still uses the temporary gt911/touch names. */
#define gt911_get_status                              st7123_get_status
#define gt911_read_touch_controller_status           st7123_read_touch_controller_status
#define gt911_read_raw_report                        st7123_read_raw_report
#define gt911_touch_irq_pending_get_and_clear        st7123_touch_irq_pending_get_and_clear
#define touch_irq_init                               st7123_touch_irq_init
#define touch_irq_print_task                         st7123_touch_irq_print_task
#define gt911_touch_get_latest                       st7123_touch_get_latest
#define g_touch_points                               g_st7123_touch_points
#define g_touch_count                                g_st7123_touch_count
#define g_touch_updated                              g_st7123_touch_updated
#define g_touch_x                                    g_st7123_touch_x
#define g_touch_y                                    g_st7123_touch_y

#endif /* GT911_H_ */
