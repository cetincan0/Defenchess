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

#include "eval.h"
#include "bitboard.h"
#include "move.h"
#include "target.h"
#include "tt.h"

void evaluate_pawn_structure(Evaluation *eval, Position *p, Color color) {
    Score pawn_structure = {0, 0};

    Bitboard my_pawns = p->bbs[pawn(color)];
    Bitboard opponent_pawns = p->bbs[pawn(~color)];
    Bitboard pawns = my_pawns;
    Bitboard supported, adjacent, doubled;
    bool opposed, passing, isolated, backward;

    eval->pawntte->pawn_passers[color] = 0;
    eval->pawntte->semi_open_files[color] = 0xFF;

    while (pawns) {
        Square sq = pop(&pawns);

        supported = my_pawns & PAWN_CAPTURE_MASK[sq][~color];
        adjacent = my_pawns & ADJACENT_MASK[sq];
        isolated = (MASK_ISOLATED[file_of(sq)] & my_pawns) == 0;
        opposed = (FRONT_MASK[sq][color] & opponent_pawns) != 0;
        passing = (PASSED_PAWN_MASK[sq][color] & opponent_pawns) == 0;
        doubled = my_pawns & FRONT_MASK[sq][color];
        backward = !(my_pawns & PASSED_PAWN_MASK[sq][~color]) &&
                   (bfi[pawn_forward(sq, color)] & eval->targets[pawn(~color)]);

        if (supported | adjacent) {
            pawn_structure += connected_bonus[opposed][bool(adjacent)][relative_rank(sq, color)];
        } else if (isolated) {
            pawn_structure -= isolated_pawn_penalty[opposed];
        } else if (backward) {
            pawn_structure -= backward_pawn_penalty[opposed];
        }

        if (doubled) {
            pawn_structure -= double_pawn_penalty;
        }

        if (passing) {
            eval->pawntte->pawn_passers[color] |= bfi[sq];
        }

        eval->pawntte->semi_open_files[color] &= ~(1 << file_of(sq));
    }

    eval->pawntte->score[color] = pawn_structure;
}

int evaluate_pawn_shelter(Position *p, Color color, Square sq) {
    int pawn_shelter_value = 0;
    int middle = std::max(FILE_B, std::min(FILE_G, file_of(sq)));

    Bitboard my_rank_forward = FRONT_RANK_MASKS[sq][color] | RANK_MASK[rank_of(sq)];
    Bitboard my_pawns = p->bbs[pawn(color)] & my_rank_forward;
    Bitboard opponent_pawns = p->bbs[pawn(~color)] & my_rank_forward;

    for (int file = middle - 1; file <= middle + 1; ++file) {
        Bitboard pawns = my_pawns & FILE_MASK[file];
        int my_rank = pawns ? relative_rank(color == white ? lsb(pawns) : msb(pawns), color) : RANK_1;

        pawns = opponent_pawns & FILE_MASK[file];
        int opponent_rank = pawns ? relative_rank(color == white ? lsb(pawns) : msb(pawns), color) : RANK_1;

        int f = std::min(file, FILE_H - file);
        pawn_shelter_value += pawn_shelter[f][my_rank];
        pawn_shelter_value -= pawn_storm[my_rank != RANK_1 && my_rank == opponent_rank - 1][f][opponent_rank];
    }

    return pawn_shelter_value;
}

void pawn_shelter_with_castling(Evaluation *eval, Position *p, Color color) {
    eval->pawntte->pawn_shelter_value[color] = evaluate_pawn_shelter(p, color, p->king_index[color]);
    if (p->info->castling & can_king_castle_mask[color]) {
        eval->pawntte->pawn_shelter_value[color] = std::max(eval->pawntte->pawn_shelter_value[color], evaluate_pawn_shelter(p, color, relative_square(G1, color)));
    }
    if (p->info->castling & can_queen_castle_mask[color]) {
        eval->pawntte->pawn_shelter_value[color] = std::max(eval->pawntte->pawn_shelter_value[color], evaluate_pawn_shelter(p, color, relative_square(C1, color)));
    }
}

