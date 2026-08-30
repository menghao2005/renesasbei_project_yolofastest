/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*----------------------------------------------------------------------------------------------------------------------
 * 文件说明  : 板级 SDRAM 初始化与自检。
 *             - bsp_sdram_init:直接操作 BUS(SDRAMC)寄存器完成 SDR SDRAM 上电初始化
 *               (行地址偏移 → 初始化序列 → 模式寄存器 LMR → 时序 → 自动刷新 → 开放访问);
 *             - SDRAMReadWrite32Bit_test:SRAM→SDRAM 32 位读写回环自检(带地址/寄存器诊断打印)。
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/

#include "r_ioport.h"
#include "bsp_cfg.h"
#include "bsp_pin_cfg.h"
#include "Uart9_Debug.h"
#include "stdio.h"
#include "board_sdram.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/* SDRAM size, in bytes */
#define SDRAM_SIZE                             (4096 * 256 * 4 * 4)
//4096行 256列 4个区 每个空间四个字节
/*
 * Set ACTIVE-to-PRECHARGE command (tRAS) timing
 * e.g. tRAS = 42ns -> 6cycles are needed at SDCLK 120MHz
 *      tRAS = 37ns -> 5cycles are needed at SDCLK 120MHz
 */
#define BSP_PRV_SDRAM_TRAS                     (6U)

/*
 * Set ACTIVE-to-READ or WRITE delay tRCD (tRCD) timing
 * e.g. tRCD = 18ns -> 3cycles are needed at SDCLK 120MHz
 *      tRCD = 15ns -> 2cycles are needed at SDCLK 120MHz
 */
#define BSP_PRV_SDRAM_TRCD                     (3U)

/*
 * Set PRECHARGE command period (tRP) timing
 * e.g. tRP  = 18ns -> 3cycles are needed at SDCLK 120MHz
 *      tRP  = 15ns -> 2cycles are needed at SDCLK 120MHz
 */
#define BSP_PRV_SDRAM_TRP                      (3U)

/*
 * Set WRITE recovery time (tWR) timing
 * e.g. tWR  = 1CLK + 6ns -> 2cycles are needed at SDCLK 120MHz
 *      tWR  = 1CLK + 7ns -> 2cycles are needed at SDCLK 120MHz
 */
#define BSP_PRV_SDRAM_TWR                      (2U)

/*
 * Set CAS (READ) latency (CL) timing
 * e.g. CL = 18ns -> 3cycles are needed at SDCLK 120MHz
 * e.g. CL = 15ns -> 2cycles are needed at SDCLK 120MHz
 */
#define BSP_PRV_SDRAM_CL                       (3U)

/*
 * Set AUTO REFRESH period (tRFC) timing
 * e.g. tRFC = 60nS -> 8cycles are needed at SDCLK 120MHz
 *      tRFC = 66nS -> 8cycles are needed at SDCLK 120MHz
 */
#define BSP_PRV_SDRAM_TRFC                     (8U)

/*
 * Set Average Refresh period
 * e.g. tREF = 64ms/8192rows -> 7.8125us/each row.  937cycles are needed at SDCLK 120MHz
 */
#define BSP_PRV_SDRAM_REF_CMD_INTERVAL         (937U)

/*
 * Set Auto-Refresh issue times in initialization sequence needed for SDRAM device
 * Typical SDR SDRAM device needs twice of Auto-Refresh command issue
 */
#define BSP_PRV_SDRAM_SDIR_REF_TIMES           (2U)

/*
 * Set RAW address offset
 * Available settings are
 * 8  : 8-bit
 * 9  : 9-bit
 * 10 : 10-bit
 * 11 : 11-bit
 */
#define BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET    8

/*
 * Select endian mode for SDRAM address space
 * 0 : Endian of SDRAM address space is the same as the endian of operating mode
 * 1 : Endian of SDRAM address space is not the endian of operating mode
 */
#define BSP_PRV_SDRAM_ENDIAN_MODE              (0U)

/*
 * Select access mode
 * Typically Continuous access should be enabled to get better SDRAM bandwidth
 * 0: Continuous access is disabled
 * 1: Continuous access is enabled
 */
#define BSP_PRV_SDRAM_CONTINUOUS_ACCESSMODE    (1U)

/*
 * Select bus width
 * 0: 16-bit
 * 1: 32-bit
 * 2: 8-bit
 */
#define BSP_PRV_SDRAM_BUS_WIDTH                1

