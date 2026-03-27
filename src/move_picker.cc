#include "move_picker.h"

#include <algorithm>

#include "chess/attacks.h"
#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/types.h"
#include "search.h"

namespace punch {

bool StaticExchangeEvaluation(const ChessBoard& board, Move m,
                              Value threshold) {
  assert(board.IsCapture(m));

  Color us = board.SideToMove();
  MoveType mt = m.TypeOf();
  Square from = m.FromSquare();
  Square to = m.ToSquare();
  Square captured_sq =
      (mt != MoveType::kEnPassant ? to : to - PawnDirection(~us));

  Value gain = kPieceValue[TypeOf(board.PieceOn(captured_sq))] - threshold;
  if (gain < 0) {
    return false;
  }

  gain -= kPieceValue[TypeOf(board.PieceOn(from))];
  if (gain >= 0) {
    return true;
  }

  Bitboard occ = board.Occupied() ^ SquareToBitboard(from);
  if (mt == MoveType::kEnPassant) {
    occ ^= SquareToBitboard(captured_sq);
  }

  Bitboard attackers = board.AttackersTo(to, occ);

  while (true) {
    us = ~us;
    attackers &= occ;
    Bitboard our_attackers = attackers & board.Us(us);

    if (our_attackers == 0) {
      return us != board.SideToMove();
    }

    PieceType attacker_type = PieceType::kNoPieceType;

    // Least Valuable Attacker
    for (PieceType pt :
         {PieceType::kPawn, PieceType::kKnight, PieceType::kBishop,
          PieceType::kRook, PieceType::kQueen, PieceType::kKing}) {
      Bitboard lva_bb = our_attackers & board.Pieces(pt);
      if (lva_bb) {
        Square attacker_sq = PopLsb(lva_bb);
        occ ^= SquareToBitboard(attacker_sq);
        attacker_type = pt;
        break;
      }
    }

    // Diagonal
    if (attacker_type == PieceType::kPawn ||
        attacker_type == PieceType::kBishop ||
        attacker_type == PieceType::kQueen) {
      attackers |=
          attacks::GetBishopAttacks(to, occ) &
          (board.Pieces(PieceType::kBishop) | board.Pieces(PieceType::kQueen));
    }
    // Horizontal & Vertical
    if (attacker_type == PieceType::kRook ||
        attacker_type == PieceType::kQueen) {
      attackers |=
          attacks::GetRookAttacks(to, occ) &
          (board.Pieces(PieceType::kRook) | board.Pieces(PieceType::kQueen));
    }

    gain = -gain - kPieceValue[attacker_type] - 1;
    if (gain >= 0) {
      return us == board.SideToMove();
    }
  }

  __builtin_unreachable();
}

template <movegen::MoveGenType T>
MovePicker<T>::MovePicker(const ChessBoard& board, SearchStack* ss,
                          Move tt_move, const SearchTable& tables) {
  movegen::GenerateLegalMoves<T>(board, moves);
  for (size_t i = 0; i < moves.size(); ++i) {
    scores[i] = ScoreMove(board, ss, moves[i], tt_move, tables);
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
                             Move tt_move, const SearchTable& tables) const {
  const Color us = board.SideToMove();

  // 1. TT Move
  if (m == tt_move) {
    return 100000000;
  }

  // 2. Promotions
  if (m.TypeOf() == MoveType::kPromotion) {
    switch (m.PromotionType()) {
      case PieceType::kQueen:
        return 90000001;
      case PieceType::kKnight:
        return 90000000;
      case PieceType::kBishop:
        return -90000000;
      case PieceType::kRook:
        return -90000001;
      default:
        __builtin_unreachable();
    }
  }

  // 3. Captures
  if (board.IsCapture(m)) {
    Piece attacker = board.PieceOn(m.FromSquare());
    Piece victim =
        (m.TypeOf() != MoveType::kEnPassant ? board.PieceOn(m.ToSquare())
                                            : MakePiece(~us, PieceType::kPawn));
    Value mvv_lva =
        (kPieceValue[TypeOf(victim)] * 10) - kPieceValue[TypeOf(attacker)];

    if (StaticExchangeEvaluation(board, m, -80)) {
      return 80000000 + mvv_lva;
    } else {
      return 30000000 + mvv_lva;
    }
  }

  // 4. Killer Moves
  if (m == ss->killers[0]) {
    return 70000001;
  } else if (m == ss->killers[1]) {
    return 70000000;
  }

  // 5. Quiet History Moves
  return tables.move_history[us][m.FromSquare()][m.ToSquare()];
}

template class MovePicker<movegen::MoveGenType::kAll>;
template class MovePicker<movegen::MoveGenType::kCaptures>;
template class MovePicker<movegen::MoveGenType::kQuiets>;

}  // namespace punch