void evaluate_pawns(Evaluation *eval, Position *p) {
    if (eval->pawntte->pawn_hash != p->info->pawn_hash) {
        eval->pawntte->pawn_hash = p->info->pawn_hash;

        eval->pawntte->king_index[white] = p->king_index[white];
        eval->pawntte->king_index[black] = p->king_index[black];

        eval->pawntte->castling = p->info->castling;

        evaluate_pawn_structure(eval, p, white);
        evaluate_pawn_structure(eval, p, black);
        pawn_shelter_with_castling(eval, p, white);
        pawn_shelter_with_castling(eval, p, black);
    } else {
        if (p->king_index[white] != eval->pawntte->king_index[white] ||
                (p->info->castling & can_castle_mask[white]) != (eval->pawntte->castling & can_castle_mask[white])) {
            pawn_shelter_with_castling(eval, p, white);
        }
        if (p->king_index[black] != eval->pawntte->king_index[black] ||
                (p->info->castling & can_castle_mask[black]) != (eval->pawntte->castling & can_castle_mask[black])) {
            pawn_shelter_with_castling(eval, p, black);
        }

        eval->pawntte->king_index[white] = p->king_index[white];
        eval->pawntte->king_index[black] = p->king_index[black];

        eval->pawntte->castling = p->info->castling;
    }
}

