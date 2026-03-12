#include "chess/movegen.h"

#include <array>
#include <bit>

#include "chess/attacks.h"
#include "chess/bitboard.h"
#include "chess/types.h"

namespace punch::movegen {

namespace {

/**
 * @brief Precomputed table of squares between any two squares on a shared ray.
 */
std::array<std::array<Bitboard, 64>, 64> squares_between_bb_ = {};

/**
 * @brief Returns a bitboard of squares between sq1 and sq2 (exclusive).
 */
inline Bitboard Between(Square sq1, Square sq2) {
  return squares_between_bb_[sq1][sq2];
}

/**
 * @brief Computes the checkmask and number of checkers.
 * The checkmask identifies squares where a move can block or capture the
 * checker.
 */
template <Color C>
std::pair<Bitboard, int> CheckMask(const ChessBoard& board, Square king_sq) {
  Bitboard checkers = board.Checkers();
  int num_checkers = PopCount(checkers);

  if (num_checkers == 0) {
    return {~0ULL, 0};
  }
  if (num_checkers > 1) {
    return {0ULL, num_checkers};
  }

  assert(num_checkers == 1);
  PUNCH_ASSUME(num_checkers == 1);

  Square checker_sq = static_cast<Square>(std::countr_zero(checkers));
  PieceType pt = TypeOf(board.PieceOn(checker_sq));

  // If the checker is a slider, the mask includes the checker and the path to
  // the king.
  if (pt == PieceType::kBishop || pt == PieceType::kRook ||
      pt == PieceType::kQueen) {
    return {Between(king_sq, checker_sq) | checkers, 1};
  }

  // If the checker is a pawn or knight, the mask only includes the checker's
  // square.
  return {checkers, 1};
}

/**
 * @brief Computes all squares controlled/attacked by the given side.
 * * Crucially, this is used to determine where the OPPOSITE king cannot move.
 * It accounts for X-ray attacks through the opponent's king by temporarily
 * removing it from the occupancy bitboard.
 */
template <Color C>
Bitboard SeenSquares(const ChessBoard& board) {
  Color us = C;
  Bitboard seen = 0ULL;
  Bitboard pawns = board.Pieces(PieceType::kPawn, us);

  // Pawn attacks
  if constexpr (C == Color::kWhite) {
    seen |= (pawns << Direction::kNorthWest) & ~GetFileBitboard(File::kFileH);
    seen |= (pawns << Direction::kNorthEast) & ~GetFileBitboard(File::kFileA);
  } else {
    seen |= (pawns >> Direction::kNorthWest) & ~GetFileBitboard(File::kFileA);
    seen |= (pawns >> Direction::kNorthEast) & ~GetFileBitboard(File::kFileH);
  }

  // Knight attacks
  Bitboard knights = board.Pieces(PieceType::kKnight, us);
  while (knights) {
    seen |= attacks::GetKnightAttacks(PopLsb(knights));
  }

  // Slider attacks:
  // We treat 'them' king as transparent.
  // This ensures that if the king moves backward along a sliding ray,
  // the square it just vacated is correctly marked as 'seen' (X-ray).
  Bitboard them_king_bb = board.Pieces(PieceType::kKing, ~C);
  Bitboard occ = board.Occupied() ^ them_king_bb;

  Bitboard bishops = board.Pieces(PieceType::kBishop, us) |
                     board.Pieces(PieceType::kQueen, us);
  while (bishops) {
    seen |= attacks::GetBishopAttacks(PopLsb(bishops), occ);
  }

  Bitboard rooks =
      board.Pieces(PieceType::kRook, us) | board.Pieces(PieceType::kQueen, us);
  while (rooks) {
    seen |= attacks::GetRookAttacks(PopLsb(rooks), occ);
  }

  // King attacks:
  // Our king also controls squares, preventing the enemy king from moving
  // adjacent.
  Square ksq = board.KingSquare(us);
  if (ksq != Square::kNone) {
    seen |= attacks::GetKingAttacks(ksq);
  }

  return seen;
}

/**
 * @brief Generates legal En Passant moves.
 * @details Special handling for the rare "horizontal revealed check" where
 * removing two pawns from a rank unblocks an attack on the king.
 */
template <Color C>
void GenerateEpMoves(const ChessBoard& board, MoveList& moves,
                     Bitboard checkmask, Bitboard pinned_d, Bitboard pinned_hv,
                     const std::array<Bitboard, 64>& pin_rays,
                     Bitboard pawns_lr, Square ep) {
  Square captured_sq = ep - PawnDirection(C);
  Bitboard captured_bb = SquareToBitboard(captured_sq);

  // The move is valid only if either the EP square or the captured pawn is in
  // the checkmask.
  if (!(checkmask & (captured_bb | SquareToBitboard(ep)))) {
    return;
  }

  Bitboard candidates = attacks::GetPawnAttacks(ep, ~C) & pawns_lr;
  Square king_sq = board.KingSquare(C);

  while (candidates) {
    Square from = PopLsb(candidates);
    // Pawn must stay on diagonal pin ray if pinned.
    if ((SquareToBitboard(from) & pinned_d) &&
        !(SquareToBitboard(ep) & pin_rays[from])) {
      continue;
    }
    // Pawn cannot move if vertically pinned (EP moves horizontally).
    if (SquareToBitboard(from) & pinned_hv) {
      continue;
    }

    // Simulate move to check for slider revealed checks.
    // Example: 7k/4p3/8/2KP3r/8/8/8/8 b - - 0 1
    Bitboard occ = (board.Occupied() ^ SquareToBitboard(from) ^ captured_bb) |
                   SquareToBitboard(ep);
    Bitboard rook_atk = attacks::GetRookAttacks(king_sq, occ);
    Bitboard bishop_atk = attacks::GetBishopAttacks(king_sq, occ);
    Bitboard sliders = (board.Pieces(PieceType::kRook, ~C) |
                        board.Pieces(PieceType::kQueen, ~C)) &
                       rook_atk;
    sliders |= (board.Pieces(PieceType::kBishop, ~C) |
                board.Pieces(PieceType::kQueen, ~C)) &
               bishop_atk;

    if (!sliders) {
      moves.push_back(Move::Make<MoveType::kEnPassant>(from, ep));
    }
  }
}

/**
 * @brief Generates legal pawn pushes and captures.
 */
template <Color C, MoveGenType Mt>
void GeneratePawnMoves(const ChessBoard& board, MoveList& moves,
                       Bitboard pinned_d, Bitboard pinned_hv,
                       const std::array<Bitboard, 64>& pin_rays,
                       Bitboard checkmask, Bitboard occ_opp) {
  constexpr Direction UP = PawnDirection(C);

  Bitboard pawns = board.Pieces(PieceType::kPawn, C);
  Bitboard occ_all = board.Occupied();

  // Pushes
  if constexpr (Mt != MoveGenType::kCaptures) {
    // Pawns pinned diagonally cannot push.
    Bitboard pawns_pushable = pawns & ~pinned_d;

    Bitboard single =
        (C == Color::kWhite ? (pawns_pushable << Direction::kNorth)
                            : (pawns_pushable >> Direction::kNorth)) &
        ~occ_all;
    Bitboard double_push =
        (C == Color::kWhite
             ? ((single & GetRankBitboard(Rank::kRank3)) << Direction::kNorth)
             : ((single & GetRankBitboard(Rank::kRank6)) >>
                Direction::kNorth)) &
        ~occ_all;

    Bitboard s = single & checkmask;
    while (s) {
      Square to = PopLsb(s);
      Square from = to - UP;
      // If vertically pinned, pawn must move along the pin ray.
      if ((SquareToBitboard(from) & pinned_hv) &&
          !(SquareToBitboard(to) & pin_rays[from])) {
        continue;
      }

      if (RankOf(to) == (C == Color::kWhite ? Rank::kRank8 : Rank::kRank1)) {
        moves.push_back(
            Move::Make<MoveType::kPromotion>(from, to, PieceType::kQueen));
        moves.push_back(
            Move::Make<MoveType::kPromotion>(from, to, PieceType::kRook));
        moves.push_back(
            Move::Make<MoveType::kPromotion>(from, to, PieceType::kBishop));
        moves.push_back(
            Move::Make<MoveType::kPromotion>(from, to, PieceType::kKnight));
      } else {
        moves.push_back(Move(from, to));
      }
    }

    Bitboard d = double_push & checkmask;
    while (d) {
      Square to = PopLsb(d);
      Square from = to - UP - UP;
      // Check for vertical pin on double push.
      if ((SquareToBitboard(from) & pinned_hv) &&
          !(SquareToBitboard(to) & pin_rays[from])) {
        continue;
      }
      moves.push_back(Move(from, to));
    }
  }

  // Captures
  if constexpr (Mt != MoveGenType::kQuiets) {
    // Pawns pinned horizontally/vertically cannot capture diagonally.
    Bitboard pawns_lr = pawns & ~pinned_hv;
    for (Direction D :
         {(C == Color::kWhite ? Direction::kNorthWest : Direction::kSouthWest),
          (C == Color::kWhite ? Direction::kNorthEast
                              : Direction::kSouthEast)}) {
      int shift = std::abs(static_cast<int>(D));
      Bitboard targets =
          (C == Color::kWhite ? (pawns_lr << shift) : (pawns_lr >> shift));

      // Filter out file wrapping.
      if (D == Direction::kNorthWest || D == Direction::kSouthWest) {
        targets &= ~GetFileBitboard(File::kFileH);
      } else {
        targets &= ~GetFileBitboard(File::kFileA);
      }

      targets &= occ_opp & checkmask;

      while (targets) {
        Square to = PopLsb(targets);
        Square from = to - D;
        // Pinned pawn must capture along its diagonal pin ray.
        if ((SquareToBitboard(from) & pinned_d) &&
            !(SquareToBitboard(to) & pin_rays[from])) {
          continue;
        }

        if (RankOf(to) == (C == Color::kWhite ? Rank::kRank8 : Rank::kRank1)) {
          moves.push_back(
              Move::Make<MoveType::kPromotion>(from, to, PieceType::kQueen));
          moves.push_back(
              Move::Make<MoveType::kPromotion>(from, to, PieceType::kRook));
          moves.push_back(
              Move::Make<MoveType::kPromotion>(from, to, PieceType::kBishop));
          moves.push_back(
              Move::Make<MoveType::kPromotion>(from, to, PieceType::kKnight));
        } else {
          moves.push_back(Move(from, to));
        }
      }
    }

    Square ep = board.EpSquare();
    if (ep != Square::kNone) {
      GenerateEpMoves<C>(board, moves, checkmask, pinned_d, pinned_hv, pin_rays,
                         pawns_lr, ep);
    }
  }
}

/**
 * @brief Generates legal castling moves.
 */
template <Color C, MoveGenType Mt>
void GenerateCastleMoves(const ChessBoard& board, Bitboard seen,
                         MoveList& moves) {
  CastlingRights cr = board.Castling();
  Bitboard occ = board.Occupied();

  auto can_castle = [&](CastlingRights right, Bitboard path,
                        Bitboard king_path) {
    if (!(cr & right)) {
      return false;
    }
    if (occ & path) {
      return false;
    }
    if (seen & king_path) {
      return false;
    }
    return true;
  };

  if constexpr (C == Color::kWhite) {
    if (can_castle(kWhiteOO, SquaresToBitboard(Square::kF1, Square::kG1),
                   SquaresToBitboard(Square::kF1, Square::kG1))) {
      moves.push_back(
          Move::Make<MoveType::kCastling>(Square::kE1, Square::kG1));
    }
    if (can_castle(kWhiteOOO,
                   SquaresToBitboard(Square::kD1, Square::kC1, Square::kB1),
                   SquaresToBitboard(Square::kD1, Square::kC1))) {
      moves.push_back(
          Move::Make<MoveType::kCastling>(Square::kE1, Square::kC1));
    }
  } else {
    if (can_castle(kBlackOO, SquaresToBitboard(Square::kF8, Square::kG8),
                   SquaresToBitboard(Square::kF8, Square::kG8))) {
      moves.push_back(
          Move::Make<MoveType::kCastling>(Square::kE8, Square::kG8));
    }
    if (can_castle(kBlackOOO,
                   SquaresToBitboard(Square::kD8, Square::kC8, Square::kB8),
                   SquaresToBitboard(Square::kD8, Square::kC8))) {
      moves.push_back(
          Move::Make<MoveType::kCastling>(Square::kE8, Square::kC8));
    }
  }
}

/**
 * @brief Core legal move generation logic for a single side.
 */
template <Color C, MoveGenType Mt>
void GenerateLegalMovesInternal(const ChessBoard& board, MoveList& movelist,
                                PieceGenType pieces) {
  Square king_sq = board.KingSquare(C);
  if (king_sq == Square::kNone) return;

  Bitboard occ_us = board.Us(C);
  Bitboard occ_opp = board.Us(~C);
  Bitboard occ_all = board.Occupied();

  // 1. Precompute checkmask and absolute pins
  auto [checkmask, checks] = CheckMask<C>(board, king_sq);

  Bitboard pinned_hv = 0ULL, pinned_d = 0ULL;
  std::array<Bitboard, 64> pin_rays = {};

  auto find_pins = [&](Bitboard pinners, Bitboard& pinned_mask) {
    while (pinners) {
      Square pinner_sq = PopLsb(pinners);
      Bitboard path = Between(king_sq, pinner_sq);
      Bitboard pinned = path & occ_us;
      if (PopCount(path & occ_all) == 1 && pinned) {
        Square pinned_sq = static_cast<Square>(std::countr_zero(pinned));
        pinned_mask |= pinned;
        pin_rays[pinned_sq] = path | SquareToBitboard(pinner_sq);
      }
    }
  };

  Bitboard pinner_hv = attacks::GetRookAttacks(king_sq, occ_opp) &
                       (board.Pieces(PieceType::kRook, ~C) |
                        board.Pieces(PieceType::kQueen, ~C));
  Bitboard pinner_d = attacks::GetBishopAttacks(king_sq, occ_opp) &
                      (board.Pieces(PieceType::kBishop, ~C) |
                       board.Pieces(PieceType::kQueen, ~C));
  find_pins(pinner_hv, pinned_hv);
  find_pins(pinner_d, pinned_d);

  Bitboard movable = (Mt == MoveGenType::kCaptures) ? occ_opp
                     : (Mt == MoveGenType::kQuiets) ? ~occ_all
                                                    : ~occ_us;

  // 2. Generate King moves
  if (pieces & kGenKing) {
    Bitboard seen = SeenSquares<~C>(board);
    Bitboard king_moves = attacks::GetKingAttacks(king_sq) & movable & ~seen;
    while (king_moves) {
      movelist.push_back(Move(king_sq, PopLsb(king_moves)));
    }

    if (Mt != MoveGenType::kCaptures && checks == 0) {
      GenerateCastleMoves<C, Mt>(board, seen, movelist);
    }
  }

  // In double check, only the king can move.
  if (checks > 1) {
    return;
  }

  // Restrict all remaining moves to the checkmask.
  movable &= checkmask;

  // 3. Generate Pawn moves
  if (pieces & kGenPawn)
    GeneratePawnMoves<C, Mt>(board, movelist, pinned_d, pinned_hv, pin_rays,
                             checkmask, occ_opp);

  // 4. Generate Knight moves
  if (pieces & kGenKnight) {
    // Pinned knights can never move.
    Bitboard knights =
        board.Pieces(PieceType::kKnight, C) & ~(pinned_hv | pinned_d);
    while (knights) {
      Square from = PopLsb(knights);
      Bitboard targets = attacks::GetKnightAttacks(from) & movable;
      while (targets) {
        movelist.push_back(Move(from, PopLsb(targets)));
      }
    }
  }

  // 5. Generate Slider moves (Bishops, Rooks, Queens)
  auto gen_sliders = [&](PieceType pt, Bitboard aligned_pin_mask,
                         Bitboard disaligned_pin_mask) {
    // Pieces pinned by the 'disaligned' type cannot move.
    Bitboard sliders = board.Pieces(pt, C) & ~disaligned_pin_mask;
    while (sliders) {
      Square from = PopLsb(sliders);
      Bitboard targets =
          (pt == PieceType::kBishop ? attacks::GetBishopAttacks(from, occ_all)
                                    : attacks::GetRookAttacks(from, occ_all));
      targets &= movable;
      // If pinned, must move along the pin ray.
      if (SquareToBitboard(from) & aligned_pin_mask) {
        targets &= pin_rays[from];
      }
      while (targets) {
        movelist.push_back(Move(from, PopLsb(targets)));
      }
    }
  };

  if (pieces & kGenBishop) {
    gen_sliders(PieceType::kBishop, pinned_d, pinned_hv);
  }
  if (pieces & kGenRook) {
    gen_sliders(PieceType::kRook, pinned_hv, pinned_d);
  }
  if (pieces & kGenQueen) {
    Bitboard queens = board.Pieces(PieceType::kQueen, C);
    while (queens) {
      Square from = PopLsb(queens);
      Bitboard targets = attacks::GetQueenAttacks(from, occ_all) & movable;
      // Queens check both pin types
      if (SquareToBitboard(from) & (pinned_hv | pinned_d)) {
        targets &= pin_rays[from];
      }
      while (targets) {
        movelist.push_back(Move(from, PopLsb(targets)));
      }
    }
  }
}

}  // namespace

void Initialize() {
  for (int sq1 = 0; sq1 < 64; ++sq1) {
    for (int sq2 = 0; sq2 < 64; ++sq2) {
      Square s1 = static_cast<Square>(sq1);
      Square s2 = static_cast<Square>(sq2);
      Bitboard b1 = SquareToBitboard(s1);
      Bitboard b2 = SquareToBitboard(s2);
      // Precompute squares between if they align diagonally or orthogonally.
      if (attacks::GetBishopAttacks(s1, 0ULL) & b2) {
        squares_between_bb_[sq1][sq2] = attacks::GetBishopAttacks(s1, b2) &
                                        attacks::GetBishopAttacks(s2, b1);
      } else if (attacks::GetRookAttacks(s1, 0ULL) & b2) {
        squares_between_bb_[sq1][sq2] =
            attacks::GetRookAttacks(s1, b2) & attacks::GetRookAttacks(s2, b1);
      } else {
        squares_between_bb_[sq1][sq2] = 0ULL;
      }
    }
  }
}

template <MoveGenType T>
void GenerateLegalMoves(const ChessBoard& board, MoveList& movelist,
                        PieceGenType pieces) {
  if (board.SideToMove() == Color::kWhite)
    GenerateLegalMovesInternal<Color::kWhite, T>(board, movelist, pieces);
  else
    GenerateLegalMovesInternal<Color::kBlack, T>(board, movelist, pieces);
}

// Explicit template instantiations for supported move generation types.
template void GenerateLegalMoves<MoveGenType::kAll>(const ChessBoard&,
                                                    MoveList&, PieceGenType);
template void GenerateLegalMoves<MoveGenType::kCaptures>(const ChessBoard&,
                                                         MoveList&,
                                                         PieceGenType);
template void GenerateLegalMoves<MoveGenType::kQuiets>(const ChessBoard&,
                                                       MoveList&, PieceGenType);

}  // namespace punch::movegen
