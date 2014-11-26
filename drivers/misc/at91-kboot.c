/*
 * K-Boot driver for Atmel AT91
 *
 * Copyright (C) 2014 Gaël Portay <g.portay@overkiz.com>
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

static void __iomem *gpbr;

static struct kobject *kboot;

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	u32 cur = __raw_readl(gpbr);
	cur &= 0xFFFF;

	return snprintf(buf, PAGE_SIZE, "%u\n", cur);
}

static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
	ssize_t ret;
	u32 reg;
	unsigned long cur;

	reg = __raw_readl(gpbr);
	ret = kstrtoul(buf, 0, &cur);
	if (ret)
		return ret;
	cur &= 0xFFFF;
	reg &= 0xFFFF0000;
	reg |= cur;

	__raw_writel(reg, gpbr);
	ret = count;

	return ret;
}

static struct kobj_attribute mode = __ATTR_RW(mode);

static ssize_t next_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	u32 next = __raw_readl(gpbr);
	next = next >> 16;

	return snprintf(buf, PAGE_SIZE, "%u\n", next);
}

static ssize_t next_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
	ssize_t ret;
	unsigned long next;
	u32 reg;

	reg = __raw_readl(gpbr);
	ret = kstrtoul(buf, 0, &next);
	if (ret)
		return ret;
	next = next << 16;
	reg &= 0xFFFF;
	reg |= next;
	__raw_writel(reg, gpbr);
	ret = count;

	return ret;
}

static struct kobj_attribute next = __ATTR_RW(next);

static struct attribute *kboot_attributes[] = {
	&mode.attr,
	&next.attr,
	NULL,
};

static struct attribute_group kboot_group = {
	.attrs = kboot_attributes,
};

static const struct of_device_id overkiz_at91_kboot_dt_ids[] = {
	{ .compatible = "overkiz,at91-kboot" },
	{ /* sentinel */ }
};

static int __init at91_kboot_init(void)
{
	int ret = 0;
	struct device_node *np;

	np = of_find_matching_node(NULL, overkiz_at91_kboot_dt_ids);
	if (!np)
		return -EINVAL;

	gpbr = of_iomap(np, 0);
	of_node_put(np);
	if (!gpbr)
		return -EINVAL;

	kboot = kobject_create_and_add("kboot", NULL);
	if (!kboot)
		return -ENOMEM;

	ret = sysfs_create_group(kboot, &kboot_group);
	if (ret) {
		kobject_del(kboot);
		return ret;
	}

	return ret;
}
module_init(at91_kboot_init);

static void __exit at91_kboot_exit(void)
{
	iounmap(gpbr);
	kobject_del(kboot);
}
module_exit(at91_kboot_exit);

MODULE_AUTHOR("Gaël PORTAY");
MODULE_DESCRIPTION("K-Boot driver for Atmel AT91");
MODULE_LICENSE("GPL");

