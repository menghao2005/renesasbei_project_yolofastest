/* ============================================================================
 * HardFault / BusFault / UsageFault / MemManage 现场诊断（naked 增强版）
 *
 * 通过 SEGGER RTT 输出异常寄存器与调用现场（PC/LR/栈帧）。
 * naked handler：异常入栈后【立即】捕获 MSP/PSP 与 EXC_RETURN，
 * 避免普通 C 函数 prologue 污染栈指针，保证 frame[0..7] 是真异常现场。
 * 额外 dump 现场下方 16 字（异常前调用者栈，含返回地址链）。
 * 强符号覆盖 startup.c 中的弱引用（#pragma weak xxx = Default_Handler）。
 * ========================================================================== */
#include "SEGGER_RTT.h"
#include "hal_data.h"

void fault_entry(uint32_t * frame, uint32_t exc_return);

/* ---- naked 入口：r0 = 异常栈帧指针（MSP 或 PSP），r1 = EXC_RETURN ---- */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "b fault_entry\n"
    );
}

__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "b fault_entry\n"
    );
}

__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "b fault_entry\n"
    );
}

__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile (
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "b fault_entry\n"
    );
}

/* ---- C 处理：打印现场 + 调用链 ---- */
void fault_entry(uint32_t * frame, uint32_t exc_return)
{
    __disable_irq();

    uint32_t hfsr = SCB->HFSR;
    uint32_t cfsr = SCB->CFSR;
    uint32_t bfar = SCB->BFAR;
    uint32_t mmfar = SCB->MMFAR;

    SEGGER_RTT_Init();
    SEGGER_RTT_printf(0, "\r\n*** FAULT (naked) ***\r\n");
    SEGGER_RTT_printf(0, "HFSR=0x%08X CFSR=0x%08X BFAR=0x%08X MMFAR=0x%08X\r\n",
                      (unsigned int) hfsr, (unsigned int) cfsr,
                      (unsigned int) bfar, (unsigned int) mmfar);
    SEGGER_RTT_printf(0, "EXC_RETURN=0x%08X\r\n", (unsigned int) exc_return);
    SEGGER_RTT_printf(0, "MSP=0x%08X PSP=0x%08X\r\n",
                      (unsigned int) __get_MSP(), (unsigned int) __get_PSP());

    /* frame = 真异常现场 [R0,R1,R2,R3,R12,LR,PC,xPSR] */
    SEGGER_RTT_printf(0, "R0 =0x%08X R1 =0x%08X\r\n",
                      (unsigned int) frame[0], (unsigned int) frame[1]);
    SEGGER_RTT_printf(0, "R2 =0x%08X R3 =0x%08X\r\n",
                      (unsigned int) frame[2], (unsigned int) frame[3]);
    SEGGER_RTT_printf(0, "R12=0x%08X LR =0x%08X\r\n",
                      (unsigned int) frame[4], (unsigned int) frame[5]);
    SEGGER_RTT_printf(0, "PC =0x%08X xPSR=0x%08X\r\n",
                      (unsigned int) frame[6], (unsigned int) frame[7]);

    /* frame 下方 = 异常前栈内容（调用者帧：返回地址/局部变量） */
    SEGGER_RTT_printf(0, "--- caller stack (below frame) ---\r\n");
    for (int i = 1; i <= 16; i++)
    {
        uint32_t v = frame[-i];
        SEGGER_RTT_printf(0, "  [-%02d] 0x%08X\r\n", i, (unsigned int) v);
    }

    while (1)
    {
        ;
    }
}
