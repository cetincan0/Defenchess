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

#include "tb.h"
#include "fathom/tbprobe.h"
#include "bitboard.h"
#include "move.h"

bool tb_initialized = false;
int SYZYGY_LARGEST = 0;

void init_syzygy(std::string syzygy_path) {
    tb_initialized = tb_init(syzygy_path.c_str());
    if (tb_initialized) {
        SYZYGY_LARGEST = int(TB_LARGEST);
    }

}

int result_to_wdl(unsigned result) {
    if (result == TB_LOSS) {
        return SYZYGY_LOSS;
    } else if (result == TB_BLESSED_LOSS || result == TB_DRAW || result == TB_CURSED_WIN) {
        return SYZYGY_DRAW;
    } else if (result == TB_WIN) {
        return SYZYGY_WIN;
    } else if (result == TB_RESULT_FAILED) {
        return SYZYGY_FAIL;
    }
    assert(false);
    return -1;
}

int probe_syzygy_wdl(Position *p) {
    if (count(p->board) > SYZYGY_LARGEST || p->info->last_irreversible != 0 || p->info->castling != 0) {
        return SYZYGY_FAIL;
    }

    unsigned result = tb_probe_wdl(
        uint64_t(p->bbs[white]),
        uint64_t(p->bbs[black]),
        uint64_t(p->bbs[king(white)] | p->bbs[king(black)]),
        uint64_t(p->bbs[queen(white)] | p->bbs[queen(black)]),
        uint64_t(p->bbs[rook(white)] | p->bbs[rook(black)]),
        uint64_t(p->bbs[bishop(white)] | p->bbs[bishop(black)]),
        uint64_t(p->bbs[knight(white)] | p->bbs[knight(black)]),
        uint64_t(p->bbs[pawn(white)] | p->bbs[pawn(black)]),
        unsigned(p->info->last_irreversible),
        unsigned(p->info->castling),
        unsigned(p->info->enpassant),
        !bool(p->color)
    );
    return result_to_wdl(result);
}

int probe_syzygy_dtz(Position *p, Move *move) {
    if (count(p->board) > SYZYGY_LARGEST) {
        return SYZYGY_FAIL;
    }

    unsigned result = tb_probe_root(
        uint64_t(p->bbs[white]),
        uint64_t(p->bbs[black]),
        uint64_t(p->bbs[king(white)] | p->bbs[king(black)]),
        uint64_t(p->bbs[queen(white)] | p->bbs[queen(black)]),
        uint64_t(p->bbs[rook(white)] | p->bbs[rook(black)]),
        uint64_t(p->bbs[bishop(white)] | p->bbs[bishop(black)]),
        uint64_t(p->bbs[knight(white)] | p->bbs[knight(black)]),
        uint64_t(p->bbs[pawn(white)] | p->bbs[pawn(black)]),
        unsigned(p->info->last_irreversible),
        unsigned(p->info->castling),
        unsigned(p->info->enpassant),
        !bool(p->color),
        nullptr
    );

    if (result == TB_RESULT_STALEMATE || result == TB_RESULT_CHECKMATE || result == TB_RESULT_FAILED) {
        return SYZYGY_FAIL;
    }

    unsigned wdl = TB_GET_WDL(result);
    Square from = Square(TB_GET_FROM(result));
    Square to = Square(TB_GET_TO(result));
    Square enpassant = Square(TB_GET_EP(result));
    unsigned promo = TB_GET_PROMOTES(result);

    if (promo != TB_PROMOTES_NONE) {
        *move = _movecast(from, to, PROMOTION);
        if (promo == TB_PROMOTES_QUEEN) {
            *move = _promoteq(*move);
        } else if (promo == TB_PROMOTES_ROOK) {
            *move = _promoter(*move);
        } else if (promo == TB_PROMOTES_BISHOP) {
            *move = _promoteb(*move);
        } else if (promo == TB_PROMOTES_KNIGHT) {
            *move = _promoten(*move);
        }
    } else if (enpassant != 0) {
        *move = _movecast(from, p->info->enpassant, ENPASSANT);
    } else {
        *move = _movecast(from, to, NORMAL);
    }

    return result_to_wdl(wdl);
}
