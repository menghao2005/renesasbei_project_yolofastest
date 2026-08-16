/***********************************************************************************************************************
 * File Name    : gt911.c
 * Description  : Touch helper used in mipi_dsi_ep.c.
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
#include "hal_data.h"
#include "common_utils.h"
#include "gt911.h"
#include "stdio.h"

/*******************************************************************************************************************//**
 * @addtogroup gt911
 * @{
 **********************************************************************************************************************/

/* User defined functions */
volatile bool g_irq_state;
extern fsp_err_t wait_for_i2c_event(i2c_master_event_t set_event);

/* TP_INT ISR sets this flag only. I2C read and printf are handled in the main loop. */
static volatile bool g_touch_irq_pending = false;

/* 连续读取失败计数（>=5 触发 IIC 复位自愈） */
static uint8_t g_st7123_fail_count = 0U;

/* Latest valid touch result for application use.
 * g_st7123_touch_points[0] is the first contact.
 * g_st7123_touch_count is 0 when no valid contact is available. */
coord_t g_st7123_touch_points[ST7123_MAX_TOUCH_POINTS] = {0};
volatile uint8_t g_st7123_touch_count = 0U;
volatile bool g_st7123_touch_updated = false;
volatile uint16_t g_st7123_touch_x = 0U;
volatile uint16_t g_st7123_touch_y = 0U;

static fsp_err_t st7123_i2c_read(uint16_t reg, uint8_t *buf, uint32_t len);
static fsp_err_t st7123_i2c_write(uint16_t reg, const uint8_t *buf, uint32_t len);
static void st7123_i2c_recover(void);
static uint8_t st7123_parse_report(const uint8_t *report, coord_t *points, uint32_t num_points);

/*******************************************************************************************************************//**
 *  @brief       This function is used to read touch data from the ST7123 controller.
 *  @param[out]  status      : Store status register returned over I2C.
 *  @param[out]  points      : Store co-ordinate values returned over I2C.
 *  @param[in]  num_points      : Store buffer data.
 *  @retval      FSP_SUCCESS : Upon successful operation, otherwise: failed
 **********************************************************************************************************************/
fsp_err_t st7123_get_status(uint8_t* status, coord_t * points, uint32_t num_points)
{
    uint8_t report[ST7123_REPORT_HEADER_BYTES + (ST7123_POINT_STRIDE_BYTES * ST7123_MAX_TOUCH_POINTS)];
    fsp_err_t err = FSP_SUCCESS;

    if ((NULL == status) || (NULL == points))
    {
        return FSP_ERR_ASSERTION;
    }

    memset(report, 0, sizeof(report));
    memset(points, 0, sizeof(coord_t) * num_points);

    err = st7123_i2c_read(ST7123_REG_REPORT_TABLE, report, sizeof(report));
    APP_ERR_RETURN(err, " ** ST7123 report read failed ** \r\n");

    uint8_t touch_count = st7123_parse_report(report, points, num_points);
    *status = (touch_count > 0U) ? (uint8_t)(ST7123_BUFFER_STATUS_READY | touch_count) : RESET_VALUE;

    return err;
}

/*******************************************************************************************************************//**
 *  @brief       Read ST7123 controller status register for bring-up diagnostics.
 **********************************************************************************************************************/
fsp_err_t st7123_read_touch_controller_status(uint8_t * status)
{
    if (NULL == status)
    {
        return FSP_ERR_ASSERTION;
    }

    return st7123_i2c_read(ST7123_REG_STATUS, status, 1U);
}

/*******************************************************************************************************************//**
 *  @brief       Read raw ST7123 report table bytes for coordinate format diagnostics.
 **********************************************************************************************************************/
fsp_err_t st7123_read_raw_report(uint8_t * report, uint32_t report_length)
{
    if (NULL == report)
    {
        return FSP_ERR_ASSERTION;
    }

    return st7123_i2c_read(ST7123_REG_REPORT_TABLE, report, report_length);
}

/*******************************************************************************************************************//**
 *  @brief       Return and clear the TP_INT pending flag.
 **********************************************************************************************************************/
bool st7123_touch_irq_pending_get_and_clear(void)
{
    bool pending = g_touch_irq_pending;
    g_touch_irq_pending = false;

    return pending;
}

/*******************************************************************************************************************//**
 *  @brief       Open and enable the TP_INT external interrupt.
 *
 *  @note        The e2 studio ICU stack is configured as falling-edge trigger. The touch controller pulls TP_INT low
 *               when a new report is ready. The callback only sets a flag; touch data is read later in main context.
 **********************************************************************************************************************/
