// SPDX-License-Identifier: GPL-2.0-only
/*
 * init/noinitramfs.c
 *
 * Copyright (C) 2006, NXP Semiconductors, All Rights Reserved
 * Author: Jean-Paul Saman <jean-paul.saman@nxp.com>
 *
 * Prism overlay: goto-cleanup replaced with orelse
 */
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/kdev_t.h>
#include <linux/syscalls.h>
#include <linux/init_syscalls.h>
#include <linux/umh.h>

static int __init default_rootfs(void)
{
	usermodehelper_enable();

	init_mkdir("/dev", 0755) orelse goto fail;
	init_mknod("/dev/console", S_IFCHR | S_IRUSR | S_IWUSR,
		   new_encode_dev(MKDEV(5, 1))) orelse goto fail;
	init_mkdir("/root", 0700) orelse goto fail;

	return 0;
fail:
	printk(KERN_WARNING "Failed to create a rootfs\n");
	return -1;
}
rootfs_initcall(default_rootfs);
