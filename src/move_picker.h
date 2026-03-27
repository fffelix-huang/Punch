#ifndef PUNCH_MOVE_PICKER_H_
#define PUNCH_MOVE_PICKER_H_

#include <array>

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/types.h"

namespace punch {

bool StaticExchangeEvaluation(const ChessBoard& board, Move m, Value threshold);

struct SearchStack;
struct SearchTable;

template <movegen::MoveGenType T>
class MovePicker {
 public:
  MovePicker(const ChessBoard& board, SearchStack* ss, Move tt_move,
             const SearchTable& tables);

  Move NextMove();

  inline int ScoreMove(const ChessBoard& board, SearchStack* ss, Move m,
                       Move tt_move, const SearchTable& tables) const;

  inline size_t NumMoves() const { return moves.size(); }

 private:
  movegen::MoveList moves;
  std::array<int, movegen::kMaxMoves> scores;
  size_t current = 0;
};

}  // namespace punch

#endif  // PUNCH_MOVE_PICKER_H_
