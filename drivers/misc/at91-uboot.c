/*
 * U-boot driver for Atmel AT91
 *
 * Copyright (C) 2019 Mickael GARDET <m.gardet@overkiz.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/device.h>

static void __iomem *bureg_base;
static struct kobject *uboot;

#define BUREG(x) (bureg_base + (x*4))
#define BUREG_2 BUREG(2)
#define BUREG_3 BUREG(3)

#define BUREG_3_UBOOT_FACTORY 2
#define BUREG_2_FORCE_UBOOT_FACTORY 3

static void write_reg(void __iomem *addr, int index, u8 value)
{
	u32 reg = readl(addr);
	u32 v = (value  << (index * 8));
	reg &= ~(0xFF << (index * 8));
	reg |= v;
	writel(reg, addr);
}

static u8 read_reg(void __iomem *addr, int index)
{
	u32 reg = readl(addr);
	return (reg >> (index * 8)) & 0xFF;
}

/* BUREG_2 */
static ssize_t factory_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t size)
{
	unsigned long value;

	ssize_t ret = kstrtoul(buf, 0, &value);
	if (ret)
		return ret;
	write_reg(BUREG_2, 0, value);

	return size;
}

static ssize_t factory_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", read_reg(BUREG_2, 0));
}

/* BUREG_3 */
static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n",
                     read_reg(BUREG_3, 0)  == BUREG_3_UBOOT_FACTORY ? "u-boot-factory" : "u-boot" );
}

static ssize_t version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", (readl(BUREG_3) >> 8) & 0xFFFF );
}

static struct kobj_attribute mode    = __ATTR_RO(mode);
static struct kobj_attribute version = __ATTR_RO(version);
static struct kobj_attribute factory = __ATTR_RW(factory);

static struct attribute *uboot_attributes[] = {
	&mode.attr,
	&version.attr,
	&factory.attr,
	NULL
};

static struct attribute_group uboot_group = {
	.attrs = uboot_attributes,
};

static const struct of_device_id overkiz_at91_uboot_dt_ids[] = {
	{ .compatible = "overkiz,at91-uboot" },
	{ /* sentinel */ }
};

static int __init at91_uboot_init(void)
{
	int ret = 0;
	struct device_node *np;
	struct resource res;

	bureg_base = NULL;
	uboot = NULL;

	np = of_find_matching_node(NULL, overkiz_at91_uboot_dt_ids);
	if (!np)
		return -EINVAL;

	if (of_address_to_resource(np, 0, &res) == 0)
	{
		/* 4 bureg registers */
		if(resource_size(&res) >= (4*4))
		{
			bureg_base = ioremap(res.start, resource_size(&res));
		}
		else
		{
			printk(KERN_ERR "at91-uboot unsupported size %d\n",resource_size(&res));
		}
	}

	of_node_put(np);
	if (!bureg_base)
		return -EINVAL;

	uboot = kobject_create_and_add("uboot", NULL);
	if (!uboot)
		return -ENOMEM;

	ret = sysfs_create_group(uboot, &uboot_group);
	if (ret) {
		kobject_del(uboot);
		return ret;
	}

	return ret;
}

static void __exit at91_uboot_exit(void)
{
	sysfs_remove_group(uboot, &uboot_group);
	iounmap(bureg_base);
	bureg_base = NULL;
	kobject_del(uboot);
	uboot = NULL;
}

module_init(at91_uboot_init);
module_exit(at91_uboot_exit);

MODULE_AUTHOR("Mickael GARDET");
MODULE_DESCRIPTION("U-Boot driver for Atmel AT91");
MODULE_LICENSE("GPL");

