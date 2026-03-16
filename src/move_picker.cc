#include "move_picker.h"

#include <algorithm>

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/types.h"

namespace punch {

template <movegen::MoveGenType T>
MovePicker<T>::MovePicker(const ChessBoard& board, Move tt_move) {
  movegen::GenerateLegalMoves<T>(board, moves);
  for (size_t i = 0; i < moves.size(); ++i) {
    scores[i] = ScoreMove(board, moves[i], tt_move);
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
int MovePicker<T>::ScoreMove(const ChessBoard& board, Move m,
                             Move tt_move) const {
  // kNoPieceType, kPawn, kKnight, kBishop, kRook, kQueen, kKing,
  static constexpr int kPieceValues[] = {0, 100, 290, 310, 500, 900, 0};

  int score = 0;

  if (m == tt_move) {
    score += 200000000;
  }

  if (m.TypeOf() == MoveType::kPromotion) {
    switch (m.PromotionType()) {
      case PieceType::kQueen:
        score += 100000001;
        break;
      case PieceType::kKnight:
        score += 100000000;
        break;
      case PieceType::kBishop:
        score -= 100000000;
        break;
      case PieceType::kRook:
        score -= 100000001;
        break;
      default:
        __builtin_unreachable();
    }
  }

  if (board.IsCapture(m)) {
    Piece attacker = board.PieceOn(m.FromSquare());
    Piece victim = board.PieceOn(m.ToSquare());
    score += 1000000 + (kPieceValues[TypeOf(victim)] * 10) -
             kPieceValues[TypeOf(attacker)];
  }

  return score;
}

template class MovePicker<movegen::MoveGenType::kAll>;
template class MovePicker<movegen::MoveGenType::kCaptures>;
template class MovePicker<movegen::MoveGenType::kQuiets>;

}  // namespace punch
