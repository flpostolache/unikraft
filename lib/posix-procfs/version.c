/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <uk/assert.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/plat/console.h>
#include <uk/posix-fd.h>
#include <uk/posix-procfs.h>
#include <uk/store.h>
#include <uk/version.h>

static const char CMDLINE_VOLID[] = "version_vol";

static int read_version(char *buff, int max_size)
{
	const struct uk_store_entry *entry;
	int rc;

	entry = uk_store_static_entry_get(uk_libid("libukboot"), UK_VERSION_STATS);
	rc = _uk_store_get_ncharp(entry, buff, max_size);
	
	return rc;
}

static ssize_t serial_read(const struct uk_file *f,
			   const struct iovec *iov, int iovcnt,
			   off_t off, long flags __unused)
{
	ssize_t total = 0;
	char *value = calloc(sizeof(char), 100);
	int rc = 0;

	rc = read_version(value, 100);

	for (int i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;
		char *last;
		int bytes_read;

		if (unlikely(!buf && len))
			return -EFAULT;

		bytes_read = strlen(value);
		strcpy(buf, value);
		if (!bytes_read)
			break;
		if (unlikely(bytes_read < 0))
			return bytes_read;

		total += bytes_read;

		last = buf + bytes_read - 1;
		if (*last == '\r')
			*last = '\n';

		uk_file_event_clear(f, UKFD_POLLIN);
	}

    return 0;
}

static const struct uk_file_ops version_ops = {
	.read = serial_read,
	.write = uk_file_nop_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static uk_file_refcnt version_ref = UK_FILE_REFCNT_INITIALIZER(version_ref);

static struct uk_file_state version_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	version_state, UKFD_POLLIN|UKFD_POLLOUT);

static const struct uk_file version_file = {
	.vol = CMDLINE_VOLID,
	.node = NULL,
	.ops = &version_ops,
	.refcnt = &version_ref,
	.state = &version_state,
	._release = uk_file_static_release
};

const struct uk_file *uk_version_create(void)
{
	const struct uk_file *f = &version_file;

	uk_file_acquire(f);
	return f;
}
