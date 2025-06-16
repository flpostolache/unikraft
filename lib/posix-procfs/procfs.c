/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/init.h>
#include <uk/posix-fd.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-procfs.h>

static int init_posix_procfs(struct uk_init_ctx *ictx __unused)
{
	const struct uk_file *version, *memory;

	version = uk_version_create();
	uk_fdtab_open(version, O_RDONLY|UKFD_O_NOSEEK);

	memory = uk_memory_create();
	uk_fdtab_open(memory, O_RDONLY|UKFD_O_NOSEEK);

	return 0;
}

uk_rootfs_initcall_prio(init_posix_procfs, 0x0, UK_PRIO_LATEST);