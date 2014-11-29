/*
 * Copyright (c) 2014, Federico G. Schwindt <fgsch@lodoss.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>

#include <maxminddb.h>

#include "cache/cache.h"

#include "vrt.h"
#include "vsa.h"

#include "vcc_if.h"

#ifndef VRT_CTX
#define VRT_CTX		const struct vrt_ctx *ctx
#endif

struct vmod_geoip2_geoip2 {
	unsigned		magic;
#define VMOD_GEOIP2_MAGIC	 	0x19800829
	MMDB_s			mmdb;
};


VCL_VOID __match_proto__(td_geoip2_geoip2__init)
vmod_geoip2__init(VRT_CTX, struct vmod_geoip2_geoip2 **vpp,
    const char *vcl_name, VCL_STRING filename)
{
	struct vmod_geoip2_geoip2 *vp;
	MMDB_s mmdb;

	(void)vcl_name;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(vpp);
	AZ(*vpp);

	if (MMDB_open(filename, MMDB_MODE_MMAP, &mmdb) != MMDB_SUCCESS)
		return;

	ALLOC_OBJ(vp, VMOD_GEOIP2_MAGIC);
	AN(vp);
	*vpp = vp;
	vp->mmdb = mmdb;
}

VCL_VOID __match_proto__(td_geoip2_geoip2__fini)
vmod_geoip2__fini(struct vmod_geoip2_geoip2 **vpp)
{
	struct vmod_geoip2_geoip2 *vp;

	if (!*vpp)
		return;

	vp = *vpp;
	*vpp = NULL;
	CHECK_OBJ_NOTNULL(vp, VMOD_GEOIP2_MAGIC);
	MMDB_close(&vp->mmdb);
	FREE_OBJ(vp);
}

VCL_STRING __match_proto__(td_geoip2_geoip2_lookup)
vmod_geoip2_lookup(VRT_CTX, struct vmod_geoip2_geoip2 *vp,
    VCL_STRING lookup_path, VCL_IP addr)
{
	MMDB_lookup_result_s res;
	MMDB_entry_data_s data;
	const struct sockaddr *sa;
	socklen_t addrlen;
	const char **ap, *path[10];
	char buf[100];
	char *p, *last;
	unsigned u, v;
	int error;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (!vp)
		return (NULL);

	if (!lookup_path || !addr || strlen(lookup_path) >= sizeof(buf))
		return (NULL);

	sa = VSA_Get_Sockaddr(addr, &addrlen);
	if (!sa)
		return (NULL);

	strncpy(buf, lookup_path, sizeof(buf));

	for (p = buf, ap = path; ap < &path[9] &&
	    (*ap = strtok_r(p, "/", &last)) != NULL; p = NULL) {
		if (**ap != '\0')
			ap++;
	}
	*ap = NULL;

	res = MMDB_lookup_sockaddr(&vp->mmdb, sa, &error);
	if (error != MMDB_SUCCESS || !res.found_entry)
		return (NULL);

	error = MMDB_aget_value(&res.entry, &data, path);
	if (error != MMDB_SUCCESS || !data.has_data)
		return (NULL);

	u = WS_Reserve(ctx->ws, 0);
	p = ctx->ws->f;

	switch (data.type) {
	case MMDB_DATA_TYPE_BOOLEAN:
		v = snprintf(p, u, "%s", data.boolean ?  "true" : "false");
		break;

	case MMDB_DATA_TYPE_UINT16:
		v = snprintf(p, u, "%u", data.uint16);
		break;

	case MMDB_DATA_TYPE_UINT32:
		v = snprintf(p, u, "%u", data.uint32);
		break;

	case MMDB_DATA_TYPE_INT32:
		v = snprintf(p, u, "%i", data.int32);
		break;

	case MMDB_DATA_TYPE_UINT64:
		v = snprintf(p, u, "%ju", (uintmax_t)data.uint64);
		break;

	case MMDB_DATA_TYPE_FLOAT:
		v = snprintf(p, u, "%f", data.float_value);
		break;

	case MMDB_DATA_TYPE_DOUBLE:
		v = snprintf(p, u, "%f", data.double_value);
		break;

	case MMDB_DATA_TYPE_UTF8_STRING:
		v = data.data_size;
		if (v < u) {
			memcpy(p, data.utf8_string, v);
			p[v] = '\0';
		}
		break;

	default:
		/* Unsupported type */
		v = 0;
		break;
	}

	if (!v || v >= u) {
		WS_Release(ctx->ws, 0);
		return (NULL);
	} else {
		WS_Release(ctx->ws, v + 1);
		return (p);
	}
}
