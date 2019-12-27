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

#include <iostream>
#include <sys/time.h>
#include <thread>
#include <set>

#include "bitboard.h"
#include "eval.h"
#include "move_utils.h"
#include "move.h"
#include "movegen.h"
#include "move_utils.h"
#include "position.h"
#include "search.h"
#include "see.h"
#include "tb.h"
#include "tt.h"

struct timeval curr_time, start_ts;

int timer_count = 1024,
    myremain = 10000,
    total_remaining = 10000,
    think_depth_limit = MAX_PLY;

bool is_movetime = false;

volatile bool is_timeout = false,
              quit_application = false,
              is_searching = false,
              is_pondering = false;

Move latest_pv, latest_ponder;
Move main_pv[MAX_PLY + 1];
std::set<Move> root_moves;

void print_pv(Position *p, Metadata *md) {
    int i = 0;
    while (md->pv[i] != no_move) {
        std::cout << move_to_str(p, md->pv[i++]) << " ";
    }
}

void set_pv(Move move, Metadata *md) {
    md->pv[0] = move;

    int i;
    for (i = 1; (md+1)->pv[i - 1] != no_move; ++i) {
        md->pv[i] = (md+1)->pv[i - 1];
    }
    md->pv[i] = no_move;
}

void set_main_pv(Metadata *md) {
    int i = 0;
    while (md->pv[i] != no_move) {
        main_pv[i] = md->pv[i];
        ++i;
    }
    main_pv[i] = no_move;
}

bool is_draw(Position *p) {
    int last_irreversible = p->info->last_irreversible;
    if (last_irreversible > 3){
        if (last_irreversible > 99) {
            return true;
        }

        SearchThread *my_thread = p->my_thread;
        int repetition_count = 0;
        int min_index = std::max(my_thread->search_ply - last_irreversible, 0);
        for (int i = my_thread->search_ply - 4; i >= min_index; i -= 2) {
            // The i >= 0 condition isn't necessary but would prevent undefined
            // behavior given an invalid fen.
            Info *info = &my_thread->infos[i];
            if (p->info->hash == info->hash) {
                if (i >= my_thread->root_ply) {
                    return true;
                }

                ++repetition_count;
                if (repetition_count == 2) {
                    return true;
                }
            }
        }
    }

    Material *eval_material = get_material(p);
    if (eval_material->endgame_type == DRAW_ENDGAME) {
        return true;
    }
    return false;
}

int draw_score(SearchThread *my_thread) {
    // Idea from Stockfish, prevents making losing moves during 3fold repetitions.
    return 1 - (my_thread->nodes & 2);
}

void update_history(int *history, int bonus) {
    *history += bonus - (*history) * std::abs(bonus) / 16384;
    assert(*history <= 16384 && *history >= -16384);
}

void save_killer(Position *p, Metadata *md, Move move, int depth, Move *quiets, int quiets_count) {
    SearchThread *my_thread = p->my_thread;
    if (move != md->killers[0]) {
        md->killers[1] = md->killers[0];
        md->killers[0] = move;
    }

    int bonus = std::min(32 * depth * depth, 16384);
    Color color = p->color;
    Square from = move_from(move);
    Square to = move_to(move);
    update_history(&my_thread->history[color][from][to], bonus);

    bool cmh_valid = is_move_valid((md-1)->current_move);
    bool fmh_valid = is_move_valid((md-2)->current_move);

    Piece piece = p->pieces[from];
    if (cmh_valid) {
        Square prev_to = move_to((md-1)->current_move);
        my_thread->counter_moves[p->pieces[prev_to]][prev_to] = move;

        update_history(&(md-1)->counter_move_history[piece][to], bonus);
    }

    if (fmh_valid) {
        update_history(&(md-2)->counter_move_history[piece][to], bonus);
    }

    for (int i = 0; i < quiets_count; ++i) {
        Move q = quiets[i];

        Piece quiet_piece = p->pieces[move_from(q)];
        Square quiet_from = move_from(q);
        Square quiet_to = move_to(q);

        update_history(&my_thread->history[color][quiet_from][quiet_to], -bonus);

        if (cmh_valid) {
            update_history(&(md-1)->counter_move_history[quiet_piece][quiet_to], -bonus);
        }
        if (fmh_valid) {
            update_history(&(md-2)->counter_move_history[quiet_piece][quiet_to], -bonus);
        }
    }
}

