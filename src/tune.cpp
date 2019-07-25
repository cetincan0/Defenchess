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

#ifdef __TUNE__

#include <cmath>
#include <fstream>
#include <thread>

#include "data.h"
#include "move.h"
#include "position.h"
#include "pst.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "tune.h"

using namespace std;

void fen_split(string s, vector<string> &f) {
    unsigned l_index = 0;
    for (unsigned i = 0 ; i < s.length() ; i++) {
        if (s[i] == '|') {
            f.push_back(s.substr(l_index, i - l_index));
            l_index = i + 1;
        }
        if (i == s.length() - 1) {
            f.push_back(s.substr(l_index));
        }
    }
}

vector<long double> diffs[MAX_THREADS];
vector<string> entire_file;
uint64_t num_fens;
long double k = 0.93L;

long double sigmoid(long double s) {
    return 1.0L / (1.0L + pow(10.0L, -k * s / 400.0L));
}

void single_error(int thread_id) {
    for (unsigned i = thread_id; i < num_fens; i += num_threads) {
        string line = entire_file[i];

        vector<string> fen_info;
        fen_split(line, fen_info);

        Position *p = import_fen(fen_info[0], thread_id);
        if (is_checked(p)) {
            continue;
        }

        Metadata *md = &p->my_thread->metadatas[0];
        md->current_move = no_move;
        md->static_eval = UNDEFINED;
        md->ply = 0;

        string result_str = fen_info[1];
        long double result;
        if (result_str == "1-0") {
            result = 1.0L;
        } else if (result_str == "0-1") {
            result = 0.0L;
        } else if (result_str == "1/2-1/2") {
            result = 0.5L;
        } else {
            result = -1.0L;
            exit(1);
        }
        int qi = alpha_beta_quiescence(p, md, -MATE, MATE, -1, false);
        qi = p->color == white ? qi : -qi;

        diffs[thread_id].push_back(pow(result - sigmoid((long double) qi), 2.0L));
    }
}

void set_parameter(Parameter *param) {
    *param->variable = param->value;
    init_values();
    init_imbalance();
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
long double kahansum() {
    long double sum, c, y, t;
    sum = 0.0L;
    c = 0.0L;
    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        for (unsigned i = 0; i < diffs[thread_id].size(); ++i) {
            y = diffs[thread_id][i] - c;
            t = sum + y;
            c = (t - sum) - y;
            sum = t;
        }
    }
    return sum;
}
#pragma GCC pop_options

long double find_error(vector<Parameter> params) {
    for (unsigned i = 0; i < params.size(); ++i) {
        Parameter *param = &params[i];
        set_parameter(param);
    }

    std::thread threads[32];
    for (int i = 0; i < num_threads; ++i) {
        diffs[i].clear();
        threads[i] = thread(single_error, i);
    }

    unsigned total_size = 0;
    for (int i = 0; i < num_threads; ++i) {
        threads[i].join();
        total_size += diffs[i].size();
    }
    return kahansum() / ((long double) total_size);
}

void read_entire_file() {
    ifstream fens;
    fens.open("../../fens/allfens2.txt");
    string line;
    while (getline(fens, line)) {
        entire_file.push_back(line);
        ++num_fens;
        if (num_fens % 1000000 == 0) {
            cout << "Reading line " << num_fens << endl;
        }
    }
    cout << "Total lines: " << num_fens << endl;
    fens.close();
}

void find_best_k(vector<Parameter> &parameters) {
    int min = 60, max = 120;

    k = (long double)(min) / 100.0L;
    long double min_err = find_error(parameters);
    cout << "k[" << min << "]:\t" << min_err << endl;

    k = (long double)(max) / 100.0L;
    long double max_err = find_error(parameters);
    cout << "k[" << max << "]:\t" << max_err << endl;

    while (max > min) {
        if (min_err < max_err) {
            if (min == max - 1) {
                k = (long double)(min) / 100.0L;
                return;
            }
            max = min + (max - min) / 2;
            k = (long double)(max) / 100.0L;
            max_err = find_error(parameters);
            cout << "k[" << max << "]:\t" << max_err << endl;
        } else {
            if (min == max - 1) {
                k = (long double)(max) / 100.0L;
                return;
            }
            min = min + (max - min) / 2;
            k = (long double)(min) / 100.0L;
            min_err = find_error(parameters);
            cout << "k[" << min << "]:\t" << min_err << endl;
        }
    }
}

