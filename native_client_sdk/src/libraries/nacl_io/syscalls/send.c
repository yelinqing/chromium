/* Copyright 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#include "nacl_io/kernel_intercept.h"
#include "nacl_io/kernel_wrap.h"

#ifdef PROVIDES_SOCKET_API

ssize_t send(int fd, const void* buf, size_t len, int flags) {
  return ki_send(fd, buf, len, flags);
}

#endif
