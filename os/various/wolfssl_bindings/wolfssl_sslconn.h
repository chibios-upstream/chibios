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

/**
 * @file    wolfssl_sslconn.h
 * @brief   wolfSSL TLS connections over lwIP netconns.
 */

#ifndef WOLFSSL_SSLCONN_H
#define WOLFSSL_SSLCONN_H

#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/ssl.h"
#include "lwip/api.h"

struct sslconn {
    WOLFSSL_CTX *ctx;
    WOLFSSL *ssl;
    struct netconn *conn;
    struct netbuf *rxbuf;   /* Received data not yet consumed.   */
    u16_t rxoff;            /* Consumed bytes in @p rxbuf.       */
};

typedef struct sslconn sslconn;

#ifdef __cplusplus
extern "C" {
#endif
  sslconn *sslconn_accept(struct sslconn *sk);
  sslconn *sslconn_new(enum netconn_type t, WOLFSSL_METHOD *method);
  void sslconn_close(sslconn *sk);
  int wolfssl_send_cb(WOLFSSL* ssl, char *buf, int sz, void *ctx);
  int wolfssl_recv_cb(WOLFSSL *ssl, char *buf, int sz, void *ctx);
#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_SSLCONN_H */