void binary_search_parameters(vector<Parameter> &parameters) {
    for (unsigned i = 0; i < parameters.size(); ++i) {
        Parameter *param = &parameters[i];
        int delta = max(10, abs(param->value / 5));
        int min = param->value - delta, max = param->value + delta;
        if (param->name == "PAWN_END") {
            min = PAWN_MID + 1;
        }

        param->value = min;
        long double min_err = find_error(parameters);
        cout << param->name << "[" << param->value << "]:\t" << min_err << endl;

        param->value = max;
        long double max_err = find_error(parameters);
        cout << param->name << "[" << param->value << "]:\t" << max_err << endl;

        while (max > min) {
            if (min_err < max_err) {
                if (min == max - 1) {
                    param->value = min;
                    set_parameter(param);
                    cout << param->name << "[" << param->value << "] (best)" << endl;
                    break;
                }
                max = min + (max - min) / 2;
                param->value = max;
                max_err = find_error(parameters);
                cout << param->name << "[" << param->value << "]:\t" << max_err << endl;
            } else {
                if (min == max - 1) {
                    param->value = max;
                    set_parameter(param);
                    cout << param->name << "[" << param->value << "] (best)" << endl;
                    break;
                }
                min = min + (max - min) / 2;
                param->value = min;
                min_err = find_error(parameters);
                cout << param->name << "[" << param->value << "]:\t" << min_err << endl;
            }
        }
    }
}

void tune() {
    num_threads = 32;
    reset_threads();
    clear_tt();
    cout.precision(32);
    vector<Parameter> best_guess;
    read_entire_file();
    init_parameters(best_guess);
    // init_pst(best_guess);
    // init_mobility(best_guess);

    for (unsigned i = 0; i < best_guess.size(); ++i) {
        cout << best_guess[i].name << endl;
        for (unsigned j = i + 1; j < best_guess.size(); ++j) {
            if (best_guess[i].name == best_guess[j].name) {
                cout << "duplicate " << best_guess[i].name << endl;
                exit(1);
            }
        }
    }

    find_best_k(best_guess);
    cout << "best k: " << k << endl;

    double best_error = find_error(best_guess);
    cout << "initial error:\t" << best_error << endl;

    for (int iteration = 0; iteration < 2; ++iteration) {
        for (unsigned p = 0; p < best_guess.size(); ++p) {
            best_guess[p].stability = 1;
        }

        binary_search_parameters(best_guess);
        bool improving = true;
        while (improving) {
            improving = false;
            for (unsigned pi = 0; pi < best_guess.size(); pi++) {
                if (best_guess[pi].stability >= 3) {
                    continue;
                }

                vector<Parameter> new_guess = best_guess;
                new_guess[pi].value += best_guess[pi].increasing ? 1 : -1;

                double new_error = find_error(new_guess);
                if (new_error < best_error) {
                    best_error = new_error;
                    best_guess = new_guess;
                    best_guess[pi].increasing = true;
                    improving = true;
                    cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << " (best)" << endl;
                    best_guess[pi].stability = 1;
                    continue;
                } else {
                    cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << endl;
                }

                new_guess[pi].value -= best_guess[pi].increasing ? 2 : -2;

                new_error = find_error(new_guess);
                if (new_error < best_error) {
                    best_error = new_error;
                    best_guess = new_guess;
                    best_guess[pi].increasing = false;
                    improving = true;
                    cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << " (best)" << endl;
                    best_guess[pi].stability = 1;
                    continue;
                } else {
                    cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << endl;
                    ++best_guess[pi].stability;
                }
            }

            for (unsigned i = 0; i < best_guess.size(); ++i) {
                Parameter *param = &best_guess[i];
                cout << "best " << param->name << ": " << param->value << endl;
            }
        }
    }

    for (unsigned i = 0; i < best_guess.size(); ++i) {
        Parameter *param = &best_guess[i];
        cout << "best " << param->name << ": " << param->value << endl;
    }

    exit(EXIT_SUCCESS);
}

