/*-
 * Copyright (c) 2008-2011 Juan Romero Pardines.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <assert.h>

#include <xbps_api.h>
#include "../xbps-bin/defs.h"
#include "defs.h"
#include "config.h"

int
show_pkg_info_from_repolist(const char *pkgname, const char *option)
{
	prop_dictionary_t pkgd;

	pkgd = xbps_repository_pool_find_virtualpkg(pkgname, false, false);
	if (pkgd == NULL) {
		pkgd = xbps_repository_pool_find_pkg(pkgname, false, false);
		if (pkgd == NULL)
			return errno;
	}

	if (option)
		show_pkg_info_one(pkgd, option);
	else
		show_pkg_info(pkgd);

	prop_object_release(pkgd);

	return 0;
}

int
show_pkg_deps_from_repolist(const char *pkgname)
{
	prop_dictionary_t pkgd;
	const char *ver, *repoloc;

	pkgd = xbps_repository_pool_find_virtualpkg(pkgname, false, false);
	if (pkgd == NULL) {
		pkgd = xbps_repository_pool_find_pkg(pkgname, false, false);
		if (pkgd == NULL)
			return errno;
	}

	prop_dictionary_get_cstring_nocopy(pkgd, "version", &ver);
	prop_dictionary_get_cstring_nocopy(pkgd, "repository", &repoloc);

	printf("Repository %s [pkgver: %s]\n", repoloc, ver);
	(void)xbps_callback_array_iter_in_dict(pkgd,
	    "run_depends", list_strings_sep_in_array, NULL);

	prop_object_release(pkgd);
	return 0;
}

static int
repo_sync_pkg_index_cb(struct repository_pool_index *rpi, void *arg, bool *done)
{
	prop_dictionary_t d;
	const char *idxver;
	uint64_t totalpkgs;
	char *plist;
	int rv = 0;

	(void)arg;
	(void)done;

	if ((rv = xbps_repository_sync_pkg_index(rpi->rpi_uri)) == 0) {
		printf("Package index file is up to date.\n");
		return 0;
	} else if (rv == -1)
		return rv;

	if ((plist = xbps_pkg_index_plist(rpi->rpi_uri)) == NULL)
		return EINVAL;

	d = prop_dictionary_internalize_from_zfile(plist);
	prop_dictionary_get_cstring_nocopy(d, "pkgindex-version", &idxver);
	prop_dictionary_get_uint64(d, "total-pkgs", &totalpkgs);
	printf("Updated package index at %s (v%s) with %ju packages.\n",
	    rpi->rpi_uri, idxver, totalpkgs);
	prop_object_release(d);
	free(plist);

	return 0;
}

int
repository_sync(void)
{
	return xbps_repository_pool_foreach(repo_sync_pkg_index_cb, NULL);
}
