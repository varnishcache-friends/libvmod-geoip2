/*-
 * Copyright (c) 2006 Verdens Gang AS
 * Copyright (c) 2006-2011 Varnish Software AS
 * All rights reserved.
 *
 * Portions Copyright 2017 UPLEX - Nils Goroll Systemoptimierung
 *
 * Authors: Poul-Henning Kamp <phk@phk.freebsd.dk>
 *	    Nils Goroll <nils.goroll@uplex.de>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cache/cache.h"
#include "common/common_param.h"
#include "ws_ext.h"

/*
  these extensions are designed to work with a future change to
  drop the alignment requirement on the f pointer when the ws
  is reserved

@@ -50,15 +50,30 @@ WS_Assert(const struct ws *ws)
        assert(ws->s < ws->e);
        assert(ws->f >= ws->s);
        assert(ws->f <= ws->e);
-       assert(PAOK(ws->f));
        if (ws->r) {
                assert(ws->r > ws->s);
                assert(ws->r <= ws->e);
                assert(PAOK(ws->r));
+       } else {
+               assert(PAOK(ws->f));
        }
        assert(*ws->e == 0x15);
 }
*/

void
WS_Assert_Open(const struct ws *ws)
{
	// WS_Assert(ws);
	assert(ws->r != NULL);
}

void
WS_Assert_Closed(const struct ws *ws)
{
	WS_Assert(ws);
	assert(ws->r == NULL);
}

/*
 * open the ws in reserved mode. ws->f is now the "current write pointer".
 *
 * returns write pointer (ws->f) or NULL
 */

char *
WS_Open(struct ws *ws)
{
	WS_Assert_Closed(ws);

	/* at least one byte payload + one byte terminator */
	if (ws->f >= ws->e - 2 ||
	    WS_Overflowed(ws))
		return (NULL);

	ws->r = ws->e;
	DSL(DBG_WORKSPACE, 0, "WS_Open(%p) = %p", ws, ws->f);
	return (ws->f);
}

/*
 * end reserved mode, null-terminate, align ws->f
 */
void
WS_Close(struct ws *ws)
{
	WS_Assert_Open(ws);
	if (ws->f > ws->s && ws->f[-1] != '\0') {
		assert(ws->f < ws->r);
		*(ws->f++) = '\0';
	}
	ws->f = (char *)PRNDUP(ws->f);
	ws->r = NULL;
	DSL(DBG_WORKSPACE, 0, "WS_Close(%p)", ws);
}

/*
 * space left
 * if open, reserving one byte for null terminator
 */

size_t
WS_Space(struct ws *ws)
{
	// WS_Assert(ws);
	return (ws->r ? ( ws->r - ws->f - 1) : ( ws->e - ws->f ));
}

/*
 * return pointer of copy (former ws->f)
 */

static inline char *
ws_cpy(struct ws *ws, const char *s, size_t l)
{
	char *r;

	if (WS_Space(ws) < l) {
		WS_MarkOverflow(ws);
		return(NULL);
	}
	r = ws->f;
	memcpy(r, s, l);
	ws->f += l;
	*ws->f = '\0';

	return (r);
}

/*
 void *
 WS_Copy(struct ws *ws, const void *str, int len)
 {
        char *r;
-       unsigned bytes;
+       size_t bytes;
 
        ws_Assert_Closed(ws);
+       AN(str);
 
        if (len == -1)
                len = strlen(str) + 1;
        assert(len >= 0);
 
        bytes = PRNDUP((unsigned)len);
-       if (ws->f + bytes > ws->e) {
-               WS_MarkOverflow(ws);
-               WS_Assert(ws);
-               return(NULL);
-       }
-       r = ws->f;
-       ws->f += bytes;
-       memcpy(r, str, len);
+
+       r = ws_cpy(ws, str, bytes);
        DSL(DBG_WORKSPACE, 0, "WS_Copy(%p, %d) = %p", ws, len, r);
        WS_Assert(ws);
        return (r);
 }
*/

/*
 * when open, return write tail (ws->f)
 */

char *
WS_Tail(struct ws *ws)
{
	WS_Assert_Open(ws);
	return (ws->f);
}

/*
 * when open, advance the tail by l
 */
void
WS_Advance(struct ws *ws, size_t l)
{
	WS_Assert_Open(ws);
	assert(ws->f + l < ws->r);
	ws->f += l;
}

/*
 * when open, restore tail
 *
 */
void
WS_Restore(struct ws *ws, char *r)
{
	WS_Assert_Open(ws);
	assert(r >= ws->s);
	assert(r < ws->f);
	ws->f = r;
}

/*
 * when open, append s of length l
 *
 * return pointer to start of appended data or NULL if no space
 */
char *
WS_Append(struct ws *ws, const char *s, size_t l)
{
	char *r;

	WS_Assert_Open(ws);
	AN(s);

	r = ws_cpy(ws, s, l);
	DSL(DBG_WORKSPACE, 0, "WS_Append(%p, %zu) = %p", ws, l, r);
	return (r);
}

/*
 * when open, printf append
 *
 * return pointer to start of appended data or NULL if no space
 */
void *
WS_Append_Printf(struct ws *ws, const char *fmt, ...)
{
	size_t u, v;
	va_list ap;
	char *p;

	WS_Assert_Open(ws);

	p = ws->f;
	/* vsnprintf is null-terminating */
	u = WS_Space(ws) + 1;
	va_start(ap, fmt);
	v = vsnprintf(p, u, fmt, ap);
	va_end(ap);
	if (v >= u) {
		WS_MarkOverflow(ws);
		p = NULL;
	} else
		WS_Advance(ws, v);
	return (p);
}