void init_mobility(vector<Parameter> &parameters) {
    for (int i = 0; i < 4; ++i) {
        for (int j = RANK_1; j < RANK_8; ++j) {
            parameters.push_back({&pawn_shelter[i][j], pawn_shelter[i][j], "pawn_shelter[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }
    for (int b = 0; b <= 1; ++b) {
        for (int i = 0; i < 4; ++i) {
            for (int j = (b ? 2 : 0); j < 8; ++j) {
                parameters.push_back({&pawn_storm[b][i][j], pawn_storm[b][i][j], "pawn_storm[" + to_string(b) + "][" + to_string(i) + "][" + to_string(j) + "]", true, 1});
            }
        }
    }
    return;

    for (int o = 0; o <= 1; ++o) {
        for (int adj = 0; adj <= 1; ++adj) {
            for (int i = RANK_2; i < RANK_8; ++i) {
                if ((o && i == RANK_7) || (!adj && i == RANK_2)) {
                    continue;
                }
                parameters.push_back({&connected_bonus[o][adj][i].midgame, connected_bonus[o][adj][i].midgame, "connected_bonus[" + to_string(o) + "][" + to_string(adj) + "][" + to_string(i) + "].midgame", true, 1});
                parameters.push_back({&connected_bonus[o][adj][i].endgame, connected_bonus[o][adj][i].endgame, "connected_bonus[" + to_string(o) + "][" + to_string(adj) + "][" + to_string(i) + "].endgame", true, 1});
            }
        }
    }
    return;

    for (int i = 0; i <= 8; ++i) {
        parameters.push_back({&mobility_bonus[KNIGHT][i].midgame, mobility_bonus[KNIGHT][i].midgame, "mobility_bonus[KNIGHT][" + to_string(i) + "].midgame", true, 1});
        parameters.push_back({&mobility_bonus[KNIGHT][i].endgame, mobility_bonus[KNIGHT][i].endgame, "mobility_bonus[KNIGHT][" + to_string(i) + "].endgame", true, 1});
    }

    for (int i = 0; i <= 13; ++i) {
        parameters.push_back({&mobility_bonus[BISHOP][i].midgame, mobility_bonus[BISHOP][i].midgame, "mobility_bonus[BISHOP][" + to_string(i) + "].midgame", true, 1});
        parameters.push_back({&mobility_bonus[BISHOP][i].endgame, mobility_bonus[BISHOP][i].endgame, "mobility_bonus[BISHOP][" + to_string(i) + "].endgame", true, 1});
    }

    for (int i = 0; i <= 14; ++i) {
        parameters.push_back({&mobility_bonus[ROOK][i].midgame, mobility_bonus[ROOK][i].midgame, "mobility_bonus[ROOK][" + to_string(i) + "].midgame", true, 1});
        parameters.push_back({&mobility_bonus[ROOK][i].endgame, mobility_bonus[ROOK][i].endgame, "mobility_bonus[ROOK][" + to_string(i) + "].endgame", true, 1});
    }

    for (int i = 0; i <= 28; ++i) {
        parameters.push_back({&mobility_bonus[QUEEN][i].midgame, mobility_bonus[QUEEN][i].midgame, "mobility_bonus[QUEEN][" + to_string(i) + "].midgame", true, 1});
        parameters.push_back({&mobility_bonus[QUEEN][i].endgame, mobility_bonus[QUEEN][i].endgame, "mobility_bonus[QUEEN][" + to_string(i) + "].endgame", true, 1});
    }
}

void init_pst(vector<Parameter> &parameters) {
    for (int i = 0; i < 2; ++i) {
        for (int j = 4; j < 28; ++j) {
            parameters.push_back({&bonusPawn[i][j], bonusPawn[i][j], "bonusPawn[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 32; ++j) {
            parameters.push_back({&bonusKnight[i][j], bonusKnight[i][j], "bonusKnight[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 32; ++j) {
            parameters.push_back({&bonusBishop[i][j], bonusBishop[i][j], "bonusBishop[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 32; ++j) {
            parameters.push_back({&bonusRook[i][j], bonusRook[i][j], "bonusRook[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 32; ++j) {
            parameters.push_back({&bonusQueen[i][j], bonusQueen[i][j], "bonusQueen[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 32; ++j) {
            parameters.push_back({&bonusKing[i][j], bonusKing[i][j], "bonusKing[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }
}

void init_parameters(vector<Parameter> &parameters) {
    // DO NOT TUNE THIS parameters.push_back({&PAWN_MID, PAWN_MID, "PAWN_MID", true});
    parameters.push_back({&PAWN_END, PAWN_END, "PAWN_END", true, 1});

    parameters.push_back({&KNIGHT_MID, KNIGHT_MID, "KNIGHT_MID", true, 1});
    parameters.push_back({&KNIGHT_END, KNIGHT_END, "KNIGHT_END", true, 1});

    parameters.push_back({&BISHOP_MID, BISHOP_MID, "BISHOP_MID", true, 1});
    parameters.push_back({&BISHOP_END, BISHOP_END, "BISHOP_END", true, 1});

    parameters.push_back({&ROOK_MID, ROOK_MID, "ROOK_MID", true, 1});
    parameters.push_back({&ROOK_END, ROOK_END, "ROOK_END", true, 1});

    parameters.push_back({&QUEEN_MID, QUEEN_MID, "QUEEN_MID", true, 1});
    parameters.push_back({&QUEEN_END, QUEEN_END, "QUEEN_END", true, 1});

    parameters.push_back({&pawn_push_threat_bonus.midgame, pawn_push_threat_bonus.midgame, "pawn_push_threat_bonus.midgame", true, 1});
    parameters.push_back({&pawn_push_threat_bonus.endgame, pawn_push_threat_bonus.endgame, "pawn_push_threat_bonus.endgame", true, 1});

    parameters.push_back({&bishop_pawn_penalty.midgame, bishop_pawn_penalty.midgame, "bishop_pawn_penalty.midgame", true, 1});
    parameters.push_back({&bishop_pawn_penalty.endgame, bishop_pawn_penalty.endgame, "bishop_pawn_penalty.endgame", true, 1});

    parameters.push_back({&double_pawn_penalty.midgame, double_pawn_penalty.midgame, "double_pawn_penalty.midgame", true, 1});
    parameters.push_back({&double_pawn_penalty.endgame, double_pawn_penalty.endgame, "double_pawn_penalty.endgame", true, 1});

    parameters.push_back({&queen_check_penalty, queen_check_penalty, "queen_check_penalty", true, 1});
    parameters.push_back({&knight_check_penalty, knight_check_penalty, "knight_check_penalty", true, 1});
    parameters.push_back({&rook_check_penalty, rook_check_penalty, "rook_check_penalty", true, 1});
    parameters.push_back({&bishop_check_penalty, bishop_check_penalty, "bishop_check_penalty", true, 1});
    parameters.push_back({&pawn_distance_penalty, pawn_distance_penalty, "pawn_distance_penalty", true, 1});
    parameters.push_back({&king_zone_attack_penalty, king_zone_attack_penalty, "king_zone_attack_penalty", true, 1});
    parameters.push_back({&king_danger_shelter_bonus, king_danger_shelter_bonus, "king_danger_shelter_bonus", true, 1});
    parameters.push_back({&king_danger_queen_penalty, king_danger_queen_penalty, "king_danger_queen_penalty", true, 1});
    parameters.push_back({&king_danger_pinned_penalty, king_danger_pinned_penalty, "king_danger_pinned_penalty", true, 1});
    parameters.push_back({&king_danger_init, king_danger_init, "king_danger_init", true, 1});
    parameters.push_back({&king_danger_weak_penalty, king_danger_weak_penalty, "king_danger_weak_penalty", true, 1});
    parameters.push_back({&king_danger_weak_zone_penalty, king_danger_weak_zone_penalty, "king_danger_weak_zone_penalty", true, 1});

    for (int i = 0; i < 4; ++i) {
        for (int j = RANK_1; j < RANK_8; ++j) {
            parameters.push_back({&pawn_shelter[i][j], pawn_shelter[i][j], "pawn_shelter[" + to_string(i) + "][" + to_string(j) + "]", true, 1});
        }
    }

    for (int b = 0; b <= 1; ++b) {
        for (int i = 0; i < 4; ++i) {
            for (int j = (b ? 2 : 0); j < 8; ++j) {
                parameters.push_back({&pawn_storm[b][i][j], pawn_storm[b][i][j], "pawn_storm[" + to_string(b) + "][" + to_string(i) + "][" + to_string(j) + "]", true, 1});
            }
        }
    }

    for (int o = 0; o <= 1; ++o) {
        for (int adj = 0; adj <= 1; ++adj) {
            for (int i = RANK_2; i < RANK_8; ++i) {
                if ((o && i == RANK_7) || (!adj && i == RANK_2)) {
                    continue;
                }
                parameters.push_back({&connected_bonus[o][adj][i].midgame, connected_bonus[o][adj][i].midgame, "connected_bonus[" + to_string(o) + "][" + to_string(adj) + "][" + to_string(i) + "].midgame", true, 1});
                parameters.push_back({&connected_bonus[o][adj][i].endgame, connected_bonus[o][adj][i].endgame, "connected_bonus[" + to_string(o) + "][" + to_string(adj) + "][" + to_string(i) + "].endgame", true, 1});
            }
        }
    }

    parameters.push_back({&ATTACK_VALUES[2], ATTACK_VALUES[2], "ATTACK_VALUES[2]", true, 1});
    parameters.push_back({&ATTACK_VALUES[3], ATTACK_VALUES[3], "ATTACK_VALUES[3]", true, 1});
    parameters.push_back({&ATTACK_VALUES[4], ATTACK_VALUES[4], "ATTACK_VALUES[4]", true, 1});
    parameters.push_back({&ATTACK_VALUES[5], ATTACK_VALUES[5], "ATTACK_VALUES[5]", true, 1});

    for (int i = 0; i < 7; ++i) {
        parameters.push_back({&passer_my_distance[i], passer_my_distance[i], "passer_my_distance[" + to_string(i) + "]", true, 1});
        parameters.push_back({&passer_enemy_distance[i], passer_enemy_distance[i], "passer_enemy_distance[" + to_string(i) + "]", true, 1});
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 7; ++j) {
            parameters.push_back({&passer_blocked[i][j].midgame, passer_blocked[i][j].midgame, "passer_blocked[" + to_string(i) + "][" + to_string(j) + "].midgame", true, 1});
            parameters.push_back({&passer_blocked[i][j].endgame, passer_blocked[i][j].endgame, "passer_blocked[" + to_string(i) + "][" + to_string(j) + "].endgame", true, 1});
            parameters.push_back({&passer_unsafe[i][j].midgame, passer_unsafe[i][j].midgame, "passer_unsafe[" + to_string(i) + "][" + to_string(j) + "].midgame", true, 1});
            parameters.push_back({&passer_unsafe[i][j].endgame, passer_unsafe[i][j].endgame, "passer_unsafe[" + to_string(i) + "][" + to_string(j) + "].endgame", true, 1});
        }
    }
    // parameters.push_back({&passed_pawn_bonus[0].midgame, passed_pawn_bonus[0].midgame, "passed_pawn_bonus[0].midgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[0].endgame, passed_pawn_bonus[0].endgame, "passed_pawn_bonus[0].endgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[1].midgame, passed_pawn_bonus[1].midgame, "passed_pawn_bonus[1].midgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[1].endgame, passed_pawn_bonus[1].endgame, "passed_pawn_bonus[1].endgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[2].midgame, passed_pawn_bonus[2].midgame, "passed_pawn_bonus[2].midgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[2].endgame, passed_pawn_bonus[2].endgame, "passed_pawn_bonus[2].endgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[3].midgame, passed_pawn_bonus[3].midgame, "passed_pawn_bonus[3].midgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[3].endgame, passed_pawn_bonus[3].endgame, "passed_pawn_bonus[3].endgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[4].midgame, passed_pawn_bonus[4].midgame, "passed_pawn_bonus[4].midgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[4].endgame, passed_pawn_bonus[4].endgame, "passed_pawn_bonus[4].endgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[5].midgame, passed_pawn_bonus[5].midgame, "passed_pawn_bonus[5].midgame", true, 1});
    // parameters.push_back({&passed_pawn_bonus[5].endgame, passed_pawn_bonus[5].endgame, "passed_pawn_bonus[5].endgame", true, 1});

    parameters.push_back({&rook_file_bonus[0].midgame, rook_file_bonus[0].midgame, "rook_file_bonus[0].midgame", true, 1});
    parameters.push_back({&rook_file_bonus[0].endgame, rook_file_bonus[0].endgame, "rook_file_bonus[0].endgame", true, 1});
    parameters.push_back({&rook_file_bonus[1].midgame, rook_file_bonus[1].midgame, "rook_file_bonus[1].midgame", true, 1});
    parameters.push_back({&rook_file_bonus[1].endgame, rook_file_bonus[1].endgame, "rook_file_bonus[1].endgame", true, 1});

    parameters.push_back({&isolated_pawn_penalty[0].midgame, isolated_pawn_penalty[0].midgame, "isolated_pawn_penalty[0].midgame", true, 1});
    parameters.push_back({&isolated_pawn_penalty[0].endgame, isolated_pawn_penalty[0].endgame, "isolated_pawn_penalty[0].endgame", true, 1});
    parameters.push_back({&isolated_pawn_penalty[1].midgame, isolated_pawn_penalty[1].midgame, "isolated_pawn_penalty[1].midgame", true, 1});
    parameters.push_back({&isolated_pawn_penalty[1].endgame, isolated_pawn_penalty[1].endgame, "isolated_pawn_penalty[1].endgame", true, 1});

    parameters.push_back({&backward_pawn_penalty[0].midgame, backward_pawn_penalty[0].midgame, "backward_pawn_penalty[0].midgame", true, 1});
    parameters.push_back({&backward_pawn_penalty[0].endgame, backward_pawn_penalty[0].endgame, "backward_pawn_penalty[0].endgame", true, 1});
    parameters.push_back({&backward_pawn_penalty[1].midgame, backward_pawn_penalty[1].midgame, "backward_pawn_penalty[1].midgame", true, 1});
    parameters.push_back({&backward_pawn_penalty[1].endgame, backward_pawn_penalty[1].endgame, "backward_pawn_penalty[1].endgame", true, 1});

    parameters.push_back({&minor_threat_bonus[1].midgame, minor_threat_bonus[1].midgame, "minor_threat_bonus[1].midgame", true, 1});
    parameters.push_back({&minor_threat_bonus[1].endgame, minor_threat_bonus[1].endgame, "minor_threat_bonus[1].endgame", true, 1});
    parameters.push_back({&minor_threat_bonus[2].midgame, minor_threat_bonus[2].midgame, "minor_threat_bonus[2].midgame", true, 1});
    parameters.push_back({&minor_threat_bonus[2].endgame, minor_threat_bonus[2].endgame, "minor_threat_bonus[2].endgame", true, 1});
    parameters.push_back({&minor_threat_bonus[3].midgame, minor_threat_bonus[3].midgame, "minor_threat_bonus[3].midgame", true, 1});
    parameters.push_back({&minor_threat_bonus[3].endgame, minor_threat_bonus[3].endgame, "minor_threat_bonus[3].endgame", true, 1});
    parameters.push_back({&minor_threat_bonus[4].midgame, minor_threat_bonus[4].midgame, "minor_threat_bonus[4].midgame", true, 1});
    parameters.push_back({&minor_threat_bonus[4].endgame, minor_threat_bonus[4].endgame, "minor_threat_bonus[4].endgame", true, 1});
    parameters.push_back({&minor_threat_bonus[5].midgame, minor_threat_bonus[5].midgame, "minor_threat_bonus[5].midgame", true, 1});
    parameters.push_back({&minor_threat_bonus[5].endgame, minor_threat_bonus[5].endgame, "minor_threat_bonus[5].endgame", true, 1});

    parameters.push_back({&rook_threat_bonus[1].midgame, rook_threat_bonus[1].midgame, "rook_threat_bonus[1].midgame", true, 1});
    parameters.push_back({&rook_threat_bonus[1].endgame, rook_threat_bonus[1].endgame, "rook_threat_bonus[1].endgame", true, 1});
    parameters.push_back({&rook_threat_bonus[2].midgame, rook_threat_bonus[2].midgame, "rook_threat_bonus[2].midgame", true, 1});
    parameters.push_back({&rook_threat_bonus[2].endgame, rook_threat_bonus[2].endgame, "rook_threat_bonus[2].endgame", true, 1});
    parameters.push_back({&rook_threat_bonus[3].midgame, rook_threat_bonus[3].midgame, "rook_threat_bonus[3].midgame", true, 1});
    parameters.push_back({&rook_threat_bonus[3].endgame, rook_threat_bonus[3].endgame, "rook_threat_bonus[3].endgame", true, 1});
    // No rook threat bonus 4
    parameters.push_back({&rook_threat_bonus[5].midgame, rook_threat_bonus[5].midgame, "rook_threat_bonus[5].midgame", true, 1});
    parameters.push_back({&rook_threat_bonus[5].endgame, rook_threat_bonus[5].endgame, "rook_threat_bonus[5].endgame", true, 1});

    parameters.push_back({&pawn_threat_bonus[2].midgame, pawn_threat_bonus[2].midgame, "pawn_threat_bonus[2].midgame", true, 1});
    parameters.push_back({&pawn_threat_bonus[2].endgame, pawn_threat_bonus[2].endgame, "pawn_threat_bonus[2].endgame", true, 1});
    parameters.push_back({&pawn_threat_bonus[3].midgame, pawn_threat_bonus[3].midgame, "pawn_threat_bonus[3].midgame", true, 1});
    parameters.push_back({&pawn_threat_bonus[3].endgame, pawn_threat_bonus[3].endgame, "pawn_threat_bonus[3].endgame", true, 1});
    parameters.push_back({&pawn_threat_bonus[4].midgame, pawn_threat_bonus[4].midgame, "pawn_threat_bonus[4].midgame", true, 1});
    parameters.push_back({&pawn_threat_bonus[4].endgame, pawn_threat_bonus[4].endgame, "pawn_threat_bonus[4].endgame", true, 1});
    parameters.push_back({&pawn_threat_bonus[5].midgame, pawn_threat_bonus[5].midgame, "pawn_threat_bonus[5].midgame", true, 1});
    parameters.push_back({&pawn_threat_bonus[5].endgame, pawn_threat_bonus[5].endgame, "pawn_threat_bonus[5].endgame", true, 1});
}

#endif
