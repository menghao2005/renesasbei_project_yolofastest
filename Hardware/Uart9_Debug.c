/*
 * Uart9_Debug.c
 *
 * 调试串口（UART 实例 g_uart9_test）驱动 + 标准库流重定向：
 *   把 newlib 的 stdin/stdout/stderr 绑定到同一条调试串口，
 *   printf/puts 等标准输出经 uart9_putchar() 逐字节发出（写后忙等完成标志）；
 *   收方向未实现接收缓冲（getchar 恒返回 EOF），中断回调仅把收到的
 *   字节原样回显转发，并置位收/发完成标志 Uart_Flag。
 */
#include "Uart9_Debug.h"
#include "stdint.h"
#include "stdio.h"

static int uart9_putchar(char ch, FILE * p_file);
static int uart9_getchar(FILE * p_file);
static int uart9_flush(FILE * p_file);

/* 收/发完成标志（UART 中断置位，putchar 忙等轮询发送完成） */
Flag Uart_Flag =
{
    .Receive = Failure,
    .Send = Failure
};

static FILE g_uart9_stdout = FDEV_SETUP_STREAM(uart9_putchar, uart9_getchar, uart9_flush, _FDEV_SETUP_RW);

/* 标准流重定向：stdin/stdout/stderr 共用同一条调试串口，
 * FDEV_SETUP_STREAM 宏把三个流后端函数绑定成一个 FILE 对象 */
FILE * const stdin  = &g_uart9_stdout;
FILE * const stdout = &g_uart9_stdout;
FILE * const stderr = &g_uart9_stdout;

/* 初始化调试串口（RA FSP 驱动打开实例） */
void uart9_Init(void)
{
    R_SCI_B_UART_Open(&g_uart9_test_ctrl, &g_uart9_test_cfg);
}

/* UART 事件回调：收到单字节 -> 置接收标志并原样回显转发；
 * 发送完成 -> 置发送标志（供 uart9_putchar 解除忙等） */
void g_uart9_testcallback(uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_RX_CHAR:
        {
            Uart_Flag.Receive = Success;
            R_SCI_B_UART_Write(&g_uart9_test_ctrl, (uint8_t* )&(p_args->data), 1);//转发
            break;
        }

        case UART_EVENT_TX_COMPLETE:
        {
            Uart_Flag.Send = Success;
            break;
        }

        default:
        {
            break;
        }
    }
}

/* 单字节发送（流输出后端）：写 UART 后忙等发送完成标志，返回发送的字节 */
static int uart9_putchar(char ch, FILE * p_file)
{
    uint8_t data = (uint8_t) ch;

    (void) p_file;
    Uart_Flag.Send = Failure;

    R_SCI_B_UART_Write(&g_uart9_test_ctrl, &data, 1);

    while (Uart_Flag.Send == Failure)
    {
        ;
    }

    return (int) data;
}

/* 读后端：未实现接收缓冲，固定返回 EOF */
static int uart9_getchar(FILE * p_file)
{
    (void) p_file;
    return _FDEV_EOF;
}

/* flush 后端：本流直写无缓冲，空操作恒返回 0 */
static int uart9_flush(FILE * p_file)
{
    (void) p_file;
    return 0;
}

/* newlib 低层写接口：printf/puts 等最终经这里逐字节发往调试串口，
 * 返回实际写入的字节数 */
int _write(int fd, char * pBuffer, int size)
{
    int index = 0;

    (void) fd;

    for (index = 0; index < size; index++)
    {
        uart9_putchar(pBuffer[index], stdout);
    }

    return size;
}
