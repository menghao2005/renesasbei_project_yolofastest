/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_b_uart_rxi_isr, /* SCI9 RXI (Receive data full) */
            [1] = sci_b_uart_txi_isr, /* SCI9 TXI (Transmit data empty) */
            [2] = sci_b_uart_tei_isr, /* SCI9 TEI (Transmit end) */
            [3] = sci_b_uart_eri_isr, /* SCI9 ERI (Receive error) */
            [4] = gpt_capture_compare_a_isr, /* GPT1 CAPTURE COMPARE A (Capture/Compare match A) */
            [5] = gpt_capture_compare_b_isr, /* GPT1 CAPTURE COMPARE B (Capture/Compare match B) */
            [6] = ceu_isr, /* CEU CEUI (CEU interrupt) */
            [7] = iic_master_rxi_isr, /* IIC0 RXI (Receive data full) */
            [8] = iic_master_txi_isr, /* IIC0 TXI (Transmit data empty) */
            [9] = iic_master_tei_isr, /* IIC0 TEI (Transmit end) */
            [10] = iic_master_eri_isr, /* IIC0 ERI (Transfer error) */
            [11] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
            [12] = glcdc_underflow_1_isr, /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
            [13] = mipi_dsi_seq0_isr, /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
            [14] = mipi_dsi_seq1_isr, /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
            [15] = mipi_dsi_vin1_isr, /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
            [16] = mipi_dsi_rcv_isr, /* MIPIDSI RCV (DSI packet receive interrupt) */
            [17] = mipi_dsi_ferr_isr, /* MIPIDSI FERR (DSI fatal error interrupt) */
            [18] = mipi_dsi_ppi_isr, /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
            [19] = drw_int_isr, /* DRW INT (DRW interrupt) */
            [20] = r_icu_isr, /* ICU IRQ20 (External pin interrupt 20) */
            [21] = r_icu_isr, /* ICU IRQ8 (External pin interrupt 8) */
            [22] = rm_ethosu_isr, /* NPU IRQ (NPU IRQ) */
            [23] = gpt_counter_overflow_isr, /* GPT0 COUNTER OVERFLOW (Overflow) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI9_RXI,GROUP0), /* SCI9 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TXI,GROUP1), /* SCI9 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TEI,GROUP2), /* SCI9 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI9_ERI,GROUP3), /* SCI9 ERI (Receive error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_GPT1_CAPTURE_COMPARE_A,GROUP4), /* GPT1 CAPTURE COMPARE A (Capture/Compare match A) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_GPT1_CAPTURE_COMPARE_B,GROUP5), /* GPT1 CAPTURE COMPARE B (Capture/Compare match B) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_CEU_CEUI,GROUP6), /* CEU CEUI (CEU interrupt) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC0_RXI,GROUP7), /* IIC0 RXI (Receive data full) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TXI,GROUP0), /* IIC0 TXI (Transmit data empty) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TEI,GROUP1), /* IIC0 TEI (Transmit end) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_IIC0_ERI,GROUP2), /* IIC0 ERI (Transfer error) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP3), /* GLCDC LINE DETECT (Specified line) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_UNDERFLOW_1,GROUP4), /* GLCDC UNDERFLOW 1 (Graphic 1 underflow) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ0,GROUP5), /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ1,GROUP6), /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_VIN1,GROUP7), /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_RCV,GROUP0), /* MIPIDSI RCV (DSI packet receive interrupt) */
            [17] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_FERR,GROUP1), /* MIPIDSI FERR (DSI fatal error interrupt) */
            [18] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_PPI,GROUP2), /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
            [19] = BSP_PRV_VECT_ENUM(EVENT_DRW_INT,GROUP3), /* DRW INT (DRW interrupt) */
            [20] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ20,GROUP4), /* ICU IRQ20 (External pin interrupt 20) */
            [21] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ8,GROUP5), /* ICU IRQ8 (External pin interrupt 8) */
            [22] = BSP_PRV_VECT_ENUM(EVENT_NPU_IRQ,GROUP6), /* NPU IRQ (NPU IRQ) */
            [23] = BSP_PRV_VECT_ENUM(EVENT_GPT0_COUNTER_OVERFLOW,GROUP7), /* GPT0 COUNTER OVERFLOW (Overflow) */
        };
        #endif
        #endif
