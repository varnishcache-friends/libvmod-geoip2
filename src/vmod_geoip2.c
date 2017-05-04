/*
 * Copyright (c) 2014-2016, Federico G. Schwindt <fgsch@lodoss.net>
 * All rights reserved.
 *
 * Portions Copyright 2017 UPLEX - Nils Goroll Systemoptimierung
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

#include "config.h"
#include <stdlib.h>
#include <errno.h>

#include <maxminddb.h>

#include "cache/cache.h"

#include "vrt.h"
#include "vsa.h"

#include "vcc_if.h"

#include "ws_ext.h"

#ifndef VRT_CTX
#define VRT_CTX		const struct vrt_ctx *ctx
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
		VSLv(tag, 0, fmt, ap);
	va_end(ap);
}

VCL_VOID __match_proto__(td_geoip2_geoip2__init)
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

	VSL(SLT_Debug, 0, "geoip2.geoip2: Using maxminddb %s",
	    MMDB_lib_version());

	error = MMDB_open(filename, MMDB_MODE_MMAP, &mmdb);
	if (error != MMDB_SUCCESS) {
		VSL(SLT_Error, 0, "geoip2.geoip2: %s",
		    MMDB_strerror(error));
		return;
	}

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

static char *
geoip2_format(VRT_CTX, MMDB_entry_data_s *data)
{
	char *p;
	uint32_t i;

	AN(data);
	switch (data->type) {
	case MMDB_DATA_TYPE_BOOLEAN:
		p = WS_Append_Printf(ctx->ws, "%s", data->boolean ?
		    "true" : "false");
		break;

	case MMDB_DATA_TYPE_BYTES:
		if (WS_Space(ctx->ws) < data->data_size * 2) {
			p = NULL;
			break;
		}
		p = WS_Tail(ctx->ws);
		for (i = 0; i < data->data_size; i++)
			sprintf(&p[i * 2], "%02X", data->bytes[i]);
		WS_Advance(ctx->ws, data->data_size * 2);
		break;

	case MMDB_DATA_TYPE_DOUBLE:
		p = WS_Append_Printf(ctx->ws, "%f", data->double_value);
		break;

	case MMDB_DATA_TYPE_FLOAT:
		p = WS_Append_Printf(ctx->ws, "%f", data->float_value);
		break;

	case MMDB_DATA_TYPE_INT32:
		p = WS_Append_Printf(ctx->ws, "%i", data->int32);
		break;

	case MMDB_DATA_TYPE_UINT16:
		p = WS_Append_Printf(ctx->ws, "%u", data->uint16);
		break;

	case MMDB_DATA_TYPE_UINT32:
		p = WS_Append_Printf(ctx->ws, "%u", data->uint32);
		break;

	case MMDB_DATA_TYPE_UINT64:
		p = WS_Append_Printf(ctx->ws, "%ju", (uintmax_t)data->uint64);
		break;

	case MMDB_DATA_TYPE_UTF8_STRING:
		p = WS_Append(ctx->ws, data->utf8_string, data->data_size);
		break;

	default:
		errno = EINVAL;
		return (NULL);
	}
	return (p);
}

VCL_STRING __match_proto__(td_geoip2_geoip2_lookup)
vmod_geoip2_lookup(VRT_CTX, struct vmod_geoip2_geoip2 *vp,
    VCL_STRING path, VCL_IP addr)
{
	MMDB_lookup_result_s res;
	MMDB_entry_data_s data;
	const struct sockaddr *sa;
	socklen_t addrlen;
	const char **ap, *arrpath[COMPONENT_MAX];
	char buf[LOOKUP_PATH_MAX];
	char *p, *last;
	int error;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(addr);

	if (!vp) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: Database not open");
		return (NULL);
	}

	if (!path || !*path || strlen(path) >= sizeof(buf)) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: Invalid or missing path (%s)",
		    path ? path : "NULL");
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

	p = NULL;
	if (WS_Open(ctx->ws)) {
		errno = 0;
		p = geoip2_format(ctx, &data);
		WS_Close(ctx->ws);
		if (p == NULL && errno == EINVAL) {
			vslv(ctx, SLT_Error,
			    "geoip2.lookup: Unsupported data type (%d)",
			    data.type);
			return (p);
		}
	}

	if (!p)
		vslv(ctx, SLT_Error,
		    "geoip2.lookup: Out of workspace");

	return (p);
}

#define geoip2_lf_err(ctx, w, what, err, errv) do {			\
		vslv((ctx), SLT_Error,				\
		    "geoip2.lookup_fields(%s) %s: %s(%d)",		\
		    (w), (what), (err), (errv));			\
		w = WS_Append(ctx->ws, "?", 1); 			\
	} while (0)

#define geoip2_lf_dbg(ctx, w, what, err, errv) do {		\
		vslv((ctx), SLT_Debug,				\
		    "geoip2.lookup_fields(%s) %s: %s(%d)",		\
		    (w), (what), (err), (errv));			\
		w = WS_Append(ctx->ws, "?", 1); 			\
	} while (0)

VCL_STRING __match_proto__(td_geoip2_geoip2_lookup_fields)
vmod_geoip2_lookup_fields(VRT_CTX, struct vmod_geoip2_geoip2 *vp,
    VCL_STRING path, VCL_STRING list, VCL_STRING sep)
{
	MMDB_lookup_result_s res;
	MMDB_entry_data_s data;
	const char **ap, *arrpath[COMPONENT_MAX];
	char buf[LOOKUP_PATH_MAX];
	char *p, *last, *w;
	const char *out, *s, *e;
	int mmdb_error, gai_error;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (!vp) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup_fields: Database not open");
		return (NULL);
	}

	if (!path || !*path || strlen(path) >= sizeof(buf)) {
		vslv(ctx, SLT_Error,
		    "geoip2.lookup_fields: Invalid or missing path (%s)",
		    path ? path : "NULL");
		return (NULL);
	}

	if (list == NULL)
		return NULL;

	if (sep == NULL)
		sep = ",";

	strncpy(buf, path, sizeof(buf));

	last = NULL;
	for (last = NULL, p = buf, ap = arrpath;
	     ap < &arrpath[COMPONENT_MAX - 1] &&
		 (*ap = strtok_r(p, "/", &last)) != NULL;
	     p = NULL) {
		if (**ap != '\0')
			ap++;
	}
	*ap = NULL;

	out = w = WS_Open(ctx->ws);

	/*
	 * we copy everything until the next separator onto our open workspace
	 * and use the copied address for the lookup
	 */
	while (w && *list) {
		/* append separators */
		s = list;
		list += strspn(list, sep);
		if (s != list) {
			assert (list > s);
			w = WS_Append(ctx->ws, s, list - s);
			s = list;
		}

		list += strcspn(list, sep);
		e = list;
		if (s == e)
			break;

		while (e > s && (e[-1] == ' ' || e[-1] == '\t'))
			e--;

		if (s == e) {
			/* only whitespace, preserve */
			w = WS_Append(ctx->ws, s, list - s);
			continue;
		}

		w = WS_Append(ctx->ws, s, e - s);
		if (w == NULL)
			break;

		while (*w == ' ' || *w == '\t')
			w++;

		/* all-whitespace case handled above */
		assert(w < WS_Tail(ctx->ws));

		res = MMDB_lookup_string(&vp->mmdb, w,
		    &gai_error, &mmdb_error);

		/* move ws tail back to start of ip address */
		WS_Restore(ctx->ws, w);

		if (gai_error != 0) {
			geoip2_lf_err(ctx, w, "getaddrinfo",
			    gai_strerror(gai_error), gai_error);
		} else if (mmdb_error != MMDB_SUCCESS) {
			geoip2_lf_err(ctx, w, "mmdb",
			    MMDB_strerror(mmdb_error), mmdb_error);
		} else if (!res.found_entry) {
			geoip2_lf_err(ctx, w, "mmdb",
			    "no entry found", 0);
		} else {
			mmdb_error =
			    MMDB_aget_value(&res.entry, &data, arrpath);
			if (mmdb_error != MMDB_SUCCESS &&
			    mmdb_error != MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR) {
				geoip2_lf_err(ctx, w, "mmdb_aget",
				    MMDB_strerror(mmdb_error), mmdb_error);
			} else if (!data.has_data) {
				geoip2_lf_dbg(ctx, w, "no data",
				    path, 0);
			} else {
				errno = 0;
				last = w;
				w = geoip2_format(ctx, &data);
				if (w == NULL && errno == EINVAL) {
					geoip2_lf_err(ctx, last, "format",
					    "unsupported data type", data.type);
					w = last;
				}
			}
		}

		/* append remaining whitespace */
		if (w && e != list) {
			assert (list > e);
			w = WS_Append(ctx->ws, e, list - e);
		}
	}

	WS_Close(ctx->ws);

	if (w == NULL)
		vslv(ctx, SLT_Error,
		    "geoip2.lookup_fields: Out of workspace");

	return (out);
}
