/* ============================================================================
 * HardFault / BusFault / UsageFault / MemManage 现场诊断
 *
 * 通过 SEGGER RTT 输出异常寄存器与调用现场（PC/LR/栈帧），
 * 不依赖 UART 初始化，异常上下文也可安全调用（RTT 只是写内存缓冲）。
 *
 * 查看方法：SEGGER J-Link RTT Viewer 连接后即可看到输出。
 * 强符号覆盖 startup.c 中的弱引用（#pragma weak xxx = Default_Handler）。
 * ========================================================================== */
#include "SEGGER_RTT.h"
#include "hal_data.h"

static void fault_dump(const char * name, uint32_t hfsr, uint32_t cfsr,
                       uint32_t bfar, uint32_t mmfar)
{
    __disable_irq();

    SEGGER_RTT_Init();
    SEGGER_RTT_printf(0, "\r\n*** %s ***\r\n", name);
    SEGGER_RTT_printf(0, "HFSR=0x%08X CFSR=0x%08X BFAR=0x%08X MMFAR=0x%08X\r\n",
                      (unsigned int) hfsr, (unsigned int) cfsr,
                      (unsigned int) bfar, (unsigned int) mmfar);

    uint32_t msp = __get_MSP();
    uint32_t psp = __get_PSP();
    SEGGER_RTT_printf(0, "MSP=0x%08X PSP=0x%08X\r\n",
                      (unsigned int) msp, (unsigned int) psp);

    /* 异常栈帧（MSP 模式压栈）: [R0,R1,R2,R3,R12,LR,PC,xPSR] */
    uint32_t * frame = (uint32_t *) msp;
    SEGGER_RTT_printf(0, "R0 =0x%08X R1 =0x%08X\r\n", (unsigned int) frame[0], (unsigned int) frame[1]);
    SEGGER_RTT_printf(0, "R2 =0x%08X R3 =0x%08X\r\n", (unsigned int) frame[2], (unsigned int) frame[3]);
    SEGGER_RTT_printf(0, "R12=0x%08X LR =0x%08X\r\n", (unsigned int) frame[4], (unsigned int) frame[5]);
    SEGGER_RTT_printf(0, "PC =0x%08X xPSR=0x%08X\r\n", (unsigned int) frame[6], (unsigned int) frame[7]);

    while (1)
    {
        ;
    }
}

void HardFault_Handler(void)
{
    fault_dump("HARD FAULT", SCB->HFSR, SCB->CFSR, SCB->BFAR, SCB->MMFAR);
}

void BusFault_Handler(void)
{
    fault_dump("BUS FAULT", SCB->HFSR, SCB->CFSR, SCB->BFAR, SCB->MMFAR);
}

void UsageFault_Handler(void)
{
    fault_dump("USAGE FAULT", SCB->HFSR, SCB->CFSR, SCB->BFAR, SCB->MMFAR);
}

void MemManage_Handler(void)
{
    fault_dump("MEMMANAGE FAULT", SCB->HFSR, SCB->CFSR, SCB->BFAR, SCB->MMFAR);
}
