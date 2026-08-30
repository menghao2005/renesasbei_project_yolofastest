/***********************************************************************************************************************
 * File Name    : mipi_dsi_ep.c
 * Description  : Contains data structures and functions setup LCD used in hal_entry.c.
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
/*----------------------------------------------------------------------------------------------------------------------
 * 补充说明 : MIPI DSI + GLCDC 显示通路初始化(LCD)与辅助功能。
 *            - ST7102 面板初始化命令表(g_lcd_init_ydp430_st7102)经 mipi_dsi_push_table 下发;
 *            - glcdc_callback/mipi_dsi_callback 仅置标志/计数,业务在主循环处理;
 *            - DWT 周期计数器用于耗时测量;ST7123 触摸中断接入见 gt911.c。
 *---------------------------------------------------------------------------------------------------------------------*/
#include "gt911.h"
#include "mipi_dsi_ep.h"
#include "board_sdram.h"
#include "r_mipi_dsi.h"
#include "hal_data.h"
#include "common_utils.h"
#include "arm_mve.h"
#include "pic_show.h"
#include "graphics.h"


/*******************************************************************************************************************//**
 * @addtogroup mipi_dsi_ep
 * @{
 **********************************************************************************************************************/

uint32_t count;

/* User defined functions */
void handle_error (fsp_err_t err,  const char * err_str);
void touch_screen_reset(void);
void external_irq_callback(external_irq_callback_args_t *p_args);
void show_pic();
void show_pic2(void);

#define GLCDC_LINE_DETECT_TIMEOUT_COUNT (100000000U)
#define MIPI_DSI_POST_VIDEO_CMD_TIMEOUT_COUNT (1000000U)

/* ST7102 supplier scripts use SSD_SEND() for private register blocks.
 * Keep this as one switch so we can quickly compare Generic vs DCS long writes.
 */
#define ST7102_VENDOR_LONG_WRITE        MIPI_DSI_CMD_ID_DCS_LONG_WRITE

uint8_t read_data              = RESET_VALUE;
uint16_t period_sec           = RESET_VALUE;
volatile mipi_dsi_phy_status_t g_phy_status;
volatile mipi_dsi_video_status_t g_mipi_video_status;
volatile bool g_vsync_flag, g_message_sent = RESET_FLAG;
volatile uint32_t g_glcdc_gr1_underflow_count = 0;
volatile uint32_t g_glcdc_gr2_underflow_count = 0;
coord_t touch_coordinates[5];





/* 切换 GLCDC Layer-1 显示缓冲并等待下一 VSYS 生效(双缓冲刷新入口) */
static void lcd_show_framebuffer(uint16_t * p_framebuffer)
{
    // lcd_clean_framebuffer(p_framebuffer);
    R_GLCDC_BufferChange(&g_display_ctrl, (uint8_t *) p_framebuffer, DISPLAY_FRAME_LAYER_1);
    g_vsync_flag = RESET_FLAG;
}




/* DWT(Cycle Count)微秒级计时:供性能测量使用 */
void DWT_init();
uint32_t DWT_get_count();
void DWT_clean_count();
uint32_t DWT_count_to_us(uint32_t delta_count);

#define DWT_DEM *(uint32_t*)0xE000EDFC

void DWT_init()
{
    DWT->CTRL = 0;
    /* SCB->DEMCR(bit24 TRCENA):使能 DWT 外设 */
    DWT_DEM |= 1<<24;
    DWT->CYCCNT = 0;
    /* DWT->CTRL.bit0(CYCCNTENA):启动周期计数 */
    DWT->CTRL |= 1<<0;
}


uint32_t DWT_get_count()
{
    return DWT->CYCCNT;
}

void DWT_clean_count()
{
    DWT->CYCCNT = 0;
}

/* CPU 主频 480MHz:CYCCNT 每 480 个计数对应 1µs */
uint32_t DWT_count_to_us(uint32_t delta_count)
{
    return delta_count/480;
}