void check_time(Position *p) {
    if (!is_main_thread(p)) {
        return;
    }

    --timer_count;

    if (timer_count == 0) {
        gettimeofday(&curr_time, nullptr);
        if (time_passed() >= total_remaining && !is_pondering) {
            is_timeout = true;
        }

        timer_count = 1024;
    }
}

int alpha_beta_quiescence(Position *p, Metadata *md, int alpha, int beta, int depth, bool in_check) {
    assert(alpha >= -MATE && alpha < beta && beta <= MATE);
    assert(depth <= 0);
    assert(in_check == is_checked(p));

    int ply = md->ply;
    bool is_pv = beta - alpha > 1;
    if (is_pv) {
        md->pv[0] = no_move;
    }

    if (ply >= MAX_PLY) {
        return !in_check ? evaluate(p) : 0;
    }

    SearchThread *my_thread = p->my_thread;
    if (is_draw(p)) {
        return draw_score(my_thread);
    }

    Info *info = p->info;
    md->current_move = no_move;
    (md+1)->ply = ply + 1;
    int tte_depth = in_check || depth >= 0 ? 0 : -1;

    Move tte_move = no_move;
    int tte_score = UNDEFINED;
    bool tt_hit;
    TTEntry *tte = get_tte(info->hash, tt_hit);
    uint8_t tt_flag = tte_flag(tte);
    if (tt_hit) {
        tte_move = tte->move;
        if (tte->depth >= tte_depth) {
            tte_score = tt_to_score(tte->score, ply);
            if (!is_pv &&
                (tt_flag == FLAG_EXACT ||
                (tt_flag == FLAG_BETA && tte_score >= beta) ||
                (tt_flag == FLAG_ALPHA && tte_score <= alpha))) {
                    return tte_score;
            }
        }
    }

    int best_score;
    if (!in_check) {
        bool is_null = (md-1)->current_move == null_move;
        if (tt_hit && tte->static_eval != UNDEFINED) {
            md->static_eval = best_score = tte->static_eval;
        } else if (is_null) {
            assert((md-1)->static_eval != UNDEFINED);
            md->static_eval = best_score = tempo * 2 - (md-1)->static_eval;
        } else {
            md->static_eval = best_score = evaluate(p);
        }
        if (best_score >= beta) {
            return best_score;
        }
        if (is_pv && best_score > alpha) {
            alpha = best_score;
        }
    } else {
        best_score = -INFINITE;
        md->static_eval = UNDEFINED;
    }

    MoveGen movegen = new_movegen(p, md, tte_move, QUIESCENCE_SEARCH, 0, in_check);

    Move best_move = no_move;
    int num_moves = 0;
    Move move;
    while ((move = next_move(&movegen, md, depth)) != no_move) {
        assert(is_move_valid(move));
        assert(is_pseudolegal(p, move));

        ++num_moves;

        if (!in_check && !see_capture(p, move, 0)) {
            continue;
        }

        if (!is_legal(p, move)) {
            --num_moves;
            continue;
        }

        make_move(p, move);
        ++my_thread->nodes;
        md->current_move = move;
        Square to = move_to(move);
        Piece piece = p->pieces[to];
        md->counter_move_history = p->my_thread->counter_move_history[piece][to];
        int score = -alpha_beta_quiescence(p, md+1, -beta, -alpha, depth - 1, is_checked(p));
        undo_move(p, move);

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                if (is_pv && is_main_thread(p)) {
                    set_pv(move, md);
                }
                best_move = move;
                if (is_pv && score < beta) {
                    alpha = score;
                } else {
                    set_tte(info->hash, tte, move, tte_depth, score_to_tt(score, ply), md->static_eval, FLAG_BETA);
                    return score;
                }
            }
        }
    }

    if (num_moves == 0 && in_check) {
        return -MATE + ply;
    }

    uint8_t flag = is_pv && best_move ? FLAG_EXACT : FLAG_ALPHA;
    set_tte(info->hash, tte, best_move, tte_depth, score_to_tt(best_score, ply), md->static_eval, flag);
    assert(best_score >= -MATE && best_score <= MATE);
    return best_score;
}

