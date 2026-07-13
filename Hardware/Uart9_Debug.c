#include "Uart9_Debug.h"
#include "stdint.h"
#include "stdio.h"

static int uart9_putchar(char ch, FILE * p_file);
static int uart9_getchar(FILE * p_file);
static int uart9_flush(FILE * p_file);

Flag Uart_Flag =
{
    .Receive = Failure,
    .Send = Failure
};

static FILE g_uart9_stdout = FDEV_SETUP_STREAM(uart9_putchar, uart9_getchar, uart9_flush, _FDEV_SETUP_RW);

FILE * const stdin  = &g_uart9_stdout;
FILE * const stdout = &g_uart9_stdout;
FILE * const stderr = &g_uart9_stdout;

void uart9_Init(void)
{
    R_SCI_B_UART_Open(&g_uart9_test_ctrl, &g_uart9_test_cfg);
}

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

static int uart9_getchar(FILE * p_file)
{
    (void) p_file;
    return _FDEV_EOF;
}

static int uart9_flush(FILE * p_file)
{
    (void) p_file;
    return 0;
}

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
