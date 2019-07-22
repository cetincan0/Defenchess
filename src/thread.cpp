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
    SearchThread *main_thread = &search_threads[0];
    main_thread->root_ply = main_thread->search_ply;

    // Copy over the root position
    for (int i = 1; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->root_ply = t->search_ply = main_thread->root_ply;

        // Need to fully copy the position and info
        std::memcpy(&t->position, &main_thread->position, sizeof(Position));
        Position *t_position = &t->position;

        // Also copy the info
        std::memcpy(t->infos + main_thread->root_ply, main_thread->infos + main_thread->root_ply, sizeof(Info));
        t_position->info = &t->infos[main_thread->root_ply];
        t_position->my_thread = t;
    }

    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->depth = 1;

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
        }
    }
}

void clear_threads() {
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *search_thread = &search_threads[i];
        // Clear history
        for (int j = 0; j < 64; ++j) {
            for (int k = 0; k < 64; ++k) {
                search_thread->history[white][j][k] = 0;
                search_thread->history[black][j][k] = 0;
            }
        }
        // Clear counter moves
        for (int j = 0; j < NUM_PIECE; ++j) {
            for (int k = 0; k < 64; ++k) {
                search_thread->counter_moves[j][k] = no_move;
            }
        }
        // Clear counter move history
        for (int j = 0; j < NUM_PIECE; ++j) {
            for (int k = 0; k < 64; ++k) {
                for (int l = 0; l < NUM_PIECE; ++l) {
                    for (int m = 0; m < 64; ++m) {
                        search_thread->counter_move_history[j][k][l][m] = 0;
                    }
                }
            }
        }
    }
}

void reset_threads() {
    search_threads = (SearchThread*) realloc(search_threads, num_threads * sizeof(SearchThread));

    for (int i = 0; i < num_threads; ++i) {
        search_threads[i].thread_id = i;
    }
    clear_threads();
}

void init_threads() {
    search_threads = (SearchThread*) malloc(num_threads * sizeof(SearchThread));

    for (int i = 0; i < num_threads; ++i) {
        search_threads[i].thread_id = i;
    }
    clear_threads();
}