#if ((BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET != 8U) && (BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET != 9U) \
    && (BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET != 10U) && (BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET != 11U))
 #error "BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET must be either of 8,9,10 or 11"
#endif

#if ((BSP_PRV_SDRAM_BUS_WIDTH != 0) && (BSP_PRV_SDRAM_BUS_WIDTH != 1U) && (BSP_PRV_SDRAM_BUS_WIDTH != 2U))
 #error "BSP_PRV_SDRAM_BUS_WIDTH must be either of 0(16-bit) or 1(32-bit) or 2(8-bit)"
#endif

#if ((BSP_PRV_SDRAM_ENDIAN_MODE != 0) && (BSP_PRV_SDRAM_ENDIAN_MODE != 1))
 #error \
    "BSP_PRV_SDRAM_ENDIAN_MODE must be either of 0(same endian as operating mode) or 2(another endian against operating mode)"
#endif

#if ((BSP_PRV_SDRAM_CONTINUOUS_ACCESSMODE != 0) && (BSP_PRV_SDRAM_CONTINUOUS_ACCESSMODE != 1))
 #error \
    "BSP_PRV_SDRAM_CONTINUOUS_ACCESSMODE must be either of 0(continuous access is disabled) or 1(continuous access is enabled)"
#endif

#define BSP_PRV_SDRAM_MR_WB_SINGLE_LOC_ACC    (1U) /* MR.M9                : Single Location Access */
#define BSP_PRV_SDRAM_MR_OP_MODE              (0U) /* MR.M8:M7             : Standard Operation */
#define BSP_PRV_SDRAM_MR_BT_SEQUENCTIAL       (0U) /* MR.M3 Burst Type     : Sequential */
#define BSP_PRV_SDRAM_MR_BURST_LENGTH         (0U) /* MR.M2:M0 Burst Length: 0(1 burst) */

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 **********************************************************************************************************************/

/* SDR SDRAM 控制器初始化:按硬件手册顺序配置 SDADR/SDIR/SDICR/SDCCR/SDMOD/SDTR/SDRFCR;
 * 各时序宏按 120MHz SDCLK 折算为周期数,见上方宏定义区 */
