#include "qspi_flash_test.h"
#include "Uart9_Debug.h"
#include "stdio.h"
#include "string.h"

/* 测试数据缓冲区：放在 SDRAM 中 */
static uint8_t  g_qspi_write_buf[QSPI_TEST_SIZE] __attribute__((aligned(32)));
static uint8_t  g_qspi_read_buf[QSPI_TEST_SIZE]  __attribute__((aligned(32)));

/* 等待 Flash 写/擦操作完成 */
static fsp_err_t qspi_wait_ready(spi_flash_ctrl_t *p_ctrl)
{
    spi_flash_status_t status;
    uint32_t timeout = 0xFFFFFFFF;

    do {
        fsp_err_t err = R_OSPI_B_StatusGet(p_ctrl, &status);
        if (FSP_SUCCESS != err) {
            printf("  [ERR] StatusGet failed: %d\r\n", err);
            return err;
        }
        if (!status.write_in_progress) {
            return FSP_SUCCESS;
        }
        timeout--;
    } while (timeout > 0);

    printf("  [ERR] Flash busy timeout!\r\n");
    return FSP_ERR_TIMEOUT;
}

/* 使用 DirectTransfer 读取 Flash 数据
 * p_ctrl: 驱动控制句柄
 * addr: Flash 目标地址
 * p_buf: 读数据缓冲区
 * len: 读取长度 (字节)
 *
 * OSPI-B 不支持 DirectRead/DirectWrite，必须用 DirectTransfer。
 * Prefetch 开启时也能正常使用 DirectTransfer。
 * 每次 DirectTransfer 最多传 8 字节 (data_length 最大 8)。
 */
