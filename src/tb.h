/*
    Defenchess, a chess engine
    Copyright 2017-2019 Can Cetin, Dogac Eldenk

    Defenchess is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Defenchess is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Defenchess.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TB_H
#define TB_H

#include <string>

#include "const.h"

extern bool tb_initialized;
extern int SYZYGY_LARGEST;

enum SyzygyResult {
    SYZYGY_LOSS,
    SYZYGY_DRAW,
    SYZYGY_WIN,
    SYZYGY_FAIL
};

void init_syzygy(std::string syzygy_path);
int probe_syzygy_wdl(Position *p);
int probe_syzygy_dtz(Position *p, Move *move);

#endif
