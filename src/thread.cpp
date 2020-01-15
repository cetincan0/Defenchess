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

#include <cstring>

#include "const.h"
#include "data.h"
#include "thread.h"

void get_ready() {
    main_thread.root_ply = main_thread.search_ply;

    // Copy over the root position
    for (int i = 1; i < num_threads; ++i) {
        SearchThread *t = get_thread(i);
        t->root_ply = t->search_ply = main_thread.root_ply;

        // Need to fully copy the position and info
        std::memcpy(&t->position, &main_thread.position, sizeof(Position));
        Position *t_position = &t->position;

        // Also copy the info
        std::memcpy(t->infos + main_thread.root_ply, main_thread.infos + main_thread.root_ply, sizeof(Info));
        t_position->info = &t->infos[main_thread.root_ply];
        t_position->my_thread = t;
    }

    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = get_thread(i);
        t->nmp_enabled = true;

        // Clear the metadata
        for (int j = 0; j < MAX_PLY + 2; ++j) {
            Metadata *md = &t->metadatas[j];
            md->current_move = no_move;
            md->static_eval = UNDEFINED;
            md->ply = 0;
            md->killers[0] = no_move;
            md->killers[1] = no_move;
            md->pv[0] = no_move;
            md->excluded_move = no_move;
            md->counter_move_history = t->counter_move_history[no_piece][0];
        }
    }
}

void delete_histories(SearchThread *t) {
    for (int j = 0; j < NUM_PIECE; ++j) {
        for (int k = 0; k < 64; ++k) {
            for (int l = 0; l < NUM_PIECE; ++l) {
                delete[] t->counter_move_history[j][k][l];
            }
            delete[] t->counter_move_history[j][k];
        }
    }
}

void init_histories(SearchThread *t) {
    for (int j = 0; j < NUM_PIECE; ++j) {
        for (int k = 0; k < 64; ++k) {
            t->counter_move_history[j][k] = new int*[NUM_PIECE];
            for (int l = 0; l < NUM_PIECE; ++l) {
                t->counter_move_history[j][k][l] = new int[64];
            }
        }
    }
}

void clear_threads() {
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *search_thread = get_thread(i);

        // Clear history
        std::memset(&search_thread->history, 0, sizeof(search_thread->history));

        init_histories(search_thread);
        for (int j = 0; j < NUM_PIECE; ++j) {
            for (int k = 0; k < 64; ++k) {
                for (int l = 0; l < NUM_PIECE; ++l) {
                    for (int m = 0; m < 64; ++m) {
                        search_thread->counter_move_history[j][k][l][m] = j == no_piece ? -1 : 0;
                    }
                }
            }
        }

        // Clear counter moves
        for (int j = 0; j < NUM_PIECE; ++j) {
            for (int k = 0; k < 64; ++k) {
                search_thread->counter_moves[j][k] = no_move;
            }
        }
    }
}

void reset_threads(int thread_num) {
    for (int i = 0; i < num_threads; ++i) {
        // Only delete histories of existing threads
        delete_histories(get_thread(i));
    }

    num_threads = thread_num;
    delete[] search_threads;
    search_threads = new SearchThread[num_threads - 1];

    for (int i = 1; i < thread_num; ++i) {
        get_thread(i)->thread_id = i;
    }
    clear_threads();
    get_ready();
}

void init_threads() {
    search_threads = new SearchThread[num_threads - 1];

    for (int i = 0; i < num_threads; ++i) {
        get_thread(i)->thread_id = i;
    }
    clear_threads();
}

