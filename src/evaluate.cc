#include "evaluate.h"

#include "chess/bitboard.h"
#include "chess/board.h"
#include "chess/types.h"

namespace punch::eval {

constexpr Value kPieceValue[] = {Value(0),   Value(100), Value(290), Value(310),
                                 Value(500), Value(900), Value(0)};

Value Evaluate(const ChessBoard& board) {
  Color us = board.SideToMove();
  Value score = 0;
  int coeff = +1;

  for (Color c : {us, ~us}) {
    for (PieceType pt :
         {PieceType::kPawn, PieceType::kKnight, PieceType::kBishop,
          PieceType::kRook, PieceType::kQueen}) {
      int piece_count = PopCount(board.Pieces(pt, c));
      score += coeff * piece_count * kPieceValue[pt];
    }
    coeff *= -1;
  }

  return score;
}

}  // namespace punch::eval
