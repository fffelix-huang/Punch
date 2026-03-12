#ifndef PUNCH_MOVEPICKER_H_
#define PUNCH_MOVEPICKER_H_

#include <array>

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/types.h"

namespace punch {

template <movegen::MoveGenType T>
class MovePicker {
 public:
  MovePicker(const ChessBoard& board, Move tt_move);

  Move NextMove();

  inline int ScoreMove(const ChessBoard& board, Move m, Move tt_move) const;

  inline size_t NumMoves() const { return moves.size(); }

 private:
  movegen::MoveList moves;
  std::array<int, movegen::kMaxMoves> scores;
  size_t current = 0;
};

}  // namespace punch

#endif  // PUNCH_MOVEPICKER_H_
