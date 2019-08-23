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

#ifndef EVAL_H
#define EVAL_H

#include "const.h"

const Bitboard flank_ranks[2] = {
    RANK_1BB | RANK_2BB | RANK_3BB | RANK_4BB,
    RANK_8BB | RANK_7BB | RANK_6BB | RANK_5BB
};

const Bitboard queenside_flank = FILE_ABB | FILE_BBB | FILE_CBB | FILE_DBB;
const Bitboard center_flank = FILE_CBB | FILE_DBB | FILE_EBB | FILE_FBB;
const Bitboard kingside_flank = FILE_EBB | FILE_FBB | FILE_GBB | FILE_HBB;

const Bitboard flank_files[8] = {
    queenside_flank,
    queenside_flank,
    queenside_flank,
    center_flank,
    center_flank,
    kingside_flank,
    kingside_flank,
    kingside_flank
};

int evaluate(Position *p);

#endif
