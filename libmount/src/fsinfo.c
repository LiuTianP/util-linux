/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libmount from util-linux project.
 *
 * Copyright (C) 2020 Karel Zak <kzak@redhat.com>
 *
 * libmount is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */
#include "mountP.h"

#include <sys/syscall.h>
#include <linux/mount.h>

/* libc fallback */
#ifndef HAVE_FSINFO
# ifndef __NR_fsinfo
#  define __NR_fsinfo -1
# endif
static ssize_t fsinfo(int dfd, const char *filename,
	       struct fsinfo_params *params, size_t params_size,
	       void *result_buffer, size_t result_buf_size)
{
	return syscall(__NR_fsinfo, dfd, filename,
		       params, params_size,
		       result_buffer, result_buf_size);
}
#endif /* HAVE_FSINFO */

int mnt_get_target_id(const char *path, unsigned int *id, unsigned int flags)
{
	struct fsinfo_mount_info info;
	struct fsinfo_params params = {
		.flags		= FSINFO_FLAGS_QUERY_PATH,
		.request	= FSINFO_ATTR_MOUNT_INFO,
		.at_flags	= flags,
	};
	int rc;

	errno = 0;
	rc = fsinfo(AT_FDCWD, path,
		    &params, sizeof(params), &info, sizeof(info));
	if (rc == -1)
		rc = -errno;
	else
		*id = info.mnt_id;
	return rc;
}

/*
 * Call fsinfo(), fill @buf with the result, on success update @bufsz
 * to the real result size.
 */
int mnt_get_id_fsinfo(	unsigned int id,
			struct fsinfo_params *params,
			size_t params_size,
			char *buf,
			size_t *bufsz)
{
	char idstr[sizeof(stringify_value(UINT_MAX))];
	ssize_t res;
	int rc = 0;

	assert(buf);
	assert(bufsz);
	assert(params);

	snprintf(idstr, sizeof(idstr), "%u", id);

	res = fsinfo(AT_FDCWD, idstr, params, params_size, buf, *bufsz);
	if (res < 0)
		rc = res;
	if ((size_t) res >= *bufsz)
		rc = -ENAMETOOLONG;
	if (rc == 0)
		*bufsz = res;
	return rc;
}
