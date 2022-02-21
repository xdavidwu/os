#include "pm.h"
#include "bcm2835_mmio.h"

void platform_reset() {
	*PM_RSTC = PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
	*PM_WDOG = PM_PASSWORD | 1;
}
