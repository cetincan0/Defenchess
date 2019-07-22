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

#include "timecontrol.h"
#include <algorithm>
#include <cmath>

TTime moves_in_time(int increment, int remaining, int movestogo){
    int importance;
    importance = 5 * std::sqrt(movestogo);

    int average_time = remaining / movestogo;
    int extra = average_time * importance * 3 / 200;
    int spend = average_time + extra + increment * (movestogo - 1) / movestogo;
    int max_usage = std::min(spend * 6, (remaining - move_overhead) / 4);
    return {spend, max_usage};
}

TTime no_movestogo(int increment, int remaining) {
    int min_to_go = increment == 0 ? 10 : 3;
    int move_num = (search_threads[0].root_ply + 1) / 2;
    int movestogo = std::max(10 + 4 * (50 - move_num) / 5 , min_to_go);
    int average_time = remaining / movestogo;
    int extra = average_time * std::max(30 - move_num, 0) / 200;
    int spend = average_time + extra + increment;
    int max_usage = std::min(spend * 6, (remaining - move_overhead) / 4);

    if (max_usage < increment) {
        max_usage = remaining - 2 * move_overhead;
    }

    return {spend, max_usage};
}

TTime get_myremain(int increment, int remaining, int movestogo){
    if (movestogo == 1) {
        return TTime{remaining - move_overhead, remaining - move_overhead};
    } else if (movestogo == 0) {
        return no_movestogo(increment, remaining);
    } else {
        return moves_in_time(increment, remaining, movestogo);
    }
}
