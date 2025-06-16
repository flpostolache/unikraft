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
#include <uk/alloc_store.h>

static const char CMDLINE_VOLID[] = "memory_vol";

static char* read_memory(void)
{
	const struct uk_store_entry *entry;
	int rc __unused = 0;
	__u64 last_alloc_size, max_alloc_size, min_alloc_size,
		  tot_nb_allocs, tot_nb_frees, nb_enomem;
	__s64 cur_nb_allocs, max_nb_allocs, cur_mem_use,
		  max_mem_use;
	char *to_read = malloc(200 * sizeof(char));

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_LAST_ALLOC_SIZE);
	rc = uk_store_get_value(entry, u64, &last_alloc_size);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_MAX_ALLOC_SIZE);
	rc = uk_store_get_value(entry, u64, &max_alloc_size);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_MIN_ALLOC_SIZE);
	rc = uk_store_get_value(entry, u64, &min_alloc_size);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_TOTAL_NUM_ALLOCS);
	rc = uk_store_get_value(entry, u64, &tot_nb_allocs);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_TOTAL_NUM_FREES);
	rc = uk_store_get_value(entry, u64, &tot_nb_frees);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_CUR_NUM_ALLOCS);
	rc = uk_store_get_value(entry, s64, &cur_nb_allocs);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_MAX_NUM_ALLOCS);
	rc = uk_store_get_value(entry, s64, &max_nb_allocs);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_CUR_MEM_USE);
	rc = uk_store_get_value(entry, s64, &cur_mem_use);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_MAX_MEM_USE);
	rc = uk_store_get_value(entry, s64, &max_mem_use);

	entry = uk_store_static_entry_get(uk_libid("libukalloc"), UK_ALLOC_STATS_NUM_ENOMEM);
	rc = uk_store_get_value(entry, u64, &nb_enomem);


	sprintf(to_read, 
            "last_alloc_size:\t%ld\n"
            "max_alloc_size:\t\t%ld\n"
            "min_alloc_size:\t\t%ld\n"
            "tot_nb_allocs:\t\t%ld\n"
            "tot_nb_frees:\t\t%ld\n"
            "cur_nb_allocs:\t\t%ld\n"
            "max_nb_allocs:\t\t%ld\n"
            "cur_mem_use:\t\t%ld\n"
            "max_mem_use:\t\t%ld\n"
            "nb_enomem:\t\t%ld\n",
			last_alloc_size,
			max_alloc_size,
			min_alloc_size,
			tot_nb_allocs,
			tot_nb_frees,
			cur_nb_allocs,
			max_nb_allocs,
			cur_mem_use,
			max_mem_use,
			nb_enomem
			);
	return to_read;
}

static ssize_t serial_read(const struct uk_file *f,
			   const struct iovec *iov, int iovcnt,
			   off_t off, long flags __unused)
{
	ssize_t total = 0;
	char *value;
	int rc = 0;

	value = read_memory();

	//uk_pr_crit("%d\n", iovcnt);

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

static const struct uk_file_ops memory_ops = {
	.read = serial_read,
	.write = uk_file_nop_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl,
};

static uk_file_refcnt memory_ref = UK_FILE_REFCNT_INITIALIZER(memory_ref);

static struct uk_file_state memory_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	memory_state, UKFD_POLLIN|UKFD_POLLOUT);

static const struct uk_file memory_file = {
	.vol = CMDLINE_VOLID,
	.node = NULL,
	.ops = &memory_ops,
	.refcnt = &memory_ref,
	.state = &memory_state,
	._release = uk_file_static_release
};

const struct uk_file *uk_memory_create(void)
{
	const struct uk_file *f = &memory_file;

	uk_file_acquire(f);
	return f;
}
