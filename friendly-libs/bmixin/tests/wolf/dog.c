/* la Bombolla GObject shell
 *
 * Copyright (c) 2024, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include "animal.h"

/* magic symbols to link with the mixin deps */
GType has_tail_get_type (void);
GType hairy_get_type (void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct {
  BMixinInstance i;
} Dog;

typedef struct {
  BMixinClass c;
} DogClass;

static void
animal_class_setup (gpointer class_ptr)
{
  AnimalClass *animal_klass =
      bm_class_get_mixin (class_ptr, animal_get_type ());

  animal_klass->maybe_domestic = TRUE;
}

BM_DEFINE_MIXIN (dog, Dog,
    BM_ADD_DEP (has_tail), BM_ADD_DEP (hairy), BM_ADD_DEP (animal),
    BM_ADD_CLASS_SETUP (animal));

static void
dog_init (GObject * object, Dog * self) {
  g_message ("dog_init");
}

static void
dog_class_init (GObjectClass * object_class, DogClass * klass) {
  AnimalClass *animal_klass;

  g_message ("dog_class_init");

  animal_klass = BM_CLASS_LOOKUP_MIXIN (klass, Animal);

  g_assert (animal_klass->maybe_domestic);
  animal_klass->is_domestic = TRUE;
}
