/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include "common_utils.h"
#include "camera_i2c_api.h"

static volatile i2c_master_event_t i2c_master_event = 0;
fsp_err_t wait_for_i2c_event (i2c_master_event_t set_event);
void g_i2c_master_callback(i2c_master_callback_args_t *p_args);

void camera_i2c_comm_event_clear(void)
{
    i2c_master_event = 0;
}

static void camera_i2c_comm_abort_and_clear(void)
{
    (void) R_IIC_MASTER_Abort(&g_i2c_master0_ctrl);
    i2c_master_event = 0;
}

fsp_err_t wait_for_i2c_event (i2c_master_event_t set_event)
{
    /* 超时 = ICLK/25 周期 ≈ 40ms（原 /10 ≈ 100ms：触摸失败时主循环卡 100ms 太痛）。
     * I2C 事务本身 <5ms，40ms 对 OV5640 初始化与触摸读都足够。 */
    uint32_t timeout = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_ICLK) / 10;
    i2c_master_event_t get_event;

    do
    {
        get_event = i2c_master_event;
        if(set_event == get_event)
        {
            i2c_master_event = 0;
            return FSP_SUCCESS;
        }

        if(I2C_MASTER_EVENT_ABORTED == get_event)
        {
            i2c_master_event = 0;
            return FSP_ERR_TRANSFER_ABORTED;
        }
    }while(timeout--);

    return FSP_ERR_TIMEOUT;
}

/*******************************************************************************************************************//**
 * @brief      Callback functions for i2c master interrupts
 *
 * @param[in]  p_args    Callback arguments
 * @retval     none
 **********************************************************************************************************************/
void g_i2c_master_callback(i2c_master_callback_args_t *p_args)
{
    if (NULL != p_args)
    {
        i2c_master_event = p_args->event;
    }
}


fsp_err_t camera_i2c_comm_write(uint32_t sub_address, uint32_t sub_address_length, const uint8_t *data, size_t data_length)
{
    fsp_err_t err = FSP_SUCCESS;
    size_t i;
    size_t buffer_length = sub_address_length + data_length;
    uint8_t buffer[buffer_length];
    i = 0;

    // Calculate Check sub-address length and copy it into the buffer
    if(sub_address_length == 1)
    {
        buffer[i++] = sub_address & 0xFF;
    }
    else if (sub_address_length == 2)
    {
        buffer[i++] = (sub_address >> 8 ) & 0xFF;
        buffer[i++] = sub_address & 0xFF;
    }
    else if (sub_address_length == 4)
    {
        buffer[i++] = (uint8_t) ((sub_address >> 24) & 0xFF);
        buffer[i++] = (uint8_t) ((sub_address >> 16) & 0xFF);
        buffer[i++] = (uint8_t) ((sub_address >> 8) & 0xFF);
        buffer[i++] = (uint8_t) (sub_address & 0xFF);
    }
    else
    {
#if USE_DEBUG_BREAKPOINTS
        __BKPT(0);
#endif
    }

    // Add the data to the buffer
    memcpy(buffer + i, data, data_length);

    camera_i2c_comm_event_clear();

    // Write I2C data
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, buffer, buffer_length, false);
    if (FSP_SUCCESS != err)
    {
        camera_i2c_comm_abort_and_clear();
        return err;
    }

    /* Wait until write transmission complete */
    err = wait_for_i2c_event (I2C_MASTER_EVENT_TX_COMPLETE);
    if (FSP_SUCCESS != err)
    {
        camera_i2c_comm_abort_and_clear();
        return err;
    }

    return err;
}

fsp_err_t camera_i2c_comm_read(uint32_t sub_address, uint32_t sub_address_length, uint8_t *data, size_t data_length)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t buffer[4];

    // Check sub-address length and format the sub_address data accordingly
    if(sub_address_length == 1)
    {
        buffer[0] = sub_address & 0xFF;
    }
    else if (sub_address_length == 2)
    {
        buffer[0] = (sub_address >> 8 ) & 0xFF;
        buffer[1] = sub_address & 0xFF;
    }
    else if (sub_address_length == 4)
    {
        buffer[0] = (uint8_t) ((sub_address >> 24) & 0xFF);
        buffer[1] = (uint8_t) ((sub_address >> 16) & 0xFF);
        buffer[2] = (uint8_t) ((sub_address >> 8) & 0xFF);
        buffer[3] = (uint8_t) (sub_address & 0xFF);
    }
    else
    {
#if USE_DEBUG_BREAKPOINTS
        __BKPT(0);
#endif
    }

    // Write register index
//    err = R_IIC_MASTER_Write(&g_i2c_master_ctrl, &buffer[0], sub_address_length, true);   //Restart for OV5640
//    APP_ERR_RETURN(err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");

    // Write register index
    camera_i2c_comm_event_clear();
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &buffer[0], sub_address_length, false);   //Stop for OV7725
    if (FSP_SUCCESS != err)
    {
        camera_i2c_comm_abort_and_clear();
        return err;
    }

    /* Wait until write transmission complete */
    err = wait_for_i2c_event (I2C_MASTER_EVENT_TX_COMPLETE);
    if (FSP_SUCCESS != err)
    {
        camera_i2c_comm_abort_and_clear();
        return err;
    }

    // Read data
    camera_i2c_comm_event_clear();
    err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, data, data_length, false);
    if (FSP_SUCCESS != err)
    {
        camera_i2c_comm_abort_and_clear();
        return err;
    }

    /* Wait until read transmission complete */
    err = wait_for_i2c_event (I2C_MASTER_EVENT_RX_COMPLETE);
    if (FSP_SUCCESS != err)
    {
        camera_i2c_comm_abort_and_clear();
        return err;
    }

    return err;
}
