/*
 * Copyright (c) 2014-2018, Federico G. Schwindt <fgsch@lodoss.net>
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
#include <string.h>

#include <maxminddb.h>

#include "cache/cache.h"

#include "vsa.h"
#include "vsb.h"

#include "vcc_if.h"

#ifndef NO_VXID
#define NO_VXID 0
#endif

struct vmod_geoip2_geoip2 {
	unsigned		magic;
#define VMOD_GEOIP2_MAGIC	 	0x19800829
	MMDB_s			mmdb;
};

#define COMPONENT_MAX		10
#define LOOKUP_PATH_MAX		100


static void
vslv(VRT_CTX, enum VSL_tag_e tag, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (ctx->vsl)
		VSLbv(ctx->vsl, tag, fmt, ap);
	else
		VSLv(tag, NO_VXID, fmt, ap);
	va_end(ap);
}

VCL_VOID
vmod_geoip2__init(VRT_CTX, struct vmod_geoip2_geoip2 **vpp,
    const char *vcl_name, VCL_STRING filename)
{
	struct vmod_geoip2_geoip2 *vp;
	MMDB_s mmdb;
	int error;

	(void)vcl_name;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(vpp);
	AZ(*vpp);

	VSL(SLT_Debug, NO_VXID,
	    "geoip2.geoip2: Using maxminddb %s",
	    MMDB_lib_version());

	error = MMDB_open(filename, MMDB_MODE_MMAP, &mmdb);
	if (error != MMDB_SUCCESS) {
		char errstr[512];

		snprintf(errstr, sizeof(errstr),
		    "geoip2.geoip2: %s",
		    MMDB_strerror(error));
		VSL(SLT_Error, NO_VXID, "%s", errstr);
		/* Send the error to the CLI too. */
		VSB_printf(ctx->msg, "%s\n", errstr);
		return;
	}

	ALLOC_OBJ(vp, VMOD_GEOIP2_MAGIC);
	AN(vp);
	*vpp = vp;
	vp->mmdb = mmdb;
}

VCL_VOID
vmod_geoip2__fini(struct vmod_geoip2_geoip2 **vpp)
{
	struct vmod_geoip2_geoip2 *vp;

	AN(*vpp);
	vp = *vpp;
	*vpp = NULL;
	CHECK_OBJ_NOTNULL(vp, VMOD_GEOIP2_MAGIC);
	MMDB_close(&vp->mmdb);
	FREE_OBJ(vp);
}

static char *
printf_bytes(struct ws *ws, const uint8_t *bytes, uint32_t size,
    unsigned json)
{
	char *p;
	uint32_t i;

	p = WS_Alloc(ws, size * 2 + json * 2 + 1);
	if (p == NULL)
		return (p);
	for (i = 0; i < size; i++)
		sprintf(&p[i * 2 + json], "%02X", bytes[i]);
	if (json) {
		p[0] = '"';
		p[size * 2 + 1] = '"';
		p[size * 2 + 2] = '\0';
	}
	return (p);
}

VCL_STRING
vmod_geoip2_lookup(VRT_CTX, struct vmod_geoip2_geoip2 *vp,
    VCL_STRING path, VCL_IP addr, VCL_BOOL json)
{
	MMDB_lookup_result_s res;
	MMDB_entry_data_s data;
	const struct sockaddr *sa;
	socklen_t addrlen;
	const char **ap, *arrpath[COMPONENT_MAX];
	const char *fmt, *q;
	char buf[LOOKUP_PATH_MAX];
	char *p, *last;
	int error;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(vp, VMOD_GEOIP2_MAGIC);

	if (!path || !*path || strlen(path) >= sizeof(buf)) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: Invalid or missing path (%s)",
		    path ? path : "NULL");
		return (NULL);
	}

	if (!addr) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: Missing ip address");
		return (NULL);
	}

	sa = VSA_Get_Sockaddr(addr, &addrlen);
	AN(sa);

	res = MMDB_lookup_sockaddr(&vp->mmdb, sa, &error);
	if (error != MMDB_SUCCESS) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: MMDB_lookup_sockaddr: %s",
		    MMDB_strerror(error));
		return (NULL);
	}

	if (!res.found_entry) {
		vslv(ctx, SLT_Debug,
		    "geoip2.lookup: No entry for this IP address (%s)",
		    VRT_IP_string(ctx, addr));
		return (NULL);
	}

	strncpy(buf, path, sizeof(buf));

	last = NULL;
	for (p = buf, ap = arrpath; ap < &arrpath[COMPONENT_MAX - 1] &&
	    (*ap = strtok_r(p, "/", &last)) != NULL; p = NULL) {
		if (**ap != '\0')
			ap++;
	}
	*ap = NULL;

	error = MMDB_aget_value(&res.entry, &data, arrpath);
	if (error != MMDB_SUCCESS &&
	    error != MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: MMDB_aget_value: %s",
		    MMDB_strerror(error));
		return (NULL);
	}

	if (!data.has_data) {
		vslv(ctx, SLT_Debug,
		    "geoip2.lookup: No data for this path (%s)",
		    path);
		return (NULL);
	}

	switch (data.type) {
	case MMDB_DATA_TYPE_BOOLEAN:
		q = WS_Printf(ctx->ws, "%s", data.boolean ?
		    "true" : "false");
		break;

	case MMDB_DATA_TYPE_BYTES:
		q = printf_bytes(ctx->ws, data.bytes,
		    data.data_size, json);
		break;

	case MMDB_DATA_TYPE_DOUBLE:
		q = WS_Printf(ctx->ws, "%f", data.double_value);
		break;

	case MMDB_DATA_TYPE_FLOAT:
		q = WS_Printf(ctx->ws, "%f", data.float_value);
		break;

	case MMDB_DATA_TYPE_INT32:
		q = WS_Printf(ctx->ws, "%i", data.int32);
		break;

	case MMDB_DATA_TYPE_UINT16:
		q = WS_Printf(ctx->ws, "%u", data.uint16);
		break;

	case MMDB_DATA_TYPE_UINT32:
		q = WS_Printf(ctx->ws, "%u", data.uint32);
		break;

	case MMDB_DATA_TYPE_UINT64:
		q = WS_Printf(ctx->ws, "%ju", (uintmax_t)data.uint64);
		break;

	case MMDB_DATA_TYPE_UTF8_STRING:
		fmt = json ? "\"%.*s\"" : "%.*s";
		q = WS_Printf(ctx->ws, fmt, data.data_size,
		    data.utf8_string);
		break;

	default:
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: Unsupported data type (%d)",
		    data.type);
		return (NULL);
	}

	if (!q)
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: Out of workspace");

	return (q);
}