fsp_err_t st7123_touch_irq_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    if (RESET_VALUE == g_external_irq_ctrl.open)
    {
        err = R_ICU_ExternalIrqOpen(&g_external_irq_ctrl, &g_external_irq_cfg);
        if ((FSP_SUCCESS != err) && (FSP_ERR_ALREADY_OPEN != err))
        {
            return err;
        }
    }

    return R_ICU_ExternalIrqEnable(&g_external_irq_ctrl);
}

/* IIC 控制器复位：触摸连续失败（总线残留/状态机卡死）时调用，Close+Open 重开。
 * IIC0 与 OV5640 共用，但 OV5640 仅在启动时配置，运行中复位安全。 */
static void st7123_i2c_recover(void)
{
    fsp_err_t close_err = R_IIC_MASTER_Close(&g_i2c_master0_ctrl);
    fsp_err_t open_err  = R_IIC_MASTER_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);
    printf("ST7123 I2C recover: close=%d open=%d\r\n", (int) close_err, (int) open_err);

    /* ST7123 软复位（GT911 协议 0x8040）：MCU 侧 Close/Open 复位不了控制器内部
     * 状态机（总线残留时读也失败）——软复位让控制器重新就绪。写失败无妨，
     * 下次读仍失败会再触发 recover。 */
    uint8_t rst = 0x01U;
    (void) st7123_i2c_write(0x8040U, &rst, 1U);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    rst = 0x00U;
    (void) st7123_i2c_write(0x8040U, &rst, 1U);
    R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);
}

/*******************************************************************************************************************//**
 *  @brief       Touch service task used by the bare-metal main loop.
 *
 *  @note        Call this function repeatedly in while(1). When TP_INT has fired, it reads the ST7123 report table,
 *               filters invalid coordinates, updates g_touch_points/g_touch_count, and prints the current contacts.
 **********************************************************************************************************************/
void st7123_touch_irq_print_task(void)
{
    uint8_t status = RESET_VALUE;
    coord_t touch_coordinates[ST7123_MAX_TOUCH_POINTS] = {0};

    if (!st7123_touch_irq_pending_get_and_clear())
    {
        return;
    }

    fsp_err_t err = st7123_get_status(&status, touch_coordinates, ST7123_MAX_TOUCH_POINTS);
    if (FSP_SUCCESS != err)
    {
        /* I2C 偶发超时（ST7123 拉伸 SCL / 总线瞬时忙）：重试两次自愈 */
        err = st7123_get_status(&status, touch_coordinates, ST7123_MAX_TOUCH_POINTS);
    }
    if (FSP_SUCCESS != err)
    {
        err = st7123_get_status(&status, touch_coordinates, ST7123_MAX_TOUCH_POINTS);
    }
    if (FSP_SUCCESS != err)
    {
        /* 连续失败（阈值 2）：IIC 控制器状态机可能卡死（总线电平残留），Close+Open 复位 */
        if (++g_st7123_fail_count >= 2U)
        {
            g_st7123_fail_count = 0U;
            st7123_i2c_recover();
        }
        if (1U == (g_st7123_fail_count % 25U))
        {
            printf("ST7123 touch read fail: %d\r\n", (int) err);
        }
        return;
    }
    g_st7123_fail_count = 0U;

    g_st7123_touch_count = 0U;
    g_st7123_touch_x = 0U;
    g_st7123_touch_y = 0U;
    memset(g_st7123_touch_points, 0, sizeof(g_st7123_touch_points));

    if (0U != (status & ST7123_BUFFER_STATUS_READY))
    {
        uint8_t touch_count = (uint8_t)(status & 0x0FU);
        if (touch_count > ST7123_MAX_TOUCH_POINTS)
        {
            touch_count = ST7123_MAX_TOUCH_POINTS;
        }

        for (uint8_t i = 0U; i < touch_count; i++)
        {
            g_st7123_touch_points[i] = touch_coordinates[i];
        }

        g_st7123_touch_count = touch_count;
        g_st7123_touch_updated = (touch_count > 0U);
        if (touch_count > 0U)
        {
            g_st7123_touch_x = g_st7123_touch_points[0].x;
            g_st7123_touch_y = g_st7123_touch_points[0].y;
        }
    }
    else
    {
        g_st7123_touch_updated = false;
    }
}

/*******************************************************************************************************************//**
 *  @brief       Copy the latest valid touch coordinates for other modules.
 *
 *  @param[out]  points       Destination buffer. Use g_st7123_touch_points directly if a copy is not required.
 *  @param[in]   num_points   Destination buffer capacity.
 *  @param[out]  touch_count  Number of copied valid contacts.
 *  @retval      true         At least one valid touch point is available.
 *  @retval      false        No valid touch point is available.
 **********************************************************************************************************************/
