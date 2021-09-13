/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_PIN_H
#define MYMPD_PIN_H

#include <stdbool.h>

#include "../../dist/src/sds/sds.h"

void pin_set(sds workdir);
bool pin_validate(const char *pin, const char *pin_hash);
#endif
