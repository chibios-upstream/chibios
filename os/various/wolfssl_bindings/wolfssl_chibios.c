/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/*
 * **** This file incorporates work covered by the following copyright and ****
 * **** permission notice:                                                 ****
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 *
 */

#include <string.h>

#include "ch.h"

#include "wolfssl_sslconn.h"
/* For the LowResTimer() and TimeNowInMilliseconds() prototypes.*/
#include "wolfssl/internal.h"
#include "lwip/tcp.h"

static int wolfssl_is_initialized = 0;

sslconn *sslconn_accept(sslconn *sk)
{
  sslconn *new;
  struct netconn *newconn = NULL;
  err_t err;

  err = netconn_accept(sk->conn, &newconn);
  if (err != ERR_OK) {
    return NULL;
  }

  new = chHeapAlloc(NULL, sizeof(sslconn));
  if (new == NULL) {
    netconn_delete(newconn);
    return NULL;
  }
  memset(new, 0, sizeof(sslconn));
  new->conn = newconn;
  new->ctx = sk->ctx;
  new->ssl = wolfSSL_new(new->ctx);
  if (new->ssl == NULL) {
    netconn_delete(newconn);
    chHeapFree(new);
    return NULL;
  }
  wolfSSL_SetIOReadCtx(new->ssl, new);
  wolfSSL_SetIOWriteCtx(new->ssl, new);

  if (wolfSSL_accept(new->ssl) == SSL_SUCCESS) {
    wolfSSL_set_using_nonblock(new->ssl, 1);
    newconn->pcb.tcp->mss = 1480;
    return new;
  }

  /* Handshake failed, the connection is released.*/
  sslconn_close(new);
  return NULL;
}

sslconn *sslconn_new(enum netconn_type t, WOLFSSL_METHOD* method)
{
    sslconn *sk;
    if (!wolfssl_is_initialized) {
        wolfSSL_Init();
        wolfssl_is_initialized++;
    }

    sk = chHeapAlloc(NULL, sizeof(sslconn));
    if (!sk)
        return NULL;
    memset(sk, 0, sizeof(sslconn));
    sk->ctx = wolfSSL_CTX_new(method);
    if (!sk->ctx)
        goto error;
    sk->conn = netconn_new(t);
    if (!sk->conn)
        goto error;
    wolfSSL_SetIORecv(sk->ctx, wolfssl_recv_cb);
    wolfSSL_SetIOSend(sk->ctx, wolfssl_send_cb);
    return sk;

error:
    if (sk->ctx)
        wolfSSL_CTX_free(sk->ctx);
    chHeapFree(sk);
    return NULL;
}

void sslconn_close(sslconn *sk)
{
    /* Sends close_notify while the connection is still open.*/
    if (sk->ssl != NULL)
        (void)wolfSSL_shutdown(sk->ssl);
    if (sk->rxbuf != NULL)
        netbuf_delete(sk->rxbuf);
    netconn_delete(sk->conn);
    wolfSSL_free(sk->ssl);
    chHeapFree(sk);
}

/* IO Callbacks */
int wolfssl_send_cb(WOLFSSL* ssl, char *buf, int sz, void *ctx)
{
  sslconn *sk = (sslconn *)ctx;
  int err;
  (void)ssl;
  err = netconn_write(sk->conn, buf, sz, NETCONN_COPY);
  if (err == ERR_OK)
    return sz;
  else if (err == ERR_CLSD || err == ERR_ABRT || err == ERR_CONN)
    return WOLFSSL_CBIO_ERR_CONN_CLOSE;
  else if (err == ERR_RST)
    return WOLFSSL_CBIO_ERR_CONN_RST;
  else if (err == ERR_MEM)
    return WOLFSSL_CBIO_ERR_GENERAL;
  else if (err == ERR_TIMEOUT)
    return WOLFSSL_CBIO_ERR_TIMEOUT;
  else
    return WOLFSSL_CBIO_ERR_WANT_WRITE;
}

int wolfssl_recv_cb(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
  sslconn *sk = (sslconn *)ctx;
  u16_t n;

  (void)ssl;

  /* Data is taken from the pending netbuf of this connection, a new one is
     received only when it has been consumed.*/
  if (sk->rxbuf == NULL) {
    err_t err = netconn_recv(sk->conn, &sk->rxbuf);
    if (err != ERR_OK) {
      sk->rxbuf = NULL;
      if (err == ERR_CLSD || err == ERR_ABRT || err == ERR_CONN)
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
      else if (err == ERR_RST)
        return WOLFSSL_CBIO_ERR_CONN_RST;
      else if (err == ERR_TIMEOUT)
        return WOLFSSL_CBIO_ERR_TIMEOUT;
      else
        return WOLFSSL_CBIO_ERR_GENERAL;
    }
    sk->rxoff = 0U;
  }

  if (sz > 0xFFFF)
    sz = 0xFFFF;
  n = netbuf_copy_partial(sk->rxbuf, buf, (u16_t)sz, sk->rxoff);
  sk->rxoff += n;
  if (sk->rxoff >= netbuf_len(sk->rxbuf)) {
    netbuf_delete(sk->rxbuf);
    sk->rxbuf = NULL;
  }

  return (int)n;
}

#ifndef ST2S
#   define ST2S(n) (((n) + CH_CFG_ST_FREQUENCY - 1UL) / CH_CFG_ST_FREQUENCY)
#endif

#ifndef ST2MS
#define ST2MS(n) (((n) * 1000UL + CH_CFG_ST_FREQUENCY - 1UL) / CH_CFG_ST_FREQUENCY)
#endif

word32 LowResTimer(void)
{
    systime_t t = chVTGetSystemTimeX();
    return ST2S(t);
}

#if defined(WOLFSSL_TLS13) && (defined(HAVE_SESSION_TICKET) || \
    !defined(NO_PSK) || defined(WOLFSSL_DTLS13))
#ifdef WOLFSSL_32BIT_MILLI_TIME
word32 TimeNowInMilliseconds(void)
#else
sword64 TimeNowInMilliseconds(void)
#endif
{
    systime_t t = chVTGetSystemTimeX();
    return ST2MS(t);
}
#endif

void *chHeapRealloc (void *addr, uint32_t size)
{
    union heap_header *hp;
    uint32_t prev_size, new_size;

    void *ptr;

    if (addr == NULL) {
        return chHeapAlloc(NULL, size);
    }

    /* previous allocated segment is preceded by an heap_header */
    hp = addr - sizeof(union heap_header);
    prev_size = hp->used.size; /* size is always multiple of 8 */

    /* Check new size memory alignment */
    if (size % 8 == 0) {
        new_size = size;
    }
    else {
        new_size = ((int) (size / 8)) * 8 + 8;
    }

    if (prev_size >= new_size) {
        return addr;
    }

    ptr = chHeapAlloc(NULL, size);
    if (ptr == NULL) {
        return NULL;
    }

    memcpy(ptr, addr, prev_size);

    chHeapFree(addr);

    return ptr;
}

void *chibios_alloc(void *heap, int size)
{
    return chHeapAlloc(heap, size);
}

void chibios_free(void *ptr)
{
    if (ptr)
        chHeapFree(ptr);
}

