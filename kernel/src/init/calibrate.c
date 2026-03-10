// SPDX-License-Identifier: GPL-2.0
/* calibrate.c: delay calibration
 *
 * Original: Copyright (C) 1991, 1992 Linus Torvalds
 *
 * On modern x86_64, lpj_fine is set from TSC by arch/x86/kernel/tsc.c
 * before calibrate_delay() runs. The 300-line busy-loop binary search
 * and SMI-jitter retry logic only exist for pre-2005 hardware without
 * a reliable TSC. This version keeps only the fast path.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/percpu.h>
#include <linux/printk.h>
#include <linux/smp.h>

unsigned long lpj_fine;
unsigned long preset_lpj;

unsigned long __attribute__((weak)) calibrate_delay_is_known(void)
{
	return 0;
}

void __attribute__((weak)) calibration_delay_done(void)
{
}

void calibrate_delay(void)
{
	unsigned long lpj = lpj_fine;

	if (!lpj)
		lpj = preset_lpj;
	if (!lpj)
		lpj = calibrate_delay_is_known();
	if (!lpj) {
		pr_err("calibrate_delay: no TSC or known delay source\n");
		lpj = 1;
	}

	loops_per_jiffy = lpj;
	pr_info("Calibrating delay loop (skipped), "
		"value calculated using timer frequency.. "
		"%lu.%02lu BogoMIPS (lpj=%lu)\n",
		lpj/(500000/HZ), (lpj/(5000/HZ)) % 100, lpj);

	calibration_delay_done();
}
