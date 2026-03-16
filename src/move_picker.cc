#include "move_picker.h"

#include <algorithm>

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/types.h"
#include "search.h"

namespace punch {

template <movegen::MoveGenType T>
MovePicker<T>::MovePicker(const ChessBoard& board, SearchStack* ss,
                          Move tt_move) {
  movegen::GenerateLegalMoves<T>(board, moves);
  for (size_t i = 0; i < moves.size(); ++i) {
    scores[i] = ScoreMove(board, ss, moves[i], tt_move);
  }
}

template <movegen::MoveGenType T>
Move MovePicker<T>::NextMove() {
  if (current >= moves.size()) {
    return Move::None();
  }

  int best_idx = std::max_element(scores.begin() + current,
                                  scores.begin() + moves.size()) -
                 scores.begin();
  std::swap(moves[current], moves[best_idx]);
  std::swap(scores[current], scores[best_idx]);

  return moves[current++];
}

template <movegen::MoveGenType T>
int MovePicker<T>::ScoreMove(const ChessBoard& board, SearchStack* ss, Move m,
                             Move tt_move) const {
  // kNoPieceType, kPawn, kKnight, kBishop, kRook, kQueen, kKing,
  static constexpr int kPieceValues[] = {0, 100, 290, 310, 500, 900, 0};

  int score = 0;

  if (m == tt_move) {
    score += 100000000;
  }

  if (m.TypeOf() == MoveType::kPromotion) {
    switch (m.PromotionType()) {
      case PieceType::kQueen:
        score += 90000001;
        break;
      case PieceType::kKnight:
        score += 90000000;
        break;
      case PieceType::kBishop:
        score -= 90000000;
        break;
      case PieceType::kRook:
        score -= 90000001;
        break;
      default:
        __builtin_unreachable();
    }
  }

  if (board.IsCapture(m)) {
    Piece attacker = board.PieceOn(m.FromSquare());
    Piece victim = board.PieceOn(m.ToSquare());
    score += 80000000 + (kPieceValues[TypeOf(victim)] * 10) -
             kPieceValues[TypeOf(attacker)];
  }

  if (m == ss->killers[0]) {
    score += 70000001;
  } else if (m == ss->killers[1]) {
    score += 70000000;
  }

  return score;
}

template class MovePicker<movegen::MoveGenType::kAll>;
template class MovePicker<movegen::MoveGenType::kCaptures>;
template class MovePicker<movegen::MoveGenType::kQuiets>;

}  // namespace punch
