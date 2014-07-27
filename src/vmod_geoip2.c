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

#include <inttypes.h>
#include <stdlib.h>

#include <maxminddb.h>

#include "cache/cache.h"

#include "vrt.h"
#include "vsa.h"

#include "vcc_if.h"

struct vmod_geoip2 {
	unsigned		magic;
#define VMOD_GEOIP2_MAGIC	 	0x19800829
	MMDB_s			mmdb;
};


static void
vmod_free(void *priv)
{
	struct vmod_geoip2 *v;

	CAST_OBJ_NOTNULL(v, priv, VMOD_GEOIP2_MAGIC);

	if (v->mmdb.filename)
		MMDB_close(&v->mmdb);

	FREE_OBJ(v);
}

VCL_VOID __match_proto__(td_geoip2_init)
vmod_init(const struct vrt_ctx *ctx, struct vmod_priv *priv,
    VCL_STRING filename)
{
	struct vmod_geoip2 *v;
	MMDB_s mmdb;

	(void)ctx;

	AN(priv);

	if (MMDB_open(filename, MMDB_MODE_MMAP, &mmdb) != MMDB_SUCCESS)
		return;

	ALLOC_OBJ(v, VMOD_GEOIP2_MAGIC);
	AN(v);

	v->mmdb = mmdb;

	priv->free = vmod_free;
	priv->priv = v;
}

static char *
vmod_lookup_common(const struct vrt_ctx *ctx, struct vmod_priv *priv,
    VCL_IP addr, const char **lookup_path)
{
	MMDB_lookup_result_s result;
	MMDB_entry_data_list_s *entry_data_list;
	MMDB_entry_data_s entry_data;
	MMDB_entry_s entry;
	struct vmod_geoip2 *v;
	const struct sockaddr *sa;
	socklen_t addrlen;
	int error, size;
	char *p;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	AN(priv);

	CAST_OBJ_NOTNULL(v, priv->priv, VMOD_GEOIP2_MAGIC);

	if (!v->mmdb.filename)
		return (NULL);

	sa = VSA_Get_Sockaddr(addr, &addrlen);
	if (!sa)
		return (NULL);

        result = MMDB_lookup_sockaddr(&v->mmdb, sa, &error);
	if (error != MMDB_SUCCESS || !result.found_entry)
		return (NULL);

	error = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
	if (error != MMDB_SUCCESS || !entry_data.offset)
		return (NULL);

	entry.mmdb = &v->mmdb;
	entry.offset = entry_data.offset;
	error = MMDB_get_entry_data_list(&entry, &entry_data_list);
	if (error != MMDB_SUCCESS)
		return (NULL);

	switch (entry_data_list->entry_data.type) {
	case MMDB_DATA_TYPE_BOOLEAN:
		p = WS_Printf(ctx->ws, "%s",
		    entry_data_list->entry_data.boolean ?
		    "true" : "false");
		break;

	case MMDB_DATA_TYPE_UINT16:
		p = WS_Printf(ctx->ws, "%u",
		    entry_data_list->entry_data.uint16);
		break;

	case MMDB_DATA_TYPE_UINT32:
		p = WS_Printf(ctx->ws, "%u",
		    entry_data_list->entry_data.uint32);
		break;

	case MMDB_DATA_TYPE_INT32:
		p = WS_Printf(ctx->ws, "%i",
		    entry_data_list->entry_data.int32);
		break;

	case MMDB_DATA_TYPE_UINT64:
		p = WS_Printf(ctx->ws, "%" PRIu64,
		    entry_data_list->entry_data.uint64);
		break;

	case MMDB_DATA_TYPE_FLOAT:
		p = WS_Printf(ctx->ws, "%f",
		    entry_data_list->entry_data.float_value);
		break;

	case MMDB_DATA_TYPE_DOUBLE:
		p = WS_Printf(ctx->ws, "%f",
		    entry_data_list->entry_data.double_value);
		break;

	case MMDB_DATA_TYPE_UTF8_STRING:
		size = entry_data_list->entry_data.data_size;
		p = WS_Alloc(ctx->ws, size + 1);
		if (p) {
			memcpy(p, entry_data_list->entry_data.utf8_string,
			    size);
			p[size] = '\0';
		}
		break;
	}

	MMDB_free_entry_data_list(entry_data_list);
	return (p);
}

VCL_STRING __match_proto__(td_geoip2_lookup)
vmod_lookup(const struct vrt_ctx *ctx, struct vmod_priv *priv, VCL_IP addr,
    VCL_STRING path)
{
	const char **ap, *argv[10];
	char *p, *s;
	char str[100];

	if (!path || strlen(path) >= sizeof(str))
		return (NULL);

	strncpy(str, path, sizeof(str));

	for (p = str, ap = argv; ap < &argv[9] &&
	    (*ap = strtok_r(p, "/", &s)) != NULL; p = NULL) {
		if (**ap != '\0')
			ap++;
	}
	*ap = NULL;

	return (vmod_lookup_common(ctx, priv, addr, argv));
}

VCL_VOID __match_proto__(td_geoip2_close)
vmod_close(const struct vrt_ctx *ctx, struct vmod_priv *priv)
{
	(void)ctx;

	AN(priv);

	if (priv->priv) {
		vmod_free(priv->priv);
		priv->priv = NULL;
		priv->free = NULL;
	}
}
