#include "evaluate.h"

#include <array>

#include "chess/bitboard.h"
#include "chess/board.h"
#include "chess/types.h"

namespace punch::eval {

namespace {

constexpr Value kPieceValue[] = {Value(0),   Value(100), Value(290), Value(310),
                                 Value(500), Value(900), Value(0)};

constexpr std::array<std::array<Bitboard, Square::kSquareNb>, Color::kColorNb>
    kPassedPawnMask = [] {
      std::array<std::array<Bitboard, Square::kSquareNb>, Color::kColorNb>
          masks{};

      auto passed_pawn_mask = [](Square sq, Color c) -> Bitboard {
        File f = FileOf(sq);
        Rank r = RankOf(sq);

        Bitboard front = 0ULL;
        if (c == Color::kWhite) {
          if (r < Rank::kRank8) {
            front = (~0ULL) << (Direction::kNorth * (r + 1));
          }
        } else {
          if (r > Rank::kRank1) {
            front = (~0ULL) >> (Direction::kNorth * (8 - r));
          }
        }

        Bitboard adjacent = GetFileBitboard(f);
        if (f != File::kFileA) {
          adjacent |= GetFileBitboard(static_cast<File>(f - 1));
        }
        if (f != File::kFileH) {
          adjacent |= GetFileBitboard(static_cast<File>(f + 1));
        }

        return front & adjacent;
      };

      for (Color c : {Color::kWhite, Color::kBlack}) {
        for (int s = 0; s < Square::kSquareNb; ++s) {
          Square sq = static_cast<Square>(s);
          masks[c][s] = passed_pawn_mask(sq, c);
        }
      }

      return masks;
    }();

constexpr Value kPassedPawnBonus[] = {Value(0),   Value(0),  Value(30),
                                      Value(30),  Value(45), Value(90),
                                      Value(200), Value(0)};
}  // namespace

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

  // Passed Pawn Bonus
  for (Color c : {us, ~us}) {
    Bitboard us_pawns = board.Pieces(PieceType::kPawn, c);
    Bitboard opp_pawns = board.Pieces(PieceType::kPawn, ~c);

    while (us_pawns) {
      Square sq = PopLsb(us_pawns);
      Bitboard mask = kPassedPawnMask[c][sq];
      if ((opp_pawns & mask) == 0) {
        Rank r = RankOf(sq);
        int relative_rank = (c == Color::kWhite ? r : 7 - r);
        score += coeff * kPassedPawnBonus[relative_rank];
      }
    }

    coeff *= -1;
  }

  return score;
}

}  // namespace punch::eval