bool st7123_touch_get_latest(coord_t * points, uint32_t num_points, uint8_t * touch_count)
{
    uint8_t count = g_st7123_touch_count;

    if (NULL != touch_count)
    {
        *touch_count = count;
    }

    if ((NULL == points) || (0U == num_points) || (0U == count))
    {
        return false;
    }

    if (count > num_points)
    {
        count = (uint8_t) num_points;
    }

    memcpy(points, g_st7123_touch_points, sizeof(coord_t) * count);

    if (NULL != touch_count)
    {
        *touch_count = count;
    }

    return true;
}

/*******************************************************************************************************************//**
 *  @brief       Read ST7123 registers. Register addresses are 16-bit, big-endian.
 *  @retval      FSP_SUCCESS : Upon successful operation, otherwise: failed
 **********************************************************************************************************************/
static fsp_err_t st7123_i2c_read(uint16_t reg, uint8_t *buf, uint32_t len)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t reg_addr[2] =
    {
        (uint8_t)((reg >> 8) & 0xFFU),
        (uint8_t)(reg & 0xFFU),
    };

    err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master0_ctrl, ST7123_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
    APP_ERR_RETURN(err, " ** IIC MASTER SlaveAddressSet ST7123 failed ** \r\n");

    /* Use STOP between the register-address write and data read.
     * The project already uses this style for OV5640; repeated-start leaves IIC0 busy here. */
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, reg_addr, sizeof(reg_addr), false);
    APP_ERR_RETURN(err, " ** IIC MASTER_Write ST7123 register failed ** \r\n");

    err = wait_for_i2c_event(I2C_MASTER_EVENT_TX_COMPLETE);
    APP_ERR_RETURN(err, " ** I2C master ST7123 register write timeout ** \r\n");

    err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, buf, len, false);
    APP_ERR_RETURN(err, " ** IIC MASTER_Read ST7123 data failed ** \r\n");

    err = wait_for_i2c_event(I2C_MASTER_EVENT_RX_COMPLETE);
    APP_ERR_RETURN(err, " ** I2C master ST7123 data read timeout ** \r\n");

    return FSP_SUCCESS;
}

static fsp_err_t st7123_i2c_write(uint16_t reg, const uint8_t * buf, uint32_t len)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t frame[2U + 8U];

    if ((NULL == buf) || (len > 8U))
    {
        return FSP_ERR_ASSERTION;
    }

    frame[0] = (uint8_t) ((reg >> 8) & 0xFFU);
    frame[1] = (uint8_t) (reg & 0xFFU);
    memcpy(&frame[2U], buf, len);

    err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master0_ctrl, ST7123_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS != err) { return err; }
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, frame, 2U + len, false);
    if (FSP_SUCCESS != err) { return err; }
    err = wait_for_i2c_event(I2C_MASTER_EVENT_TX_COMPLETE);
    return err;
}

/*******************************************************************************************************************//**
 * @brief      Parse ST7123 report table. The first four bytes are report header; each contact uses eight bytes.
 **********************************************************************************************************************/
static uint8_t st7123_parse_report(const uint8_t *report, coord_t *points, uint32_t num_points)
{
    uint8_t touch_count = 0U;
    uint32_t max_points = (num_points < ST7123_MAX_TOUCH_POINTS) ? num_points : ST7123_MAX_TOUCH_POINTS;

    for (uint32_t i = 0U; i < ST7123_MAX_TOUCH_POINTS; i++)
    {
        const uint8_t *point = &report[ST7123_REPORT_HEADER_BYTES + (i * ST7123_POINT_STRIDE_BYTES)];

        if (0U != (point[0] & 0x80U))
        {
            uint16_t x = (uint16_t)(((point[0] & 0x3FU) << 8) | point[1]);
            uint16_t y = (uint16_t)(((point[2] & 0x3FU) << 8) | point[3]);

            /* Ignore stale/unused report slots. A real touch must be inside the 480x800 panel area. */
            if ((x < ST7123_PANEL_WIDTH) && (y < ST7123_PANEL_HEIGHT))
            {
                if (touch_count < max_points)
                {
                    points[touch_count].x = x;
                    points[touch_count].y = y;
                }

                touch_count++;
            }
        }
    }

    return touch_count;
}

/*******************************************************************************************************************//**
 * @brief      Touch IRQ callback function.
 **********************************************************************************************************************/
void external_irq_callback(external_irq_callback_args_t *p_args)
{
    if ((NULL != p_args) && (g_external_irq_cfg.channel == p_args->channel))
    {
        g_touch_irq_pending = true;
        g_irq_state = true;
    }
}
/*******************************************************************************************************************//**
 * @} (end addtogroup gt911)
 **********************************************************************************************************************/
