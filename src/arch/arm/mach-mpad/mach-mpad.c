/*
 * mach-mpad.c
 *
 * Copyright (c) 2007-2011  jianjun jiang <jerryjianjun@gmail.com>
 * official site: http://xboot.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <configs.h>
#include <default.h>
#include <types.h>
#include <macros.h>
#include <io.h>
#include <mode/mode.h>
#include <xboot/log.h>
#include <xboot/printk.h>
#include <xboot/machine.h>
#include <xboot/initcall.h>
#include <s5pv210/reg-wdg.h>
#include <s5pv210/reg-gpio.h>
#include <s5pv210/reg-timer.h>
#include <s5pv210/reg-keypad.h>
#include <s5pv210-cp15.h>

extern x_u8	__text_start[];
extern x_u8 __text_end[];
extern x_u8 __ramdisk_start[];
extern x_u8 __ramdisk_end[];
extern x_u8 __data_shadow_start[];
extern x_u8 __data_shadow_end[];
extern x_u8 __data_start[];
extern x_u8 __data_end[];
extern x_u8 __bss_start[];
extern x_u8 __bss_end[];
extern x_u8 __heap_start[];
extern x_u8 __heap_end[];
extern x_u8 __stack_start[];
extern x_u8 __stack_end[];

/*
 * system initial, like power lock
 */
static void mach_init(void)
{
	/* gph0_5 high level for power lock */
	writel(S5PV210_GPH0CON, (readl(S5PV210_GPH0CON) & ~(0xf<<20)) | (0x1<<20));
	writel(S5PV210_GPH0PUD, (readl(S5PV210_GPH0PUD) & ~(0x3<<10)) | (0x0<<10));
	writel(S5PV210_GPH0DAT, (readl(S5PV210_GPH0DAT) & ~(0x1<<5)) | (0x1<<5));
}

/*
 * system halt, shutdown machine.
 */
static x_bool mach_halt(void)
{
	/* gph0_5 low level for power unlock */
	writel(S5PV210_GPH0CON, (readl(S5PV210_GPH0CON) & ~(0xf<<20)) | (0x1<<20));
	writel(S5PV210_GPH0PUD, (readl(S5PV210_GPH0PUD) & ~(0x3<<10)) | (0x0<<10));
	writel(S5PV210_GPH0DAT, (readl(S5PV210_GPH0DAT) & ~(0x1<<5)) | (0x0<<5));

	return TRUE;
}

/*
 * reset the cpu by setting up the watchdog timer and let him time out
 */
static x_bool mach_reset(void)
{
	/* disable watchdog */
	writel(S5PV210_WTCON, 0x0000);

	/* initialize watchdog timer count register */
	writel(S5PV210_WTCNT, 0x0001);

	/* enable watchdog timer; assert reset at timer timeout */
	writel(S5PV210_WTCON, 0x0021);

	return TRUE;
}

/*
 * get system mode
 */
static enum mode mach_getmode(void)
{
	/* set GPH3_4 intput and pull up */
	writel(S5PV210_GPH3CON, (readl(S5PV210_GPH3CON) & ~(0xf<<16)) | (0x0<<16));
	writel(S5PV210_GPH3PUD, (readl(S5PV210_GPH3PUD) & ~(0x3<<8)) | (0x2<<8));

	if((readl(S5PV210_GPH3DAT) & (0x1<<4)) == 0)
		return MODE_MENU;
	return MODE_NORMAL;
}

/*
 * clean up system before running os
 */
static x_bool mach_cleanup(void)
{
	/* stop timer 0 ~ 4 */
	writel(S5PV210_TCON, 0x0);

	/* stop system timer */
	writel(S5PV210_SYSTIMER_TCON, 0x0);

	/* disable irq */
	irq_disable();

	/* disable fiq */
	fiq_disable();

	/* disable icache */
	icache_disable();

	/* disable dcache */
	dcache_disable();

	/* disable mmu */
	mmu_disable();

	/* disable vic */
	vic_disable();

	return TRUE;
}

/*
 * for anti-piracy
 */
static x_bool mach_genuine(void)
{
	return TRUE;
}

/*
 * a portable data interface for machine.
 */
static struct machine mpad = {
	.info = {
		.board_name 		= "mpad",
		.board_desc 		= "mpad based on s5pv210",
		.board_id			= "0x0001a870",

		.cpu_name			= "s5pv210",
		.cpu_desc			= "cortex-a8",
		.cpu_id				= "0x00000000",
	},

	.res = {
		.mem_banks = {
			[0] = {
				.start		= 0x30000000,
				.end		= 0x30000000 + SZ_256M - 1,
			},

			[1] = {
				.start		= 0x40000000,
				.end		= 0x40000000 + SZ_256M - 1,
			},

			[2] = {
				.start		= 0,
				.end		= 0,
			},
		},

		.xtal				= 24*1000*1000,
	},

	.link = {
		.text_start			= (const x_sys)__text_start,
		.text_end			= (const x_sys)__text_end,

		.ramdisk_start		= (const x_sys)__ramdisk_start,
		.ramdisk_end		= (const x_sys)__ramdisk_end,

		.data_shadow_start	= (const x_sys)__data_shadow_start,
		.data_shadow_end	= (const x_sys)__data_shadow_end,

		.data_start			= (const x_sys)__data_start,
		.data_end			= (const x_sys)__data_end,

		.bss_start			= (const x_sys)__bss_start,
		.bss_end			= (const x_sys)__bss_end,

		.heap_start			= (const x_sys)__heap_start,
		.heap_end			= (const x_sys)__heap_end,

		.stack_start		= (const x_sys)__stack_start,
		.stack_end			= (const x_sys)__stack_end,
	},

	.pm = {
		.init 				= mach_init,
		.suspend			= NULL,
		.resume				= NULL,
		.halt				= mach_halt,
		.reset				= mach_reset,
	},

	.misc = {
		.getmode			= mach_getmode,
		.cleanup			= mach_cleanup,
		.genuine			= mach_genuine,
	},

	.priv					= NULL,
};

static __init void mach_mpad_init(void)
{
	if(!machine_register(&mpad))
		LOG_E("failed to register machine 'mpad'");
}

module_init(mach_mpad_init, LEVEL_MACH);