int alpha_beta(Position *p, Metadata *md, int alpha, int beta, int depth, bool in_check) {
    assert(-MATE <= alpha && alpha < beta && beta <= MATE);
    assert(in_check == is_checked(p));
    if (depth < 1) {
        return alpha_beta_quiescence(p, md, alpha, beta, 0, in_check);
    }

    int ply = md->ply;
    bool is_pv = beta - alpha > 1;
    if (is_pv) {
        md->pv[0] = no_move;
    }

    bool root_node = ply == 0;
    SearchThread *my_thread = p->my_thread;

    check_time(p);
    if (!root_node) {
        if (is_timeout) {
            return TIMEOUT;
        }

        if (ply >= MAX_PLY) {
            return !in_check ? evaluate(p) : 0;
        }

        if (is_draw(p)) {
            return draw_score(my_thread);
        }

        // Mate distance pruning
        alpha = std::max(-MATE + ply, alpha);
        beta = std::min(MATE - ply - 1, beta);
        if (alpha >= beta) {
            return alpha;
        }
    }

    if (is_pv && ply > my_thread->selply) {
        my_thread->selply = ply;
    }

    Info *info = p->info;
    md->current_move = (md+1)->excluded_move = no_move;
    Move excluded_move = md->excluded_move;
    uint64_t pos_hash = info->hash ^ uint64_t(excluded_move << 16);

    (md+1)->killers[0] = (md+1)->killers[1] = no_move;
    (md+1)->ply = ply + 1;

    Move tte_move = no_move;
    int tte_score = UNDEFINED;
    bool tt_hit;
    TTEntry *tte = get_tte(pos_hash, tt_hit);
    uint8_t tt_flag = tte_flag(tte);
    if (tt_hit) {
        tte_move = tte->move;
        if (tte->depth >= depth) {
            tte_score = tt_to_score(tte->score, ply);
            if (!is_pv &&
                (tt_flag == FLAG_EXACT ||
                (tt_flag == FLAG_BETA && tte_score >= beta) ||
                (tt_flag == FLAG_ALPHA && tte_score <= alpha))) {
                    return tte_score;
            }
        }
    }

    // Probe tablebase
    if (!root_node && tb_initialized) {
        int wdl = probe_syzygy_wdl(p);
        if (wdl != SYZYGY_FAIL) {
            ++my_thread->tb_hits;
            int tb_score = wdl == SYZYGY_LOSS ? MATED_IN_MAX_PLY + ply + 1
                         : wdl == SYZYGY_WIN  ? MATE_IN_MAX_PLY  - ply - 1 : 0;

            set_tte(pos_hash, tte, no_move, std::min(depth + SYZYGY_LARGEST, MAX_PLY - 1), score_to_tt(tb_score, ply), UNDEFINED, FLAG_EXACT);
            return tb_score;
        }
    }

    bool is_null = (md-1)->current_move == null_move;
    if (!in_check) {
        if (tt_hit && tte->static_eval != UNDEFINED) {
            md->static_eval = tte->static_eval;
        } else if (is_null) {
            assert((md-1)->static_eval != UNDEFINED);
            md->static_eval = tempo * 2 - (md-1)->static_eval;
        } else {
            md->static_eval = evaluate(p);
        }
    } else {
        md->static_eval = UNDEFINED;
    }

    // Razoring
    if (!in_check && !is_pv && depth < 2 && md->static_eval <= alpha - razoring_margin) {
        return alpha_beta_quiescence(p, md, alpha, beta, 0, false);
    }

    // Futility
    if (!in_check && !is_pv && depth < 7 && md->static_eval - 80 * depth >= beta && info->non_pawn_material[p->color]) {
        return md->static_eval;
    }

    // Null move pruning
    if (!in_check && !is_pv && my_thread->nmp_enabled && !is_null &&
        excluded_move == no_move && depth > 2 && md->static_eval >= beta && info->non_pawn_material[p->color])
    {
        int R = 3 + depth / 4 + std::min((md->static_eval - beta) / PAWN_MID, 3);

        make_null_move(p);
        md->current_move = null_move;
        md->counter_move_history = p->my_thread->counter_move_history[no_piece][0];
        ++my_thread->nodes;
        int null_eval = -alpha_beta(p, md+1, -beta, -beta + 1, depth - R, false);
        undo_null_move(p);

        if (null_eval >= beta) {
            if (depth < 10) {
                return null_eval;
            }

            my_thread->nmp_enabled = false;
            int verification = alpha_beta(p, md, beta - 1, beta, depth - R, false);
            my_thread->nmp_enabled = true;
            if (verification >= beta) {
                return verification;
            }
        }
    }

    // Probcut
    Move move;
    if (!in_check && !is_pv && depth > 4 && std::abs(beta) < MATE_IN_MAX_PLY) {
        int rbeta = std::min(beta + 100, MATE);
        MoveGen movegen = new_movegen(p, md, tte_move, PROBCUT_SEARCH, rbeta - md->static_eval, false);

        while ((move = next_move(&movegen, md, depth)) != no_move) {
            if (move != excluded_move && is_legal(p, move)) {
                make_move(p, move);
                md->current_move = move;
                Square to = move_to(move);
                Piece piece = p->pieces[to];
                md->counter_move_history = p->my_thread->counter_move_history[piece][to];
                int value = -alpha_beta(p, md+1, -rbeta, -rbeta +  1, depth - 4, is_checked(p));
                undo_move(p, move);

                if (value >= rbeta) {
                    return value;
                }
            }
        }
    }

    MoveGen movegen = new_movegen(p, md, tte_move, NORMAL_SEARCH, 0, in_check);

    Move best_move = no_move;
    Move quiets[64];
    int quiets_count = 0;
    int best_score = -INFINITE;
    int num_moves = 0;

    bool improving = !in_check && ply > 1 && (md->static_eval >= (md-2)->static_eval || (md-2)->static_eval == UNDEFINED);

    while ((move = next_move(&movegen, md, depth)) != no_move) {
        assert(is_pseudolegal(p, move));
        assert(is_move_valid(move));
        assert(0 < depth || in_check);

        if (move == excluded_move) {
            continue;
        }

        if (root_node && root_moves.find(move) == root_moves.end()) {
            continue;
        }

        ++num_moves;
        assert(!(movegen.stage == QUIETS && move_type(move) == PROMOTION));
        bool checks = gives_check(p, move);
        bool capture_or_promo = is_capture_or_promotion(p, move);
        bool important = capture_or_promo || checks || move == tte_move || is_advanced_pawn_push(p, move) || best_score <= MATED_IN_MAX_PLY;

        int extension = 0;
        if (checks && see_capture(p, move, 0)) {
            extension = 1;
        } else if (depth >= 8 &&
            move == tte_move &&
            !root_node &&
            excluded_move == no_move &&
            std::abs(tte_score) < TB_WIN &&
            (tt_flag & FLAG_BETA) &&
            tte->depth >= depth - 2 &&
            is_legal(p, move))
        {
            assert(tte_score != UNDEFINED);
            int rbeta = std::max(tte_score - 2 * depth, -MATE + 1);
            md->excluded_move = move;
            int singular_value = alpha_beta(p, md, rbeta - 1, rbeta, depth / 2, in_check);
            md->excluded_move = no_move;

            if (singular_value < rbeta) {
                extension = 1;
            }
        }

        // History score has to be obtained before calling make move which changes the position
        int history, cmh, fmh;
        get_history_scores(p, md, move, &history, &cmh, &fmh);

        if (!root_node && !important && depth < 8 && num_moves >= futility_move_counts[improving][depth]) {
            continue;
        }

        if (!root_node && !important && depth < 5 && cmh < 0 && fmh < 0) {
            continue;
        }

        if (!important && depth < 9 && !see_capture(p, move, -10 * depth * depth)) {
            continue;
        }

        if (!is_pv && best_score > MATED_IN_MAX_PLY && !see_capture(p, move, -PAWN_END * depth)) {
            continue;
        }

        if (!is_legal(p, move)) {
            --num_moves;
            continue;
        }

        make_move(p, move);
        ++my_thread->nodes;
        md->current_move = move;
        Square to = move_to(move);
        Piece piece = p->pieces[to];
        md->counter_move_history = p->my_thread->counter_move_history[piece][to];
        if (!capture_or_promo && quiets_count < 64) {
            quiets[quiets_count++] = move;
        }

        int new_depth = depth - 1 + extension;
        int score;

        if (is_pv && num_moves == 1) {
            score = -alpha_beta(p, md+1, -beta, -alpha, new_depth, checks);
        } else {
            // late move reductions
            int reduction = 0;
            if (depth >= 3 && num_moves > 1 && !capture_or_promo) {
                reduction = lmr(is_pv, depth, num_moves);
                if (!is_pv && !improving) {
                    ++reduction;
                }

                if (move == md->killers[0] || move == md->killers[1] || move == movegen.counter_move) {
                    --reduction;
                }

                int quiet_score = history + cmh + fmh;
                reduction = std::max(reduction - quiet_score / 12288, 0);
            }

            score = -alpha_beta(p, md+1, -alpha - 1, -alpha, new_depth - reduction, checks);

            if (reduction > 0 && score > alpha) {
                score = -alpha_beta(p, md+1, -alpha - 1, -alpha, new_depth, checks);
            }

            if (is_pv && score > alpha && score < beta) {
                score = -alpha_beta(p, md+1, -beta, -alpha, new_depth, checks);
            }
        }
        undo_move(p, move);
        assert(is_timeout || (score >= -MATE && score <= MATE));

        if (is_timeout) {
            return TIMEOUT;
        }

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                if (is_pv && is_main_thread(p)) {
                    set_pv(move, md);
                }
                best_move = move;
                if (is_pv && score < beta) {
                    alpha = score;
                } else {
                    if (!capture_or_promo) {
                        save_killer(p, md, move, depth, quiets, quiets_count - 1);
                    }
                    if (excluded_move == no_move) {
                        set_tte(pos_hash, tte, move, depth, score_to_tt(score, ply), md->static_eval, FLAG_BETA);
                    }
                    return score;
                }
            }
        }
    }

    if (num_moves == 0) {
        best_score = excluded_move != no_move ? alpha : in_check ? -MATE + ply : 0;
    }

    if (excluded_move == no_move) {
        uint8_t flag = is_pv && best_move ? FLAG_EXACT : FLAG_ALPHA;
        set_tte(pos_hash, tte, best_move, depth, score_to_tt(best_score, ply), md->static_eval, flag);
    }
    if (!in_check && best_move && !is_capture_or_promotion(p, best_move)) {
        save_killer(p, md, best_move, depth, quiets, quiets_count - 1);
    }
    assert(best_score >= -MATE && best_score <= MATE);
    return best_score;
}

