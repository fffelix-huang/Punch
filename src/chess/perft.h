#ifndef PUNCH_CHESS_PERFT_H_
#define PUNCH_CHESS_PERFT_H_

#include "board.h"

namespace punch {

uint64_t Perft(ChessBoard& board, int depth);

}  // namespace punch

#endif  // PUNCH_CHESS_PERFT_H_
