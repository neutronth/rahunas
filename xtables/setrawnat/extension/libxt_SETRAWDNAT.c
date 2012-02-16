/*
 *	"SETRAWNAT" target extension for iptables
 *	Copyright 2012, Neutron Soutmun <neo.neutron@gmail.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License; either
 *	version 2 of the License, or any later version, as published by the
 *	Free Software Foundation.
 *
 *	Derived from RAWNAT written by Jan Engelhardt
 */
#include <netinet/in.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <xtables.h>
#include <linux/netfilter.h>
#include "xt_SETRAWNAT.h"
#include "compat_user.h"

enum {
	HAS_SETNAME = 1 << 0,
};

static const struct option setrawdnat_tg_opts[] = {
	{.name = "bind-set", .has_arg = true, .val = 'b'},
	{},
};

static void setrawdnat_tg_help(void)
{
	printf(
"SETRAWDNAT target options:\n"
"    --bind-set setname    binding to IPSet hash:iplookup type set\n"
);
}

static int
setrawdnat_tg_parse(int c, char **argv, int invert, unsigned int *flags,
                    const void *entry, struct xt_entry_target **target)
{
	struct xt_setrawnat_tginfo *info = (void *)(*target)->data;

	switch (c) {
	case 'b':
		strncpy (info->setname, optarg, IPSET_MAXNAMELEN);
		*flags |= HAS_SETNAME;
		return true;
	}
	return false;
}

static void setrawdnat_tg_check(unsigned int flags)
{
	if (!(flags & HAS_SETNAME))
		xtables_error(PARAMETER_PROBLEM, "SETRAWDNAT: "
			"\"--bind-set\" is required.");
}

static void
setrawdnat_tg_print(const void *entry, const struct xt_entry_target *target,
                    int numeric)
{
	const struct xt_setrawnat_tginfo *info = (const void *)target->data;

	printf(" bind-set %s ", info->setname);
}

static void
setrawdnat_tg_save(const void *entry, const struct xt_entry_target *target)
{
	const struct xt_setrawnat_tginfo *info = (const void *)target->data;

	printf(" --bind-set %s", info->setname);
}

static struct xtables_target setrawdnat_tg_reg[] = {
	{
		.version       = XTABLES_VERSION,
		.name          = "SETRAWDNAT",
		.revision      = 0,
		.family        = NFPROTO_IPV4,
		.size          = XT_ALIGN(sizeof(struct xt_setrawnat_tginfo)),
		.userspacesize = XT_ALIGN(sizeof(struct xt_userspace_setrawnat_tginfo)),
		.help          = setrawdnat_tg_help,
		.parse         = setrawdnat_tg_parse,
		.final_check   = setrawdnat_tg_check,
		.print         = setrawdnat_tg_print,
		.save          = setrawdnat_tg_save,
		.extra_opts    = setrawdnat_tg_opts,
	},
	{
		.version       = XTABLES_VERSION,
		.name          = "SETRAWDNAT",
		.revision      = 0,
		.family        = NFPROTO_IPV6,
		.size          = XT_ALIGN(sizeof(struct xt_setrawnat_tginfo)),
		.userspacesize = XT_ALIGN(sizeof(struct xt_userspace_setrawnat_tginfo)),
		.help          = setrawdnat_tg_help,
		.parse         = setrawdnat_tg_parse,
		.final_check   = setrawdnat_tg_check,
		.print         = setrawdnat_tg_print,
		.save          = setrawdnat_tg_save,
		.extra_opts    = setrawdnat_tg_opts,
	},
};

static void _init(void)
{
	xtables_register_targets(setrawdnat_tg_reg,
		sizeof(setrawdnat_tg_reg) / sizeof(*setrawdnat_tg_reg));
}