void bsp_sdram_init (void)
{
    /** Set row address offset BEFORE init sequence (SDADR may be locked after INIT) */
    R_BUS->SDRAM.SDADR_b.MXC = BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET - 8U;

    /** Setting for SDRAM initialization sequence */
#if (BSP_PRV_SDRAM_TRP < 3)
    R_BUS->SDRAM.SDIR_b.PRC = 3U;
#else
    R_BUS->SDRAM.SDIR_b.PRC = BSP_PRV_SDRAM_TRP - 3U;
#endif

    while (R_BUS->SDRAM.SDSR)
    {
        /* According to h/w maual, need to confirm that all the status bits in SDSR are 0 before SDIR modification. */
    }

    R_BUS->SDRAM.SDIR_b.ARFC = BSP_PRV_SDRAM_SDIR_REF_TIMES;

    while (R_BUS->SDRAM.SDSR)
    {
        /* According to h/w maual, need to confirm that all the status bits in SDSR are 0 before SDIR modification. */
    }

#if (BSP_PRV_SDRAM_TRFC < 3)
    R_BUS->SDRAM.SDIR_b.ARFI = 0U;
#else
    R_BUS->SDRAM.SDIR_b.ARFI = BSP_PRV_SDRAM_TRFC - 3U;
#endif

    while (R_BUS->SDRAM.SDSR)
    {
        /* According to h/w maual, need to confirm that all the status bits in SDSR are 0 before SDICR modification. */
    }

    /** Start SDRAM initialization sequence.
     * Following operation is automatically done when set SDICR.INIRQ bit.
     * Perform a PRECHARGE ALL command and wait at least tRP time.
     * Issue an AUTO REFRESH command and wait at least tRFC time.
     * Issue an AUTO REFRESH command and wait at least tRFC time.
     */
    R_BUS->SDRAM.SDICR_b.INIRQ = 1U;
    while (R_BUS->SDRAM.SDSR_b.INIST)
    {
        /* Wait the end of initialization sequence. */
    }

    /** Setting for SDRAM controller */
    R_BUS->SDRAM.SDCCR_b.BSIZE  = BSP_PRV_SDRAM_BUS_WIDTH;             /* set SDRAM bus width */
    R_BUS->SDRAM.SDAMOD_b.BE    = BSP_PRV_SDRAM_CONTINUOUS_ACCESSMODE; /* enable continuous access */
    R_BUS->SDRAM.SDCMOD_b.EMODE = BSP_PRV_SDRAM_ENDIAN_MODE;           /* set endian mode for SDRAM address space */

    while (R_BUS->SDRAM.SDSR)
    {
        /* According to h/w maual, need to confirm that all the status bits in SDSR are 0 before SDMOD modification. */
    }

    /** Using LMR command, program the mode register */
    R_BUS->SDRAM.SDMOD = ((((uint16_t) (BSP_PRV_SDRAM_MR_WB_SINGLE_LOC_ACC << 9) |
                            (uint16_t) (BSP_PRV_SDRAM_MR_OP_MODE << 7)) |
                           (uint16_t) (BSP_PRV_SDRAM_CL << 4)) |
                          (uint16_t) (BSP_PRV_SDRAM_MR_BT_SEQUENCTIAL << 3)) |
                         (uint16_t) (BSP_PRV_SDRAM_MR_BURST_LENGTH << 0);

    /** wait at least tMRD time */
    while (R_BUS->SDRAM.SDSR_b.MRSST)
    {
        /* Wait until Mode Register setting done. */
    }

    /** Set timing parameters for SDRAM */
    R_BUS->SDRAM.SDTR_b.RAS = BSP_PRV_SDRAM_TRAS - 1U; /* set ACTIVE-to-PRECHARGE command cycles*/
    R_BUS->SDRAM.SDTR_b.RCD = BSP_PRV_SDRAM_TRCD - 1U; /* set ACTIVE to READ/WRITE delay cycles */
    R_BUS->SDRAM.SDTR_b.RP  = BSP_PRV_SDRAM_TRP - 1U;  /* set PRECHARGE command period cycles */
    R_BUS->SDRAM.SDTR_b.WR  = BSP_PRV_SDRAM_TWR - 1U;  /* set write recovery cycles */
    R_BUS->SDRAM.SDTR_b.CL  = BSP_PRV_SDRAM_CL;        /* set SDRAM column latency cycles */

    /* MXC already set before init sequence (above) */

    R_BUS->SDRAM.SDRFCR_b.REFW = (uint16_t) (BSP_PRV_SDRAM_TRFC - 1U); /* set Auto-Refresh issuing cycle */
    R_BUS->SDRAM.SDRFCR_b.RFC  = BSP_PRV_SDRAM_REF_CMD_INTERVAL - 1U;  /* set Auto-Refresh period */

    /** Start Auto-refresh */
    R_BUS->SDRAM.SDRFEN_b.RFEN = 1U;

    /** Enable SDRAM access */
    R_BUS->SDRAM.SDCCR_b.EXENB = 1U;
}



#define SDRAM_EXAMPLE_DATALEN    1024

volatile uint32_t SRAM_write_buff_Cache[SDRAM_EXAMPLE_DATALEN];
volatile uint32_t SRAM_read_buff_Cache[SDRAM_EXAMPLE_DATALEN];


/*数组加上BSP_PLACE_IN_SECTION(".sdram_nocache")存放在sdram里面，注意sdram区的地址0x68000000开始*/
volatile uint32_t sdram_cache[SDRAM_EXAMPLE_DATALEN] BSP_PLACE_IN_SECTION(".sdram_nocache"); 
volatile uint32_t sdram_nocache[SDRAM_EXAMPLE_DATALEN]  BSP_PLACE_IN_SECTION(".sdram_nocache");


/*******************************************************************************
 *
 ******************************************************************************/
#define EXAMPLE_SDRAM_START_ADDRESS (0x68000000U)
#define EXAMPLE_DTCM_START_ADDRESS  (0x20000000U) //Data Tightly Coupled Memory Cortex-M 的"CPU 私藏高速 RAM


uint8_t timer1s_flag = 0;
/* Callback function */



