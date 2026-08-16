#ifndef __QSPI_FLASH_TEST_H
#define __QSPI_FLASH_TEST_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

/* QSPI Flash 测试配置 */
#define QSPI_TEST_ADDRESS       ((uint8_t *)0x80000000)   /* OSPI CS0 起始地址 */
#define QSPI_TEST_SIZE          (256)                       /* 测试数据大小 (bytes) */
#define QSPI_TEST_SECTOR_SIZE   (0x1000)                   /* 4KB sector 大小 */
#define QSPI_TEST_PATTERN_BYTE  (0xA5)                      /* 测试模式字节 */

/* 测试结果枚举 */
typedef enum {
    QSPI_TEST_OK = 0,
    QSPI_TEST_ERR_OPEN,
    QSPI_TEST_ERR_ERASE,
    QSPI_TEST_ERR_WRITE,
    QSPI_TEST_ERR_READ,
    QSPI_TEST_ERR_VERIFY,
    QSPI_TEST_ERR_TIMEOUT
} qspi_test_result_t;

/* 函数声明 */
qspi_test_result_t qspi_flash_test(void);
qspi_test_result_t qspi_flash_read_test(void);      /* 仅读取测试，不破坏数据 */
void               qspi_flash_dump(uint32_t addr, uint32_t len);
void               qspi_flash_info(void);

#endif /* __QSPI_FLASH_TEST_H */