void print_info(Position *p, Metadata *md, int depth, int score, int alpha, int beta, bool pv_printed) {
    SearchThread *my_thread = p->my_thread;
    gettimeofday(&curr_time, nullptr);
    int time_taken = time_passed();
    uint64_t tb_hits = sum_tb_hits();
    std::cout << "info depth " << depth << " seldepth " << (my_thread->selply + 1) << " multipv 1 ";
    std::cout << "tbhits " << tb_hits << " score ";

    if (score <= MATED_IN_MAX_PLY) {
        std::cout << "mate " << ((-MATE - score) / 2 + 1);
    } else if (score >= MATE_IN_MAX_PLY) {
        std::cout << "mate " << ((MATE - score) / 2 + 1);
    } else {
        std::cout << "cp " << score * 100 / PAWN_END;
    }

    if (score <= alpha) {
        std::cout << " upperbound";
    } else if (score >= beta) {
        std::cout << " lowerbound";
    }

    std::cout << " hashfull " << hashfull();

    uint64_t nodes = sum_nodes();
    std::cout << " nodes " << nodes <<  " nps " << nodes*1000/(time_taken+1) << " time " << time_taken << " pv ";
    if (pv_printed) {
        print_pv(p, md);
    } else {
        // For fail lows, only print the first move of the main pv
        std::cout << move_to_str(p, main_pv[0]);
    }
    std::cout << std::endl;
}