Score evaluate_pieces(Evaluation *eval, Position *p, Color color) {
    Score piece_score = {0, 0};

    Bitboard capturable, king_threats;

    Bitboard my_bishops = p->bbs[bishop(color)];
    Bitboard my_knights = p->bbs[knight(color)];
    Bitboard my_rooks = p->bbs[rook(color)];
    Bitboard my_queens = p->bbs[queen(color)];
    Bitboard pinned = p->info->pinned[color];

    Bitboard opponent_bishops = p->bbs[bishop(~color)];
    Bitboard opponent_knights = p->bbs[knight(~color)];
    Bitboard opponent_rooks = p->bbs[rook(~color)];
    Bitboard opponent_queens = p->bbs[queen(~color)];

    eval->targets[bishop(color)] = 0;
    while (my_bishops) {
        Square sq = pop(&my_bishops);
        Bitboard bishop_targets = generate_bishop_targets(p->board ^ p->bbs[queen(color)], sq);
        if (pinned & bfi[sq]) {
            bishop_targets &= FROMTO_MASK[p->king_index[color]][sq];
        }

        capturable = opponent_bishops | opponent_rooks | opponent_queens;
        int mobility = count(bishop_targets & (eval->mobility_area[color] | capturable));
        piece_score += mobility_bonus[BISHOP][mobility];

        // Bishop with same colored pawns
        piece_score -= bishop_pawn_penalty * count(COLOR_MASKS[TILE_COLOR[sq]] & p->bbs[pawn(color)]);

        // King threats
        king_threats = bishop_targets & eval->king_zone[~color];
        if (king_threats) {
            ++eval->num_king_attackers[~color];
            eval->king_zone_score[~color] += ATTACK_VALUES[BISHOP];
            eval->num_king_zone_attacks[~color] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & bishop_targets;
        eval->targets[color] |= eval->targets[bishop(color)] |= bishop_targets;
    }

    eval->targets[knight(color)] = 0;
    while (my_knights) {
        Square sq = pop(&my_knights);
        Bitboard knight_targets = generate_knight_targets(sq);

        if (pinned & bfi[sq]) {
            // Pinned knights cannot move
            knight_targets = 0;
        }

        capturable = opponent_knights | opponent_bishops | opponent_rooks | opponent_queens;
        int mobility = count(knight_targets & (eval->mobility_area[color] | capturable));
        piece_score += mobility_bonus[KNIGHT][mobility];

        // King threats
        king_threats = knight_targets & eval->king_zone[~color];
        if (king_threats) {
            ++eval->num_king_attackers[~color];
            eval->king_zone_score[~color] += ATTACK_VALUES[KNIGHT];
            eval->num_king_zone_attacks[~color] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & knight_targets;
        eval->targets[color] |= eval->targets[knight(color)] |= knight_targets;
    }

    eval->targets[rook(color)] = 0;
    while (my_rooks) {
        Square sq = pop(&my_rooks);
        Bitboard rook_targets = generate_rook_targets(p->board ^ (p->bbs[queen(color)] | p->bbs[rook(color)]), sq);

        if (pinned & bfi[sq]) {
            rook_targets &= FROMTO_MASK[p->king_index[color]][sq];
        }

        capturable = opponent_rooks | opponent_queens;
        int mobility = count(rook_targets & (eval->mobility_area[color] | capturable));
        piece_score += mobility_bonus[ROOK][mobility];

        // Bonus for being on a semiopen or open file
        if (eval->pawntte->semi_open_files[color] & (1 << file_of(sq))) {
            piece_score += rook_file_bonus[bool(eval->pawntte->semi_open_files[~color] & (1 << file_of(sq)))];
        }

        // King threats
        king_threats = rook_targets & eval->king_zone[~color];
        if (king_threats) {
            ++eval->num_king_attackers[~color];
            eval->king_zone_score[~color] += ATTACK_VALUES[ROOK];
            eval->num_king_zone_attacks[~color] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & rook_targets;
        eval->targets[color] |= eval->targets[rook(color)] |= rook_targets;
    }

    eval->targets[queen(color)] = 0;
    while (my_queens) {
        Square sq = pop(&my_queens);
        Bitboard queen_targets = generate_queen_targets(p->board, sq);

        if (pinned & bfi[sq]) {
            queen_targets &= FROMTO_MASK[p->king_index[color]][sq];
        }

        capturable = opponent_queens;
        int mobility = count(queen_targets & (eval->mobility_area[color] | capturable));
        piece_score += mobility_bonus[QUEEN][mobility];

        // King threats
        king_threats = queen_targets & eval->king_zone[~color];
        if (king_threats) {
            ++eval->num_king_attackers[~color];
            eval->king_zone_score[~color] += ATTACK_VALUES[QUEEN];
            eval->num_king_zone_attacks[~color] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & queen_targets;
        eval->targets[color] |= eval->targets[queen(color)] |= queen_targets;
        ++eval->num_queens[color];
    }
    
    return piece_score;
}

Score evaluate_king(Evaluation *eval, Position *p, Color color) {
    Score king_score = {0, 0};
    Square king_sq = p->king_index[color];

    int pawn_shelter_value = eval->pawntte->pawn_shelter_value[color];
    king_score.midgame += pawn_shelter_value;

    if (p->bbs[pawn(color)]) {
        int pawn_distance = 0;
        while (!(DISTANCE_RING[king_sq][pawn_distance++] & p->bbs[pawn(color)])) {}
        king_score.endgame -= pawn_distance_penalty * pawn_distance;
    }

    Bitboard flank_attacks = eval->targets[~color] & flank_ranks[color] & flank_files[file_of(king_sq)];
    Bitboard flank_attacks2 = flank_attacks & eval->double_targets[~color];
    king_score.midgame -= king_flank_penalty * (count(flank_attacks) + count(flank_attacks2));

    if (eval->num_king_attackers[color] > (1 - eval->num_queens[~color])) {
        Bitboard weak = eval->targets[king(color)] & eval->targets[~color] & ~eval->double_targets[color];
        Bitboard weak_zone = eval->targets[~color] & ~eval->targets[color] & eval->king_zone[color] & ~p->bbs[~color];

        int king_danger = king_danger_init
                        - pawn_shelter_value * king_danger_shelter_bonus / 10
                        - !eval->num_queens[~color] * king_danger_queen_penalty
                        + eval->num_king_attackers[color] * eval->king_zone_score[color]
                        + eval->num_king_zone_attacks[color] * king_zone_attack_penalty
                        + bool(p->info->pinned[color]) * king_danger_pinned_penalty
                        + count(weak) * king_danger_weak_penalty
                        + count(weak_zone) * king_danger_weak_zone_penalty;

        // Safe squares that can be checked by the opponent
        Bitboard safe = ~p->bbs[~color] & (~eval->targets[color] | (weak & eval->double_targets[~color]));

        Bitboard rook_attacks = generate_rook_targets(p->board, king_sq);
        Bitboard bishop_attacks = generate_bishop_targets(p->board, king_sq);
        Bitboard knight_targets = generate_knight_targets(king_sq);

        if ((rook_attacks | bishop_attacks) & eval->targets[queen(~color)] & safe) {
            king_danger += queen_check_penalty;
        }

        safe |= eval->double_targets[~color] & ~(eval->double_targets[color] | p->bbs[~color]) & eval->targets[queen(color)];

        if (rook_attacks & eval->targets[rook(~color)] & safe) {
            king_danger += rook_check_penalty;
        }
        if (bishop_attacks & eval->targets[bishop(~color)] & safe) {
            king_danger += bishop_check_penalty;
        }
        if (knight_targets & eval->targets[knight(~color)] & safe) {
            king_danger += knight_check_penalty;
        }

        if (king_danger > 0) {
            king_score -= Score{king_danger * king_danger / 4096, king_danger / 20};
        }
    }

    return king_score;
}

Score evaluate_threat(Evaluation *eval, Position *p, Color color) {
    Score threat_score = {0, 0};

    Bitboard opponent_non_pawns = p->bbs[~color] ^ p->bbs[pawn(~color)];
    Bitboard very_supported = eval->targets[pawn(~color)] | (eval->double_targets[~color] & ~eval->double_targets[color]);
    Bitboard not_supported = p->bbs[~color] & ~very_supported & eval->targets[color];

    Bitboard attacked;
    if (opponent_non_pawns | not_supported) {
        attacked = (opponent_non_pawns | not_supported) & (eval->targets[knight(color)] | eval->targets[bishop(color)]);
        while (attacked) {
            Square sq = pop(&attacked);
            threat_score += minor_threat_bonus[piece_type(p->pieces[sq])];
        }

        attacked = (p->bbs[queen(~color)] | not_supported) & eval->targets[rook(color)];
        while (attacked) {
            Square sq = pop(&attacked);
            threat_score += rook_threat_bonus[piece_type(p->pieces[sq])];
        }

        attacked = opponent_non_pawns & eval->targets[pawn(color)];
        while (attacked) {
            Square sq = pop(&attacked);
            threat_score += pawn_threat_bonus[piece_type(p->pieces[sq])];
        }

        attacked = not_supported & eval->targets[king(color)];
        if (attacked) {
            threat_score += king_threat_bonus[more_than_one(attacked)];
        }
    }

    Bitboard pawn_moves  = (color == white ? p->bbs[pawn(color)] << 8 : p->bbs[pawn(color)] >> 8) & ~p->board;
    pawn_moves |= (color == white ? ((pawn_moves & RANK_3BB) << 8) : ((pawn_moves & RANK_6BB) >> 8)) & ~p->board;
    pawn_moves &= ~eval->targets[pawn(~color)] & (eval->targets[color] | ~eval->targets[~color]);
    threat_score += pawn_push_threat_bonus * count(generate_pawn_threats(pawn_moves, color) & p->bbs[~color]);

    return threat_score;
}

Score evaluate_passers(Evaluation *eval, Position *p, Color color) {
    Score passer_score = {0, 0};
    Bitboard passers = eval->pawntte->pawn_passers[color];
    bool blocked, unsafe;

    while (passers) {
        Square sq = pop(&passers);
        int r = relative_rank(sq, color);
        passer_score += passed_pawn_bonus[r];

        passer_score.endgame -= passer_my_distance[r] * distance(p->king_index[color], sq);
        passer_score.endgame += passer_enemy_distance[r] * distance(p->king_index[~color], sq);

        blocked = bool(bfi[pawn_forward(sq, color)] & p->board);
        unsafe = bool(bfi[pawn_forward(sq, color)] & eval->targets[~color]);

        passer_score += passer_blocked[blocked][r];
        passer_score += passer_unsafe[unsafe][r];
    }

    return passer_score;
}

void pre_eval(Evaluation *eval, Position *p) {
    eval->pawntte = get_pawntte(p);

    eval->targets[white_pawn] = generate_pawn_threats(p->bbs[white_pawn], white);
    eval->targets[black_pawn] = generate_pawn_threats(p->bbs[black_pawn], black);
    eval->targets[white_king] = generate_king_targets(p->king_index[white]);
    eval->targets[black_king] = generate_king_targets(p->king_index[black]);
    eval->targets[white] = 0;
    eval->targets[black] = 0;

    eval->double_targets[white] = ((p->bbs[white_pawn] & ~FILE_ABB) << 7) & ((p->bbs[white_pawn] & ~FILE_HBB) << 9);
    eval->double_targets[black] = ((p->bbs[black_pawn] & ~FILE_HBB) >> 7) & ((p->bbs[black_pawn] & ~FILE_ABB) >> 9);

    eval->double_targets[white] |= eval->targets[white_king] & eval->targets[white_pawn];
    eval->double_targets[black] |= eval->targets[black_king] & eval->targets[black_pawn];

    eval->targets[white] = eval->targets[white_king] | eval->targets[white_pawn];
    eval->targets[black] = eval->targets[black_king] | eval->targets[black_pawn];

    if (relative_rank(p->king_index[white], white) == RANK_1) {
        eval->king_zone[white] = KING_EXTENDED_MASKS[white][p->king_index[white]];
    } else {
        eval->king_zone[white] = eval->targets[white_king];
    }

    if (relative_rank(p->king_index[black], black) == RANK_1) {
        eval->king_zone[black] = KING_EXTENDED_MASKS[black][p->king_index[black]];
    } else {
        eval->king_zone[black] = eval->targets[black_king];
    }

    eval->num_king_attackers[white] = count(eval->targets[black_pawn] & eval->king_zone[white]);
    eval->num_king_attackers[black] = count(eval->targets[white_pawn] & eval->king_zone[black]);

    Bitboard low_ranks= RANK_2BB | RANK_3BB;
    Bitboard blocked_pawns= p->bbs[pawn(white)] & ((p->board >> 8) | low_ranks);
    eval->mobility_area[white] = ~(blocked_pawns | p->bbs[white_king] | eval->targets[black_pawn]);

    low_ranks = RANK_6BB | RANK_7BB;
    blocked_pawns = p->bbs[pawn(black)] & ((p->board << 8) | low_ranks);
    eval->mobility_area[black] = ~(blocked_pawns | p->bbs[black_king] | eval->targets[white_pawn]);

    eval->num_king_zone_attacks[white] = eval->num_king_zone_attacks[black] = 0;
    eval->king_zone_score[white] = eval->king_zone_score[black] = 0;
    eval->num_queens[white] = eval->num_queens[black] = 0;
}

int scaling_factor(Position *p) {
    if (only_one(p->bbs[white_bishop]) &&
            only_one(p->bbs[black_bishop]) &&
            only_one(COLOR_MASKS[white] & (p->bbs[white_bishop] | p->bbs[black_bishop]))) {
        if (p->info->non_pawn_material[white] == BISHOP_MID && p->info->non_pawn_material[black] == BISHOP_MID) {
            return SCALE_PURE_OCB;
        } else {
            return SCALE_OCB_WITH_PIECES;
        }
    }

    Color winner = p->score.endgame > 0 ? white : black;
    if (!p->bbs[pawn(winner)] && p->info->non_pawn_material[winner] <= p->info->non_pawn_material[~winner] + piece_values[white_bishop]) {
        return SCALE_NO_PAWNS;
    }

    return SCALE_NORMAL;
}

int evaluate(Position *p) {
    assert(!is_checked(p));
    Evaluation eval;
    pre_eval(&eval, p);

    eval.score = p->score;
    evaluate_pawns(&eval, p);
    eval.score += eval.pawntte->score[white] - eval.pawntte->score[black];

    int early = (eval.score.midgame + eval.score.endgame) / 2;
    if (std::abs(early) > 1000) {
        return p->color == white ? early : -early;
    }

    Material *eval_material = get_material(p);
    eval.score += eval_material->score;
    eval.score += evaluate_pieces(&eval, p, white) - evaluate_pieces(&eval, p, black);
    eval.score += evaluate_king(&eval, p, white) - evaluate_king(&eval, p, black) +
                  evaluate_threat(&eval, p, white) - evaluate_threat(&eval, p, black) +
                  evaluate_passers(&eval, p, white) - evaluate_passers(&eval, p, black);

    int scale = scaling_factor(p);
    int ret = (eval.score.midgame * eval_material->phase + eval.score.endgame * (256 - eval_material->phase) * scale / SCALE_NORMAL) / 256;
    return (p->color == white ? ret : -ret) + tempo;
}