static fsp_err_t qspi_direct_read(spi_flash_ctrl_t *p_ctrl, uint32_t addr,
                                   uint8_t *p_buf, uint32_t len)
{
    fsp_err_t err;
    spi_flash_direct_transfer_t transfer;

    uint32_t offset = 0;
    while (offset < len) {
        /* 每次最多读 8 字节 */
        uint32_t chunk = len - offset;
        if (chunk > 8) {
            chunk = 8;
        }

        transfer.command       = 0x03;               /* 标准读取命令 */
        transfer.command_length = 1;                  /* 1 字节命令 */
        transfer.address_length = 3;                  /* 3 字节地址 (24-bit) */
        transfer.address       = addr + offset;       /* 当前地址 */
        transfer.dummy_cycles  = 0;                   /* 标准读不需要 dummy cycles */
        transfer.data_length   = (uint8_t)chunk;      /* 本次读取字节数 */
        transfer.data_u64      = 0;

        err = R_OSPI_B_DirectTransfer(p_ctrl, &transfer,
                                       SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
        if (FSP_SUCCESS != err) {
            return err;
        }

        /* 从 data_u64 中提取读取到的数据 */
        for (uint32_t j = 0; j < chunk; j++) {
            p_buf[offset + j] = (uint8_t)(transfer.data_u64 >> (j * 8));
        }
        offset += chunk;
    }
    return FSP_SUCCESS;
}

/* 打印 Flash 设备信息 */
void qspi_flash_info(void)
{
    printf("========================================\r\n");
    printf("  QSPI Flash (OSPI-B) Info\r\n");
    printf("========================================\r\n");
    printf("  Instance:     g_ospi0\r\n");
    printf("  Channel:      CS0\r\n");
    printf("  Base Address: 0x80000000\r\n");
    printf("  Size:         256 MB\r\n");
    printf("  Protocol:     1S-1S-1S / 1S-4S-4S\r\n");
    printf("  Page Size:    64 bytes\r\n");
    printf("  Erase Cmds:   0x20 (4KB), 0x52 (32KB), 0x60 (Chip)\r\n");
    printf("  Read Cmd:     0x03 (std), 0xEB (fast quad)\r\n");
    printf("  Write Cmd:    0x02 (std), 0x32 (quad)\r\n");
    printf("  Prefetch:     Enabled\r\n");
    printf("  Combination:  64-byte\r\n");
    printf("========================================\r\n");
}

/* 完整读写擦测试 */
qspi_test_result_t qspi_flash_test(void)
{
    fsp_err_t err;
    uint32_t  i;
    uint32_t  fail_count = 0;

    printf("\r\n");
    printf("########################################\r\n");
    printf("#  QSPI Flash Read/Write/Erase Test   #\r\n");
    printf("########################################\r\n");
    qspi_flash_info();

    /* ====== Step 1: 打开 QSPI Flash 驱动 ====== */
    printf("\r\n[1/5] Opening QSPI Flash driver...\r\n");
    err = R_OSPI_B_Open(&g_ospi0_ctrl, &g_ospi0_cfg);
    if (FSP_SUCCESS != err) {
        printf("  [ERR] R_OSPI_B_Open failed: %d\r\n", err);
        return QSPI_TEST_ERR_OPEN;
    }
    printf("  [OK]  Driver opened successfully.\r\n");

    /* ====== Step 2: 擦除目标 sector ====== */
    printf("\r\n[2/5] Erasing sector at 0x%08X...\r\n", (uint32_t)QSPI_TEST_ADDRESS);
    err = R_OSPI_B_Erase(&g_ospi0_ctrl, QSPI_TEST_ADDRESS, QSPI_TEST_SECTOR_SIZE);
    if (FSP_SUCCESS != err) {
        printf("  [ERR] R_OSPI_B_Erase failed: %d\r\n", err);
        R_OSPI_B_Close(&g_ospi0_ctrl);
        return QSPI_TEST_ERR_ERASE;
    }
    err = qspi_wait_ready(&g_ospi0_ctrl);
    if (FSP_SUCCESS != err) {
        R_OSPI_B_Close(&g_ospi0_ctrl);
        return QSPI_TEST_ERR_TIMEOUT;
    }
    printf("  [OK]  Sector erased.\r\n");

    /* ====== Step 3: 验证擦除（读回全 0xFF） ====== */
    printf("\r\n[3/5] Verifying erase (expect all 0xFF)...\r\n");
    memset(g_qspi_read_buf, 0x00, QSPI_TEST_SIZE);
    err = qspi_direct_read(&g_ospi0_ctrl, (uint32_t)QSPI_TEST_ADDRESS,
                           g_qspi_read_buf, QSPI_TEST_SIZE);
    if (FSP_SUCCESS != err) {
        printf("  [ERR] DirectTransfer read failed: %d\r\n", err);
        R_OSPI_B_Close(&g_ospi0_ctrl);
        return QSPI_TEST_ERR_READ;
    }

    for (i = 0; i < QSPI_TEST_SIZE; i++) {
        if (g_qspi_read_buf[i] != 0xFF) {
            if (fail_count < 8) {
                printf("  [WARN] byte[%d]=0x%02X (expected 0xFF)\r\n", (int)i, g_qspi_read_buf[i]);
            }
            fail_count++;
        }
    }
    if (fail_count == 0) {
        printf("  [OK]  All %d bytes are 0xFF (erased).\r\n", QSPI_TEST_SIZE);
    } else {
        printf("  [WARN] %d/%d bytes are NOT 0xFF\r\n", (int)fail_count, QSPI_TEST_SIZE);
    }

    /* ====== Step 4: 写入测试数据 ====== */
    printf("\r\n[4/5] Writing test pattern (page by page)...\r\n");

    /* 填充写缓冲区：递增模式 0x00, 0x01, 0x02, ... */
    for (i = 0; i < QSPI_TEST_SIZE; i++) {
        g_qspi_write_buf[i] = (uint8_t)(i & 0xFF);
    }

    /* R_OSPI_B_Write 每次只能写一个 page（64 字节），不能跨 page 边界。
     * 需要按 page 分次写入。 */
    {
        uint32_t written = 0;
        uint32_t page_size = g_ospi0_cfg.page_size_bytes;  /* 64 */
        while (written < QSPI_TEST_SIZE) {
            uint32_t chunk = QSPI_TEST_SIZE - written;
            if (chunk > page_size) {
                chunk = page_size;
            }
            /* 确保不跨 page 边界 */
            uint32_t page_offset = ((uint32_t)QSPI_TEST_ADDRESS + written) & (page_size - 1);
            if (chunk > (page_size - page_offset)) {
                chunk = page_size - page_offset;
            }

            err = R_OSPI_B_Write(&g_ospi0_ctrl,
                                 g_qspi_write_buf + written,
                                 QSPI_TEST_ADDRESS + written,
                                 chunk);
            if (FSP_SUCCESS != err) {
                printf("  [ERR] Write page at +%d failed: %d\r\n", (int)written, err);
                R_OSPI_B_Close(&g_ospi0_ctrl);
                return QSPI_TEST_ERR_WRITE;
            }
            err = qspi_wait_ready(&g_ospi0_ctrl);
            if (FSP_SUCCESS != err) {
                R_OSPI_B_Close(&g_ospi0_ctrl);
                return QSPI_TEST_ERR_TIMEOUT;
            }
            written += chunk;
        }
    }
    printf("  [OK]  %d bytes written.\r\n", QSPI_TEST_SIZE);

    /* ====== Step 5: 读回并验证 ====== */
    printf("\r\n[5/5] Reading back and verifying...\r\n");
    memset(g_qspi_read_buf, 0x00, QSPI_TEST_SIZE);

    err = qspi_direct_read(&g_ospi0_ctrl, (uint32_t)QSPI_TEST_ADDRESS,
                           g_qspi_read_buf, QSPI_TEST_SIZE);
    if (FSP_SUCCESS != err) {
        printf("  [ERR] DirectTransfer read failed: %d\r\n", err);
        R_OSPI_B_Close(&g_ospi0_ctrl);
        return QSPI_TEST_ERR_READ;
    }

    fail_count = 0;
    for (i = 0; i < QSPI_TEST_SIZE; i++) {
        if (g_qspi_read_buf[i] != g_qspi_write_buf[i]) {
            if (fail_count < 8) {
                printf("  [FAIL] byte[%d]: wrote=0x%02X, read=0x%02X\r\n",
                       (int)i, g_qspi_write_buf[i], g_qspi_read_buf[i]);
            }
            fail_count++;
        }
    }

    /* ====== 关闭驱动 ====== */
    R_OSPI_B_Close(&g_ospi0_ctrl);

    /* ====== 结果 ====== */
    printf("\r\n========================================\r\n");
    if (fail_count == 0) {
        printf("  RESULT: ALL %d BYTES MATCH - PASS!\r\n", QSPI_TEST_SIZE);
        printf("========================================\r\n\n");
        return QSPI_TEST_OK;
    } else {
        printf("  RESULT: %d/%d BYTES MISMATCH - FAIL!\r\n", (int)fail_count, QSPI_TEST_SIZE);
        printf("========================================\r\n\n");
        return QSPI_TEST_ERR_VERIFY;
    }
}

/* 仅读取测试：读取 Flash 中已有的数据并打印，不擦除不写入 */
qspi_test_result_t qspi_flash_read_test(void)
{
    fsp_err_t err;

    printf("\r\n");
    printf("========================================\r\n");
    printf("  QSPI Flash Read-Only Test\r\n");
    printf("========================================\r\n");

    /* 打开驱动 */
    err = R_OSPI_B_Open(&g_ospi0_ctrl, &g_ospi0_cfg);
    if (FSP_SUCCESS != err) {
        printf("  [ERR] R_OSPI_B_Open failed: %d\r\n", err);
        return QSPI_TEST_ERR_OPEN;
    }
    printf("  [OK]  Driver opened.\r\n");

    /* 读取前 256 字节 */
    memset(g_qspi_read_buf, 0x00, QSPI_TEST_SIZE);
    err = qspi_direct_read(&g_ospi0_ctrl, (uint32_t)QSPI_TEST_ADDRESS,
                           g_qspi_read_buf, QSPI_TEST_SIZE);
    if (FSP_SUCCESS != err) {
        printf("  [ERR] DirectTransfer read failed: %d\r\n", err);
        R_OSPI_B_Close(&g_ospi0_ctrl);
        return QSPI_TEST_ERR_READ;
    }

    /* 打印前 128 字节 */
    qspi_flash_dump((uint32_t)QSPI_TEST_ADDRESS, 128);

    R_OSPI_B_Close(&g_ospi0_ctrl);
    printf("  [OK]  Read test done.\r\n");
    printf("========================================\r\n\n");
    return QSPI_TEST_OK;
}

/* 打印 Flash 数据 hex dump */
void qspi_flash_dump(uint32_t addr, uint32_t len)
{
    fsp_err_t err;
    uint8_t   dump_buf[256] __attribute__((aligned(32)));

    if (len > sizeof(dump_buf)) {
        len = sizeof(dump_buf);
    }

    err = R_OSPI_B_Open(&g_ospi0_ctrl, &g_ospi0_cfg);
    if (FSP_SUCCESS != err) {
        printf("  [ERR] Open for dump failed\r\n");
        return;
    }

    memset(dump_buf, 0, len);
    err = qspi_direct_read(&g_ospi0_ctrl, addr, dump_buf, len);

    if (FSP_SUCCESS != err) {
        printf("  [ERR] Read for dump failed: %d\r\n", err);
        R_OSPI_B_Close(&g_ospi0_ctrl);
        return;
    }

    for (uint32_t i = 0; i < len; i += 16) {
        printf("  0x%08X: ", (unsigned int)(addr + i));
        for (uint32_t j = 0; j < 16 && (i + j) < len; j++) {
            printf("%02X ", dump_buf[i + j]);
        }
        printf("\r\n");
    }

    R_OSPI_B_Close(&g_ospi0_ctrl);
}
