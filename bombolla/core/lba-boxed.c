/* la Bombolla GObject shell
 *
 * Copyright (c) 2025, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lba-boxed.h"

LBA_DEFINE_BOXED (LbaBoxed, lba_boxed);

void
lba_boxed_init (LbaBoxed *bxd, GType self_type, GDestroyNotify free) {
  bxd->self_type = self_type;
  bxd->refcount = 1;
  bxd->free = free;
}

static void
lba_boxed_clear (LbaBoxed *bxd) {
  GDestroyNotify f;

  g_assert (bxd->refcount == 0);
  g_assert (bxd->free != NULL);
  g_assert (bxd->self_type != 0);

  f = bxd->free;

  /* Spoil the data to crash on double-free */
  bxd->self_type = 0;
  bxd->refcount = -1;
  bxd->free = NULL;

  f (bxd);
}

void
lba_boxed_unref (gpointer b) {
  LbaBoxed *bxd = (LbaBoxed *) b;

  if (bxd == NULL)
    return;

  if (g_atomic_int_dec_and_test (&bxd->refcount))
    lba_boxed_clear (bxd);
}

gpointer
lba_boxed_ref (gpointer b) {
  LbaBoxed *bxd = (LbaBoxed *) b;

  g_return_val_if_fail (bxd != NULL, NULL);
  g_assert (bxd->free != NULL);
  g_assert (bxd->refcount >= 0);
  g_assert (bxd->self_type != 0);

  g_atomic_int_inc (&bxd->refcount);
  return bxd;
}