void thread_think(SearchThread *my_thread, bool in_check) {
    Position *p = &my_thread->position;
    Metadata *md = &my_thread->metadatas[2]; // Start from 2 so that we can do (md-2) without checking
    bool is_main = is_main_thread(p);

    Move pv_at_depth[MAX_PLY * 2];

    int previous = -MATE;
    int score = -MATE;
    int init_remain = myremain;
    int init_total_remaining = total_remaining;
    int depth = 0;

    while (++depth <= think_depth_limit) {
        my_thread->selply = 0;

        int aspiration = 10;
        int alpha = -MATE;
        int beta = MATE;

        if (depth >= 5) {
            alpha = std::max(previous - aspiration, -MATE);
            beta = std::min(previous + aspiration, MATE);
        }

        bool failed_low = false;

        while (true) {
            score = alpha_beta(p, md, alpha, beta, depth, in_check);

            if (is_timeout) {
                break;
            }

            // Only set main pv when it's not a fail low
            if (is_main && score > alpha) {
                set_main_pv(md);
            }

            if (is_main && (score <= alpha || score >= beta) && depth > 12) {
                print_info(p, md, depth, score, alpha, beta, false);
            }

            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(score - aspiration, -MATE);
                failed_low = true;
            } else if (score >= beta) {
                beta = std::min(score + aspiration, MATE);
            } else {
                break;
            }

            aspiration += aspiration / 2;
            assert(alpha >= -MATE && beta <= MATE);
        }

        previous = score;

        if (is_timeout) {
            break;
        }

        if (!is_main) {
            continue;
        }

        print_info(p, md, depth, score, alpha, beta, true);

        if (time_passed() > myremain && !is_pondering) {
            is_timeout = true;
            break;
        }

        // At this point, it is guaranteed that the current depth has finished
        // so it's safe to assume that the pv and ponder are valid.
        pv_at_depth[depth] = latest_pv = main_pv[0];
        latest_ponder = main_pv[1];

        if (depth >= 6 && !is_movetime) {
            if (failed_low) {
                myremain = myremain * (200 + std::min(depth, 20)) / 200;
            }
            if (pv_at_depth[depth] == pv_at_depth[depth - 1]) {
                myremain = std::max(init_remain / 2, myremain * 94 / 100);
                total_remaining = std::min(myremain * 6, total_remaining);
            } else {
                myremain = std::max(init_remain, myremain);
                total_remaining = std::max(init_total_remaining, total_remaining);
            }
        }
    }
}

