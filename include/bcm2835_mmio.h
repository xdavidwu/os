#ifndef BCM2835_MMIO_H
#define BCM2835_MMIO_H

#include <stdint.h>

#define MMIO_BASE	0x3f000000

#define PM_OFFSET	0x00100000

#define PM_RSTC	((uint32_t *)(MMIO_BASE + PM_OFFSET + 0x1c))
#define PM_WDOG	((uint32_t *)(MMIO_BASE + PM_OFFSET + 0x24))

#define PM_PASSWORD	0x5a000000
#define PM_RSTC_WRCFG_FULL_RESET	0x20

#define GPIO_OFFSET	0x00200000

#define GPFSEL1	((uint32_t *)(MMIO_BASE + GPIO_OFFSET + 0x4))
#define GPPUD	((uint32_t *)(MMIO_BASE + GPIO_OFFSET + 0x94))
#define GPPUDCLK0	((uint32_t *)(MMIO_BASE + GPIO_OFFSET + 0x98))

#define FSEL14_OFFSET	12
#define FSEL15_OFFSET	15

#define FSELMASK	0b111U
#define ALT5	0b010U

#define AUX_OFFSET	0x00215000

// TODO: struct these
#define AUXENB	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x4))
#define AUXENB_MINIUART	0b1U
#define AUX_MU_IO_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x40))
#define AUX_MU_IER_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x44))
#define AUX_MU_IIR_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x48))
#define AUX_MU_IIR_RFIFO_CLEAR	(1 << 1)
#define AUX_MU_IIR_WFIFO_CLEAR	(1 << 2)
#define AUX_MU_LCR_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x4c))
#define AUX_MU_LCR_8BIT	0b11
#define AUX_MU_MCR_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x50))
#define AUX_MU_LSR_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x54))
#define AUX_MU_LSR_RX_READY	(1 << 0)
#define AUX_MU_LSR_TX_READY	(1 << 5)
#define AUX_MU_MSR_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x58))
#define AUX_MU_SCRATCH	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x5c))
#define AUX_MU_CNTL_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x60))
#define AUX_MU_CNTL_RX_ENABLE	(1 << 0)
#define AUX_MU_CNTL_TX_ENABLE	(1 << 1)
#define AUX_MU_STAT_REG	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x64))
#define AUX_MU_BAUD	((uint32_t *)(MMIO_BASE + AUX_OFFSET + 0x68))

#endif