uint32_t sdram_write_count = 0;
/* SRAM<--->SDRAM 读写测试 */
void SDRAMReadWrite32Bit_test(void)
{
       uint32_t index;
       uint32_t datalen = SDRAM_EXAMPLE_DATALEN ;
       uint32_t first_err_count = 0;

       /* Print variable addresses for diagnosis */
       DBG_LOG("==== ADDRESS DUMP ====\r\n");
       DBG_LOG("&SRAM_write_buff_Cache = 0x%08lX\r\n", (unsigned long)(uintptr_t)SRAM_write_buff_Cache);
       DBG_LOG("&SRAM_read_buff_Cache  = 0x%08lX\r\n", (unsigned long)(uintptr_t)SRAM_read_buff_Cache);
       DBG_LOG("&sdram_cache           = 0x%08lX\r\n", (unsigned long)(uintptr_t)sdram_cache);
       DBG_LOG("&sdram_nocache         = 0x%08lX\r\n", (unsigned long)(uintptr_t)sdram_nocache);
       DBG_LOG("sizeof(sdram_cache)=%lu bytes\r\n", (unsigned long)sizeof(sdram_cache));
       DBG_LOG("sdram_cache end = 0x%08lX\r\n", (unsigned long)(uintptr_t)(&sdram_cache[datalen]));
       DBG_LOG("SDADR (MXC) register = 0x%02X (expected MXC=%lu)\r\n",
              (unsigned)R_BUS->SDRAM.SDADR,
              (unsigned long)(BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET - 8U));
       DBG_LOG("======================\r\n");

       DBG_LOG("==== SDRAM test start, datalen=%lu words (=%lu bytes) ====\r\n",
              (unsigned long)datalen, (unsigned long)datalen * 4);

       DBG_LOG("[A] direct write+readback test on sdram_cache[0] and [256]...\r\n");

       /* Test 1: write known pattern to sdram_cache[0] and read back */
       sdram_cache[0] = 0xDEADBEEF;
       sdram_cache[1] = 0x12345678;
       sdram_cache[256] = 0xAABBCCDD;
       sdram_cache[257] = 0x11223344;

       DBG_LOG("  Wrote sdram_cache[0]=0xDEADBEEF,  [1]=0x12345678\r\n");
       DBG_LOG("  Wrote sdram_cache[256]=0xAABBCCDD, [257]=0x11223344\r\n");
       DBG_LOG("  Read sdram_cache[0]=0x%08lX (expect 0xDEADBEEF)\r\n", (unsigned long)sdram_cache[0]);
       DBG_LOG("  Read sdram_cache[1]=0x%08lX (expect 0x12345678)\r\n", (unsigned long)sdram_cache[1]);
       DBG_LOG("  Read sdram_cache[256]=0x%08lX (expect 0xAABBCCDD)\r\n", (unsigned long)sdram_cache[256]);
       DBG_LOG("  Read sdram_cache[257]=0x%08lX (expect 0x11223344)\r\n", (unsigned long)sdram_cache[257]);

       /* Clear for main test */
       sdram_cache[0] = 0;
       sdram_cache[1] = 0;
       sdram_cache[256] = 0;
       sdram_cache[257] = 0;

       DBG_LOG("[B] filling SRAM buffer with sequential pattern...\r\n");
       for (index = 0; index < datalen; index++)
       {
           SRAM_write_buff_Cache[index] = index;
       }
       DBG_LOG("[B] OK\r\n");

       DBG_LOG("[C] writing to sdram_cache...\r\n");
       for (index = 0; index < datalen; index++)
       {
           sdram_cache[index] = SRAM_write_buff_Cache[index];
       }
       DBG_LOG("[C] OK\r\n");

       DBG_LOG("[D] reading back from sdram_cache...\r\n");
       for (index = 0; index < datalen; index++)
       {
           SRAM_read_buff_Cache[index] = sdram_cache[index];
       }
       DBG_LOG("[D] OK\r\n");

       DBG_LOG("[E] verifying (first 10 errors)...\r\n");
       index = 0;
       while(index<datalen && first_err_count < 10)
       {
           if(SRAM_read_buff_Cache[index] != SRAM_write_buff_Cache[index])
           {
               DBG_LOG("  FAIL[%lu]: write_idx=%lu val=0x%08lX  read_idx=%lu val=0x%08lX (delta_idx=%ld)\r\n",
                      (unsigned long)first_err_count,
                      (unsigned long)index,
                      (unsigned long)SRAM_write_buff_Cache[index],
                      (unsigned long)index,
                      (unsigned long)SRAM_read_buff_Cache[index],
                      (long)((long)SRAM_read_buff_Cache[index] - (long)index));
               first_err_count++;
           }
           index++;
       }

       if (first_err_count == 0)
       {
           DBG_LOG("**** SDRAM test PASS (all %lu words match) ****\r\n", (unsigned long)datalen);
       }
       else
       {
           DBG_LOG("**** SDRAM test FAILED: %lu mismatches found in first %lu words ****\r\n",
                  (unsigned long)first_err_count, (unsigned long)index);
           while(1) { ; }
       }
}
