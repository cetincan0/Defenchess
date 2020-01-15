# Defenchess
### Overview

Defenchess is a free, open source UCI chess engine written in C++. This project has been created purely as a hobby and a challenge for ourselves to create a strong chess engine. It is meant to be used with a UCI compatible chess GUI.

### Parameters
* #### Hash
  The size of the transposition table in megabytes. (default 16)

* #### Threads
  The number of threads used while searching. (default 1)

* #### SyzygyPath
  The path to the Syzygy Tablebase WDL/DTZ files. (default empty)

* #### MoveOverhead
  The minimum amount of time in milliseconds to be left on the clock while moving. Should be increased if the engine is losing on time. (default 100)

* #### Ponder
  This parameter is just to tell GUIs that the engine is capable of pondering. The engine will respond to the "go ponder" command regardless of it being set to true or false.

* #### UCI\_Chess960
  This parameter will let the engine play Chess960 (FRC) when set to true.


### Special thanks
- Donna and the Chess Programming Wiki for the inspiration and helping us understand the basics of chess engines
- Stockfish and many other open source chess engines for many of the ideas/optimizations used in our engine
