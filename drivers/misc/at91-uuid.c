/*
 * KIZBOX3 driver for Atmel AT91
 *
 * Copyright (C) 2019 Kevin Carli
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

static void __iomem *sfrsn_base;
static struct kobject *uuid;

#define SFR_SN(x) (sfrsn_base + (x*4))
#define SFR_SN0 SFR_SN(0)
#define SFR_SN1 SFR_SN(1)

static u32 read_reg(void __iomem *addr, int index)
{
	return readl(addr);
}

static ssize_t snlow_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", readl(SFR_SN0));
}
static ssize_t snup_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", readl(SFR_SN1));
}

static struct kobj_attribute snlow   = __ATTR_RO(snlow);
static struct kobj_attribute snup = __ATTR_RO(snup);

static struct attribute *uuid_attributes[] = {
	&snlow.attr,
	&snup.attr,
	NULL
};

static struct attribute_group uuid_group = {
	.attrs = uuid_attributes,
};

static const struct of_device_id overkiz_at91_uuid_dt_ids[] = {
	{ .compatible = "overkiz,at91-uuid" },
	{ /* sentinel */ }
};

static int __init at91_uuid_init(void)
{
	int ret = 0;
	struct device_node *np;
	struct resource res;

	sfrsn_base = NULL;
	uuid = NULL;

	np = of_find_matching_node(NULL, overkiz_at91_uuid_dt_ids);
	if (!np)
		return -EINVAL;

	if (of_address_to_resource(np, 0, &res) == 0)
	{
		if(resource_size(&res) >= (2*4))
		{
			sfrsn_base = ioremap(res.start, resource_size(&res));
		}
		else
		{
			printk(KERN_ERR "at91-uuid unsupported size %d\n",resource_size(&res));
		}
	}

	of_node_put(np);
	if (!sfrsn_base)
		return -EINVAL;

	uuid = kobject_create_and_add("uuid", NULL);
	if (!uuid)
		return -ENOMEM;

	ret = sysfs_create_group(uuid, &uuid_group);
	if (ret) {
		kobject_del(uuid);
		return ret;
	}

	return ret;
}

static void __exit at91_uuid_exit(void)
{
	sysfs_remove_group(uuid, &uuid_group);
	iounmap(sfrsn_base);
	sfrsn_base = NULL;
	kobject_del(uuid);
	uuid = NULL;
}

module_init(at91_uuid_init);
module_exit(at91_uuid_exit);

MODULE_AUTHOR("Kevin Carli");
MODULE_DESCRIPTION("KIZBOX3 driver to get serial number");
MODULE_LICENSE("GPL");

