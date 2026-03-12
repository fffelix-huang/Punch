#include "chess/perft.h"

#include "chess/board.h"
#include "chess/movegen.h"

namespace punch {

uint64_t Perft(ChessBoard& board, int depth) {
  if (depth == 0) {
    return 1;
  }

  movegen::MoveList movelist;
  movegen::GenerateLegalMoves(board, movelist);

  if (depth == 1) {
    return movelist.size();
  }

  uint64_t nodes = 0;

  for (Move m : movelist) {
    StateInfo st;
    board.MakeMove(m, st);
    nodes += Perft(board, depth - 1);
    board.UnmakeMove(m);
  }

  return nodes;
}

}  // namespace punch