uint32_t DWT_pre_count=0, DWT_post_count=0, time_sdram_access=0;
uint32_t DWT_delta=0;

/* ST7102 + BOE 4.3 inch panel vendor initialization.
 * Ported from GX09C_ST7102+BOE4.3_2LANE_90HZ.c. The first SSD_SEND argument
 * in the vendor file is a host-side macro parameter, so it is not included
 * in the DSI payload below.
 */
const lcd_table_setting_t g_lcd_init_ydp430_st7102[] =
{
    /* Hardware reset already performed by ST7102_init_HW() before reaching here.
     * Keep this table byte-for-byte aligned with the Gitee GX09C vendor script:
     * SSD_LANE(2,0), vendor commands, Sleep Out, Display On, TE On.
     */
    /* Key sequence */
    {4,  {0x99, 0x71, 0x02, 0xA2}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {4,  {0x99, 0x71, 0x02, 0xA3}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {4,  {0x99, 0x71, 0x02, 0xA4}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},

    /* Power */
    {8,  {0xB0, 0x22, 0x57, 0x1E, 0x61, 0x2F, 0x57, 0x61}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {3,  {0xB7, 0x64, 0x64}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {3,  {0xBF, 0xB4, 0xB4}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},

    /* Gamma, 5.5 V group from vendor file */
    {38, {0xC8, 0x00, 0x00, 0x13, 0x24, 0x44, 0x00, 0x74, 0x03, 0xB8, 0x04, 0x11, 0x16, 0x08, 0x86, 0x04,
          0x21, 0xD3, 0x02, 0x10, 0x0F, 0x22, 0x4D, 0x0E, 0x90, 0x09, 0x32, 0xF0, 0x0B, 0x40, 0x0E, 0xF3,
          0x7D, 0x0E, 0xA9, 0xBF, 0x03, 0xC4}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {38, {0xC9, 0x00, 0x00, 0x13, 0x24, 0x44, 0x00, 0x74, 0x03, 0xB8, 0x04, 0x11, 0x16, 0x08, 0x86, 0x04,
          0x21, 0xD3, 0x02, 0x10, 0x0F, 0x22, 0x4D, 0x0E, 0x90, 0x09, 0x32, 0xF0, 0x0B, 0x40, 0x0E, 0xF3,
          0x7D, 0x0E, 0xA9, 0xBF, 0x03, 0xC4}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},

    /* GIP and panel timing related vendor registers */
    {7,  {0xD7, 0x10, 0x0C, 0x36, 0x19, 0x90, 0x90}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {33, {0xA3, 0x51, 0x03, 0x80, 0xCF, 0x44, 0x00, 0x00, 0x00, 0x00, 0x04, 0x78, 0x78, 0x00, 0x1A, 0x00,
          0x45, 0x05, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x02, 0x20, 0x52, 0x00, 0x05, 0x00, 0x00,
          0xFF}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {45, {0xA6, 0x02, 0x00, 0x24, 0x55, 0x35, 0x00, 0x38, 0x00, 0x78, 0x78, 0x00, 0x24, 0x55, 0x36, 0x00,
          0x37, 0x00, 0x78, 0x78, 0x02, 0xAC, 0x51, 0x3A, 0x00, 0x00, 0x00, 0x78, 0x78, 0x03, 0xAC, 0x21,
          0x00, 0x04, 0x00, 0x00, 0x78, 0x78, 0x3E, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {49, {0xA7, 0x19, 0x19, 0x00, 0x64, 0x40, 0x07, 0x16, 0x40, 0x00, 0x04, 0x03, 0x78, 0x78, 0x00, 0x64,
          0x40, 0x25, 0x34, 0x00, 0x00, 0x02, 0x01, 0x78, 0x78, 0x00, 0x64, 0x40, 0x4B, 0x5A, 0x00, 0x00,
          0x02, 0x01, 0x78, 0x78, 0x00, 0x24, 0x40, 0x69, 0x78, 0x00, 0x00, 0x00, 0x00, 0x78, 0x78, 0x00,
          0x44}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {38, {0xAC, 0x08, 0x0A, 0x11, 0x00, 0x13, 0x03, 0x1B, 0x18, 0x06, 0x1A, 0x19, 0x1B, 0x1B, 0x1B, 0x18,
          0x1B, 0x09, 0x0B, 0x10, 0x02, 0x12, 0x01, 0x1B, 0x18, 0x06, 0x1A, 0x19, 0x1B, 0x1B, 0x1B, 0x18,
          0x1B, 0xFF, 0x67, 0xFF, 0x67, 0x00}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {8,  {0xAD, 0xCC, 0x40, 0x46, 0x11, 0x04, 0x78, 0x78}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {15, {0xE8, 0x30, 0x07, 0x00, 0x94, 0x94, 0x9C, 0x00, 0xE2, 0x04, 0x00, 0x00, 0x00, 0x00, 0xEF}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {34, {0xE7, 0x8B, 0x3C, 0x00, 0x0C, 0xF0, 0x5D, 0x00, 0x5D, 0x00, 0x5D, 0x00, 0x5D, 0x00, 0xFF, 0x00,
          0x08, 0x7B, 0x00, 0x00, 0xC8, 0x6A, 0x5A, 0x08, 0x1A, 0x3C, 0x00, 0x81, 0x01, 0xCC, 0x01, 0x7F,
          0xF0, 0x22}, ST7102_VENDOR_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},

    {1,   {0x11}, MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {600, {0},    MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG,  (mipi_dsi_cmd_flag_t) 0},
    {1,   {0x29}, MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {120, {0},    MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG,  (mipi_dsi_cmd_flag_t) 0},

    /* TE OFF (0x34): In DPI video mode, TE is unnecessary.
     * Enabling TE (0x35) without host-side TE sync can cause tearing/flickering artifacts
     * because the panel's internal refresh timing conflicts with the continuous DPI video stream.
     * Use TE OFF to let the panel refresh synchronously with the incoming DPI video data. */
    {1,   {0x34}, MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {10,  {0},    MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG,  (mipi_dsi_cmd_flag_t) 0},

    {0x00, {0}, MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE, (mipi_dsi_cmd_flag_t) 0},
};




/*******************************************************************************************************************//**
 * @brief      Initialize LCD
 *
 * @param[in]  table  LCD Controller Initialization structure.
 * @retval     None.
 **********************************************************************************************************************/
void mipi_dsi_push_table (const lcd_table_setting_t *table)
{
    const lcd_table_setting_t *p_entry = table;

    while (MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE != p_entry->cmd_id)
    {
        mipi_dsi_cmd_t msg =
        {
          .channel = 0,
          .cmd_id = p_entry->cmd_id,
          .flags = p_entry->flags,
          .tx_len = p_entry->size,
          .p_tx_buffer = p_entry->buffer,
        };

        if (MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG == msg.cmd_id)
        {
            R_BSP_SoftwareDelay (p_entry->size, BSP_DELAY_UNITS_MILLISECONDS);//table->size
        }
        else
        {
            g_message_sent = false;
            /* Send a command to the peripheral device */
             R_MIPI_DSI_Command (&g_mipi_dsi0_ctrl, &msg);
            /* Wait */
            while (!g_message_sent);
        }
        p_entry++;
    }
}

#define RGB_565_RED    (0x1F << 11)
#define RGB_565_GREEN  (0x3F << 5)
#define RGB_565_BLUE   (0x1F << 0)
#define RGB_565_WHITE   (0xFFFF)
#define RGB_565_GRAY   (0x8410U)

#define act_hz 222
#define act_vz 480

//uint16_t color[4] = {RGB_565_RED, RGB_565_GREEN, RGB_565_BLUE, RGB_565_WHITE };
//uint16_t color[4+8] = {RGB_565_RED,RGB_565_GREEN,RGB_565_BLUE,RGB_565_WHITE,0xe6d7, 0xcee7,0xe5c0,0xd1cf,0x67FC,0xCE7F,0xFEA0,0xC618 } ;
uint16_t color[4+8] = {RGB_565_RED,RGB_565_GREEN,RGB_565_BLUE,RGB_565_WHITE,0xe6d7, 0xcee7,0xe5c0,0xd1cf,0x67FC,0xCE7F,0xFEA0,0xC618 };
uint32_t color_32bit[4] = {0xffffffff,0xffff0000,0xff00ff00,0xff0000ff};
typedef enum
{
    simple = 0,
    partition = 1,
    gradient = 2,
    partition_shift_probe = 3,
    video_path_probe = 4
} color_pattern_t;


color_pattern_t color_p = simple;

//uint16_t color_temp;
/* 整屏填充指定 RGB565 颜色(测试图案) */
void show_RGB(uint8_t R, uint8_t G, uint8_t B);
void show_RGB(uint8_t R, uint8_t G, uint8_t B)
{
    uint16_t * p = (uint16_t *)&g_display_sdram[0][0];
    uint16_t color_temp;
    color_temp = (uint16_t)(((R&0x1F)<<11)|((G&0x3F)<<5)|(B&0x1F));

    for(uint32_t x=0;x<g_vr_size;x++)
    {
        for(uint32_t y=0;y<g_hz_size;y++)
        {
            p[y+x*g_hstride] = color_temp;
        }
        //color_temp+=0x1863;
    }

}

/* 整屏灰度渐变测试图案(每行灰度步进 0x0821) */
void show_GRAY();
void show_GRAY()
{
    uint16_t * p = (uint16_t *)&g_display_sdram[0][0];
    uint16_t color_temp;
    color_temp = (uint16_t)((((0x1F/8)&0x1F)<<11)|(((0x3F/8)&0x3F)<<5)|((0x1F/8)&0x1F));

    for(uint32_t x=0;x<g_vr_size;x++)
    {
        for(uint32_t y=0;y<g_hz_size;y++)
        {
            p[y+x*g_hstride] = color_temp;
        }
        color_temp+=0x0821;
    }

}


/* 居中显示图片 gImage_qier(222x480,RGB565),四周背景填白后切换缓冲 */
void show_pic()
{
    show_pic2();
}

void show_pic2(void)
{
    const uint32_t pic_w = 222;
    const uint32_t pic_h = 480;

    uint16_t * p = (uint16_t *)&g_display_sdram[0][0];

    uint32_t x_offset = (g_hz_size - pic_w) / 2;
    uint32_t y_offset = (g_vr_size - pic_h) / 2;

    for (uint32_t y = 0; y < g_vr_size; y++)
    {
        for (uint32_t x = 0; x < g_hstride; x++)
        {
            p[y * g_hstride + x] = RGB_565_WHITE;
        }
    }

    for (uint32_t y = 0; y < pic_h; y++)
    {
        for (uint32_t x = 0; x < pic_w; x++)
        {
            uint32_t src_index = ((y * pic_w) + x) * BYTES_PER_PIXEL;
            uint16_t pixel = (uint16_t) (gImage_qier[src_index] | ((uint16_t) gImage_qier[src_index + 1] << 8));

            p[(y + y_offset) * g_hstride + (x + x_offset)] = pixel;
        }
    }

    lcd_show_framebuffer(p);
}

/*******************************************************************************************************************//**
 * @brief      This function is used to reset the LCD after power on.
 *
 * @param[in]  none
 * @retval     none
 **********************************************************************************************************************/
void touch_screen_reset(void)
{
     /* Reset touch chip by setting GPIO reset pin low. */
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_13, BSP_IO_LEVEL_HIGH);
     R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_MICROSECONDS);
     R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_13, BSP_IO_LEVEL_LOW);
     R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_MICROSECONDS);
     R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_13, BSP_IO_LEVEL_HIGH);
     R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);

}

/*******************************************************************************************************************//**
 *  @brief       This function handles errors, closes all opened modules, and prints errors.
 *
 *  @param[in]   err       error status
 *  @param[in]   err_str   error string
 *  @retval      None
 **********************************************************************************************************************/
void handle_error (fsp_err_t err,  const char * err_str)
{
    if(FSP_SUCCESS != err)
    {
        /* Print the error */
        APP_ERR_PRINT(err_str);

        /* Close opened GLCD module*/
        if(RESET_VALUE != g_display_ctrl.state)
        {

            if(FSP_SUCCESS != R_GLCDC_Close (&g_display_ctrl))
            {
                APP_ERR_PRINT("GLCDC Close API failed\r\n");
            }

        }

        /* Close opened ICU module*/
        if(RESET_VALUE != g_external_irq_ctrl.open)
        {
            if(FSP_SUCCESS != R_ICU_ExternalIrqClose (&g_external_irq_ctrl))
            {
                APP_ERR_PRINT("ICU ExternalIrqClose API failed\r\n");
            }
        }

        /* Close opened IIC master module*/
        if(RESET_VALUE != g_i2c_master0_ctrl.open)
        {
            if(FSP_SUCCESS != R_IIC_MASTER_Close(&g_i2c_master0_ctrl))
            {
                APP_ERR_PRINT("IIC MASTER Close API failed\r\n");
            }
        }

        APP_ERR_TRAP(err);
    }
}

/* 本工程自定义的电平翻转:先读引脚电平再写反相电平(供触摸中断翻转调试引脚 P110) */
void R_IOPORT_PinToggle(ioport_ctrl_t * p_ctrl, bsp_io_port_pin_t pin)
{
    bsp_io_level_t level;

    R_IOPORT_PinRead(p_ctrl, pin, &level);
    R_IOPORT_PinWrite(p_ctrl, pin,(level == BSP_IO_LEVEL_LOW) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}
/*******************************************************************************************************************//**
 * @brief      Callback functions for GLCDC interrupts
 *
 * @param[in]  p_args    Callback arguments
 * @retval     none
 **********************************************************************************************************************/
void glcdc_callback (display_callback_args_t * p_args)
{
    if (DISPLAY_EVENT_LINE_DETECTION == p_args->event)
    {
        g_vsync_flag = SET_FLAG;

//        R_IOPORT_PinToggle(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10);
    }
    else if (DISPLAY_EVENT_GR1_UNDERFLOW == p_args->event)
    {
        g_glcdc_gr1_underflow_count++;
    }
    else if (DISPLAY_EVENT_GR2_UNDERFLOW == p_args->event)
    {
        g_glcdc_gr2_underflow_count++;
    }
}

/*******************************************************************************************************************//**
 * @brief      Callback functions for MIPI DSI interrupts
 *
 * @param[in]  p_args    Callback arguments
 * @retval     none
 **********************************************************************************************************************/
void mipi_dsi_callback(mipi_dsi_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case MIPI_DSI_EVENT_SEQUENCE_0:
        {
            if (MIPI_DSI_SEQUENCE_STATUS_DESCRIPTORS_FINISHED == p_args->tx_status)
            {
                g_message_sent = SET_FLAG;
            }
            break;
        }
        case MIPI_DSI_EVENT_PHY:
        {
            g_phy_status |= p_args->phy_status;
            break;
        }
        case MIPI_DSI_EVENT_VIDEO:
        {
            g_mipi_video_status |= p_args->video_status;
            break;
        }
        default:
        {
            break;
        }

    }
}

uint16_t color_dtcm[1]  = {RGB_565_RED};



/* ST7102(LCD)硬件复位:按面板手册时序操作复位引脚 P000(高→低→高脉冲) */
void ST7102_init_HW();
void ST7102_init_HW()
{
    R_BSP_PinAccessEnable();
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(4, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_HIGH);

}

/*******************************************************************************************************************//**
 * @brief      This function is used initialize related module and start display operation.
 *
 * @param[in]  none
 * @retval     none
 **********************************************************************************************************************/
void mipi_dsi_start_display();
void mipi_dsi_entry(void)
{
    g_hz_size = (g_display_cfg.input[0].hsize);// 显示屏水平分辨率（如800/854）
    g_vr_size = (g_display_cfg.input[0].vsize);// 显示屏垂直分辨率（如480/480）
    g_hstride = (g_display_cfg.input[0].hstride);// 行跨度（每行字节数，内存对齐用

    ST7102_init_HW();//LCD复位  LCD芯片硬件复位（GPIO电平操作复位引脚）

    // DWT_init();
    // DWT_clean_count();
    // DWT_pre_count = DWT_get_count();

    /* Initialize GLCDC module */
    R_GLCDC_Open(&g_display_ctrl, &g_display_cfg);
    /* LCD reset */
//    touch_screen_reset();// 触摸屏+LCD额外复位


    /* Initialize LCD. */
    mipi_dsi_push_table(g_lcd_init_ydp430_st7102);
    /*
     * R_GLCDC_Open opens DSI and allows LP commands. Start video only after
     * ST7102 has completed Sleep Out/Display On to avoid a stale video phase.
     */
    R_GLCDC_Start(&g_display_ctrl);
    /* st7102_post_video_relock() removed: all commands (COLMOD, MADCTL, Display On)
     * are already sent in g_lcd_init_ydp430_st7102[] table. Sending DCS commands
     * during active video mode can corrupt the DSI video stream. */

    /* Initialize ICU module */
    R_ICU_ExternalIrqOpen(&g_external_irq_ctrl, &g_external_irq_cfg);// ICU：外部中断（触摸屏中断、MIPI DSI事件）
    /* Handle error */
    /* Start display 8-color bars */
    mipi_dsi_start_display();// 启动GLCDC显示，渲染8色条画面（对应display_draw函数）

}



/*******************************************************************************************************************//**
 * @brief      Start video mode and draw color bands on the LCD screen
 *
 * @param[in]  None.
 * @retval     None.
 **********************************************************************************************************************/
 void mipi_dsi_start_display(void)
{
    uint16_t * const p    = (uint16_t *)&g_display_sdram[0][0];
    
    lcd_show_framebuffer(p);

    /* Initialize buffer pointers */
    /* Enable external interrupt */
//    R_ICU_ExternalIrqEnable(&g_external_irq_ctrl);//9. 使能外部中断（如触摸中断）
    /* Handle error */

     g_vsync_flag = RESET_FLAG;


}

/*******************************************************************************************************************//**
 * @brief      Minimal test: fill screen black, draw a red square at center.
 *             Call after mipi_dsi_entry() to verify framebuffer→GLCDC→MIPI→panel path.
 **********************************************************************************************************************/
void test_single_pixel(void)
{
    uint16_t *p = (uint16_t *)&g_display_sdram[0][0];

    /* 1. Fill screen black */
    for (uint32_t y = 0; y < g_vr_size; y++)//800
        for (uint32_t x = 0; x < g_hz_size; x++)//480
            p[y * g_hz_size + x] = 0x0000;

    /* 2. Draw 20×20 red square at center */
    const uint32_t half = 5;
    for (uint32_t y = g_vr_size / 2 - half; y < g_vr_size / 2 + half; y++)
        for (uint32_t x = g_hz_size / 2 - half; x < g_hz_size / 2 + half; x++)
            p[y * g_hz_size + x] = 0xF800;  /* RGB565 pure red */

    /* 3. Push and wait for VSync */
    lcd_show_framebuffer(p);
}

/*******************************************************************************************************************//**
 * @} (end addtogroup mipi_dsi_ep)
 **********************************************************************************************************************/