void think(Position *p, std::vector<std::string> word_list) {
    init_time(p, word_list);

    // Set the table generation
    start_search();

    // First check TB
    Move tb_move;
    bool in_check = is_checked(p);
    Metadata *md = &main_thread.metadatas[2]; // Start from 2 so that we can do (md-2) without checking

    // Clear root moves
    root_moves.clear();

    int wdl = probe_syzygy_dtz(p, &tb_move);
    if (wdl != SYZYGY_FAIL) {
        // Return draws immediately
        if (wdl == SYZYGY_DRAW) {
            while (is_pondering) {}
            std::cout << "info score cp 0" << std::endl;
            std::cout << "bestmove " << move_to_str(p, tb_move) << std::endl;
            return;
        }
        root_moves.insert(tb_move);
    } else {
        MoveGen movegen = new_movegen(p, md, no_move, NORMAL_SEARCH, 0, in_check);
        Move move;
        while ((move = next_move(&movegen, md, 0)) != no_move) {
            if (is_legal(p, move)) {
                root_moves.insert(move);
            }
        }
        if (root_moves.size() == 1) {
            while (is_pondering) {}
            std::cout << "bestmove " << move_to_str(p, *root_moves.begin()) << std::endl;
            return;
        }
        if (root_moves.size() == 0) {
            while (is_pondering) {}
            std::cout << "bestmove none" << std::endl;
            return;
        }
    }

    gettimeofday(&start_ts, nullptr);

    std::thread *threads = new std::thread[num_threads];
    initialize_nodes();

    is_searching = true;
    for (int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread(thread_think, get_thread(i), in_check);
    }

    // Stop threads
    for (int i = 0; i < num_threads; ++i) {
        threads[i].join();
    }

    delete[] threads;

    // If search is stopped for some reason while pondering, wait before printing best move
    while (is_pondering) {}

    gettimeofday(&curr_time, nullptr);
    std::cout << "info time " << time_passed() << std::endl;
    std::cout << "bestmove " << move_to_str(p, main_pv[0]);

    if (main_pv[0] == latest_pv && is_move_valid(latest_ponder)) {
        std::cout << " ponder " << move_to_str(p, latest_ponder);
    }

    std::cout << std::endl;

    if (quit_application) {
        exit(EXIT_SUCCESS);
    }
    is_searching = false;
}

void bench() {
    uint64_t nodes = 0;
    std::vector<std::string> empty_word_list;

    struct timeval bench_start, bench_end;
    gettimeofday(&bench_start, nullptr);
    int tmp_depth = think_depth_limit;
    int tmp_myremain = myremain;
    think_depth_limit = 13;
    is_timeout = false;

    for (int i = 0; i < 36; i++){
        std::cout << "\nPosition [" << (i + 1) << "|36]\n" << std::endl;
        Position *p = import_fen(benchmarks[i], 0);

        myremain = 3600000;
        think(p, empty_word_list);
        nodes += main_thread.nodes;

        clear_tt();
    }

    gettimeofday(&bench_end, nullptr);
    int time_taken = bench_time(bench_start, bench_end);
    think_depth_limit = tmp_depth;
    myremain = tmp_myremain;

    std::cout << "\n------------------------\n";
    std::cout << "Time  : " << time_taken << std::endl;
    std::cout << "Nodes : " << nodes << std::endl;
    std::cout << "NPS   : " << nodes * 1000 / (time_taken + 1) << std::endl;
}

