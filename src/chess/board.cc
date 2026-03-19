#include "chess/board.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "chess/attacks.h"
#include "chess/bitboard.h"
#include "chess/types.h"
#include "chess/zobrist.h"

namespace punch {

void ChessBoard::LoadFen(std::string_view fen) {
  // Reset board
  std::ranges::fill(board_, Piece::kNoPiece);
  std::ranges::fill(by_type_, 0ULL);
  std::ranges::fill(by_color_, 0ULL);
  zobrist_key_ = 0;
  st_ = nullptr;

  std::istringstream iss{std::string(fen)};
  std::string pieces, side, castling, ep, halfmove, fullmove;
  iss >> pieces >> side >> castling >> ep >> halfmove >> fullmove;

  // 1. Pieces
  Square sq = Square::kA8;
  for (char c : pieces) {
    if (c == '/') {
      sq += Direction::kSouth * 2;
    } else if (std::isdigit(c)) {
      sq += Direction::kEast * (c - '0');
    } else {
      Piece pc = Piece::kNoPiece;
      // clang-format off
      switch (c) {
        case 'P': pc = Piece::kWhitePawn; break;
        case 'N': pc = Piece::kWhiteKnight; break;
        case 'B': pc = Piece::kWhiteBishop; break;
        case 'R': pc = Piece::kWhiteRook; break;
        case 'Q': pc = Piece::kWhiteQueen; break;
        case 'K': pc = Piece::kWhiteKing; break;
        case 'p': pc = Piece::kBlackPawn; break;
        case 'n': pc = Piece::kBlackKnight; break;
        case 'b': pc = Piece::kBlackBishop; break;
        case 'r': pc = Piece::kBlackRook; break;
        case 'q': pc = Piece::kBlackQueen; break;
        case 'k': pc = Piece::kBlackKing; break;
        default: break;
      }
      // clang-format on
      if (pc != Piece::kNoPiece) {
        PutPiece(sq, pc);
      }
      sq += Direction::kEast;
    }
  }

  // 2. Side to move
  side_to_move_ = (side == "w" ? Color::kWhite : Color::kBlack);
  zobrist_key_ ^= zobrist::ColorKey(side_to_move_);

  // 3. Castling rights
  castling_rights_ = CastlingRights::kNoCastling;
  if (castling != "-") {
    for (char c : castling) {
      // clang-format off
      switch (c) {
        case 'K': SetCastlingRights(castling_rights_, CastlingRights::kWhiteOO); break;
        case 'Q': SetCastlingRights(castling_rights_, CastlingRights::kWhiteOOO); break;
        case 'k': SetCastlingRights(castling_rights_, CastlingRights::kBlackOO); break;
        case 'q': SetCastlingRights(castling_rights_, CastlingRights::kBlackOOO); break;
      }
      // clang-format on
    }
  }
  zobrist_key_ ^= zobrist::CastlingKey(castling_rights_);

  // 4. En passant square
  ep_square_ = Square::kNone;
  if (ep != "-") {
    File f = static_cast<File>(ep[0] - 'a');
    Rank r = static_cast<Rank>(ep[1] - '1');
    ep_square_ = MakeSquare(f, r);
    zobrist_key_ ^= zobrist::EnPassantKey(f);
  }

  // 5. Halfmove clock
  rule50_ = (halfmove.empty() ? 0 : std::stoi(halfmove));

  // 6. Fullmove number -> game_ply_
  int full_move = (fullmove.empty() ? 1 : std::stoi(fullmove));
  game_ply_ = (full_move - 1) * 2 + (side_to_move_ == Color::kBlack ? 1 : 0);
}

std::string ChessBoard::GetFen() const {
  std::string fen;
  for (int r = 7; r >= 0; r--) {
    int empty = 0;
    for (int f = 0; f < 8; f++) {
      Square sq = MakeSquare(static_cast<File>(f), static_cast<Rank>(r));
      Piece pc = PieceOn(sq);
      if (pc == Piece::kNoPiece) {
        empty++;
      } else {
        if (empty > 0) {
          fen += std::to_string(empty);
          empty = 0;
        }
        char c = ' ';
        PieceType pt = TypeOf(pc);
        Color color = ColorOf(pc);
        // clang-format off
        switch (pt) {
          case PieceType::kPawn:   c = 'p'; break;
          case PieceType::kKnight: c = 'n'; break;
          case PieceType::kBishop: c = 'b'; break;
          case PieceType::kRook:   c = 'r'; break;
          case PieceType::kQueen:  c = 'q'; break;
          case PieceType::kKing:   c = 'k'; break;
          default: break;
        }
        // clang-format on
        if (color == Color::kWhite) {
          c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        fen += c;
      }
    }
    if (empty > 0) fen += std::to_string(empty);
    if (r > 0) fen += '/';
  }

  fen += (side_to_move_ == Color::kWhite ? " w " : " b ");

  if (castling_rights_ == CastlingRights::kNoCastling) {
    fen += '-';
  } else {
    if (castling_rights_ & CastlingRights::kWhiteOO) fen += 'K';
    if (castling_rights_ & CastlingRights::kWhiteOOO) fen += 'Q';
    if (castling_rights_ & CastlingRights::kBlackOO) fen += 'k';
    if (castling_rights_ & CastlingRights::kBlackOOO) fen += 'q';
  }

  fen += ' ';
  if (ep_square_ == Square::kNone) {
    fen += '-';
  } else {
    fen += static_cast<char>('a' + FileOf(ep_square_));
    fen += static_cast<char>('1' + RankOf(ep_square_));
  }

  fen += ' ';
  fen += std::to_string(rule50_);
  fen += ' ';
  fen += std::to_string(FullMoveNumber());

  return fen;
}

void ChessBoard::MakeMove(Move m, StateInfo& new_st) {
  Square from = m.FromSquare();
  Square to = m.ToSquare();
  Piece pc = PieceOn(from);
  Piece captured = (m.TypeOf() == MoveType::kEnPassant
                        ? MakePiece(~side_to_move_, PieceType::kPawn)
                        : PieceOn(to));

  // Fill new state
  new_st.prev_state = st_;
  new_st.zobrist_key = zobrist_key_;
  new_st.rule50 = rule50_;
  new_st.castling_rights = castling_rights_;
  new_st.ep_square = ep_square_;
  new_st.captured_piece = captured;
  st_ = &new_st;

  // Update half move
  rule50_++;
  if (TypeOf(pc) == PieceType::kPawn || captured != Piece::kNoPiece) {
    rule50_ = 0;
  }

  game_ply_++;

  // Update hash for state that's about to change
  zobrist_key_ ^= zobrist::ColorKey(side_to_move_);
  zobrist_key_ ^= zobrist::CastlingKey(castling_rights_);
  if (ep_square_ != Square::kNone) {
    zobrist_key_ ^= zobrist::EnPassantKey(FileOf(ep_square_));
  }

  // Perform move
  if (m.TypeOf() == MoveType::kCastling) {
    MovePiece(from, to);
    Square rfrom, rto;
    if (to == Square::kG1) {
      // white oo
      rfrom = Square::kH1;
      rto = Square::kF1;
    } else if (to == Square::kC1) {
      // white ooo
      rfrom = Square::kA1;
      rto = Square::kD1;
    } else if (to == Square::kG8) {
      // black oo
      rfrom = Square::kH8;
      rto = Square::kF8;
    } else {
      // black ooo
      rfrom = Square::kA8;
      rto = Square::kD8;
    }
    MovePiece(rfrom, rto);
  } else if (m.TypeOf() == MoveType::kEnPassant) {
    MovePiece(from, to);
    RemovePiece(to - PawnDirection(side_to_move_));
  } else if (m.TypeOf() == MoveType::kPromotion) {
    RemovePiece(from);
    if (captured != Piece::kNoPiece) {
      RemovePiece(to);
    }
    PutPiece(to, MakePiece(side_to_move_, m.PromotionType()));
  } else {
    if (captured != Piece::kNoPiece) {
      RemovePiece(to);
    }
    MovePiece(from, to);
  }

  // Update EP square
  ep_square_ = Square::kNone;
  if (TypeOf(pc) == PieceType::kPawn &&
      std::abs(static_cast<int>(RankOf(from)) - static_cast<int>(RankOf(to))) ==
          2) {
    Square mid = from + PawnDirection(side_to_move_);
    // Check if any enemy pawn can capture it
    if (attacks::GetPawnAttacks(mid, side_to_move_) &
        Pieces(PieceType::kPawn, ~side_to_move_)) {
      ep_square_ = mid;
      zobrist_key_ ^= zobrist::EnPassantKey(FileOf(ep_square_));
    }
  }

  // Update castling rights
  if (castling_rights_ != CastlingRights::kNoCastling) {
    if (from == Square::kE1 || to == Square::kE1) {
      ClearCastlingRights(castling_rights_, CastlingRights::kWhiteCastling);
    }
    if (from == Square::kE8 || to == Square::kE8) {
      ClearCastlingRights(castling_rights_, CastlingRights::kBlackCastling);
    }
    if (from == Square::kA1 || to == Square::kA1) {
      ClearCastlingRights(castling_rights_, CastlingRights::kWhiteOOO);
    }
    if (from == Square::kH1 || to == Square::kH1) {
      ClearCastlingRights(castling_rights_, CastlingRights::kWhiteOO);
    }
    if (from == Square::kA8 || to == Square::kA8) {
      ClearCastlingRights(castling_rights_, CastlingRights::kBlackOOO);
    }
    if (from == Square::kH8 || to == Square::kH8) {
      ClearCastlingRights(castling_rights_, CastlingRights::kBlackOO);
    }
  }

  side_to_move_ = ~side_to_move_;
  zobrist_key_ ^= zobrist::ColorKey(side_to_move_);
  zobrist_key_ ^= zobrist::CastlingKey(castling_rights_);
}

void ChessBoard::UnmakeMove(Move m) {
  side_to_move_ = ~side_to_move_;
  game_ply_--;

  Square from = m.FromSquare();
  Square to = m.ToSquare();
  Piece captured = st_->captured_piece;

  if (m.TypeOf() == MoveType::kCastling) {
    MovePiece(to, from);
    Square rfrom, rto;
    if (to == Square::kG1) {
      // White OO
      rfrom = Square::kH1;
      rto = Square::kF1;
    } else if (to == Square::kC1) {
      // White OOO
      rfrom = Square::kA1;
      rto = Square::kD1;
    } else if (to == Square::kG8) {
      // Black OO
      rfrom = Square::kH8;
      rto = Square::kF8;
    } else {
      // Black OOO
      rfrom = Square::kA8;
      rto = Square::kD8;
    }
    MovePiece(rto, rfrom);
  } else if (m.TypeOf() == MoveType::kEnPassant) {
    MovePiece(to, from);
    PutPiece(to - PawnDirection(side_to_move_),
             MakePiece(~side_to_move_, PieceType::kPawn));
  } else if (m.TypeOf() == MoveType::kPromotion) {
    RemovePiece(to);
    PutPiece(from, MakePiece(side_to_move_, PieceType::kPawn));
    if (captured != Piece::kNoPiece) {
      PutPiece(to, captured);
    }
  } else {
    MovePiece(to, from);
    if (captured != Piece::kNoPiece) {
      PutPiece(to, captured);
    }
  }

  rule50_ = st_->rule50;
  castling_rights_ = st_->castling_rights;
  ep_square_ = st_->ep_square;
  zobrist_key_ = st_->zobrist_key;
  st_ = st_->prev_state;
}

bool ChessBoard::IsAttacked(Square sq, Color by_color) const {
  Bitboard occ = Occupied();
  // A square is attacked by color C if a pawn of color ~C at sq would attack a
  // C pawn
  if (attacks::GetPawnAttacks(sq, ~by_color) &
      Pieces(PieceType::kPawn, by_color))
    return true;
  if (attacks::GetKnightAttacks(sq) & Pieces(PieceType::kKnight, by_color))
    return true;
  if (attacks::GetKingAttacks(sq) & Pieces(PieceType::kKing, by_color))
    return true;
  if (attacks::GetBishopAttacks(sq, occ) &
      (Pieces(PieceType::kBishop, by_color) |
       Pieces(PieceType::kQueen, by_color)))
    return true;
  if (attacks::GetRookAttacks(sq, occ) & (Pieces(PieceType::kRook, by_color) |
                                          Pieces(PieceType::kQueen, by_color)))
    return true;
  return false;
}

Bitboard ChessBoard::Checkers() const noexcept {
  Color us = side_to_move_;
  Color them = ~us;
  Square ksq = KingSquare(us);

  Bitboard occ = Occupied();
  Bitboard checkers = 0ULL;

  // A king at ksq is attacked by color 'them' if a pawn of color 'us' at ksq
  // would attack a 'them' pawn
  checkers |= attacks::GetPawnAttacks(ksq, us) & Pieces(PieceType::kPawn, them);
  checkers |= attacks::GetKnightAttacks(ksq) & Pieces(PieceType::kKnight, them);
  checkers |=
      attacks::GetBishopAttacks(ksq, occ) &
      (Pieces(PieceType::kBishop, them) | Pieces(PieceType::kQueen, them));
  checkers |=
      attacks::GetRookAttacks(ksq, occ) &
      (Pieces(PieceType::kRook, them) | Pieces(PieceType::kQueen, them));

  return checkers;
}

bool ChessBoard::GivesCheck(Move m) const noexcept {
  Square from = m.FromSquare();
  Square to = m.ToSquare();
  Color us = side_to_move_;
  Color them = ~us;
  Square ksq = KingSquare(them);
  Bitboard occ = Occupied();
  Bitboard from_bb = SquareToBitboard(from);
  Bitboard to_bb = SquareToBitboard(to);
  Bitboard ksq_bb = SquareToBitboard(ksq);
  MoveType mt = m.TypeOf();

  Bitboard next_occ = (occ ^ from_bb) | to_bb;

  Square rfrom = Square::kNone;
  Square rto = Square::kNone;

  // Castling
  if (mt == MoveType::kCastling) {
    if (to == Square::kG1) {
      rfrom = Square::kH1;
      rto = Square::kF1;
    } else if (to == Square::kC1) {
      rfrom = Square::kA1;
      rto = Square::kD1;
    } else if (to == Square::kG8) {
      rfrom = Square::kH8;
      rto = Square::kF8;
    } else if (to == Square::kC8) {
      rfrom = Square::kA8;
      rto = Square::kD8;
    } else {
      __builtin_unreachable();
    }

    next_occ ^= SquareToBitboard(rfrom) | SquareToBitboard(rto);
  }

  // En Passant
  if (mt == MoveType::kEnPassant) {
    next_occ ^= SquareToBitboard(to - PawnDirection(us));
  }

  PieceType pt =
      (mt == MoveType::kPromotion) ? m.PromotionType() : TypeOf(PieceOn(from));

  // Direct Check
  switch (pt) {
    case PieceType::kPawn:
      if (attacks::GetPawnAttacks(to, us) & ksq_bb) {
        return true;
      }
      break;
    case PieceType::kKnight:
      if (attacks::GetKnightAttacks(to) & ksq_bb) {
        return true;
      }
      break;
    case PieceType::kBishop:
      if (attacks::GetBishopAttacks(to, next_occ) & ksq_bb) {
        return true;
      }
      break;
    case PieceType::kRook:
      if (attacks::GetRookAttacks(to, next_occ) & ksq_bb) {
        return true;
      }
      break;
    case PieceType::kQueen:
      if (attacks::GetQueenAttacks(to, next_occ) & ksq_bb) {
        return true;
      }
      break;
    default:
      break;
  }

  // Castling Rook Direct Check
  if (mt == MoveType::kCastling) {
    if (attacks::GetRookAttacks(rto, next_occ) & ksq_bb) {
      return true;
    }
  }

  // Discovered Check
  Bitboard our_rq =
      (Pieces(PieceType::kRook, us) | Pieces(PieceType::kQueen, us)) &
      ~from_bb & ~to_bb;
  if (mt == MoveType::kCastling) {
    our_rq = (our_rq ^ SquareToBitboard(rfrom)) | SquareToBitboard(rto);
  }
  if (attacks::GetRookAttacks(ksq, next_occ) & our_rq) {
    return true;
  }

  Bitboard our_bq =
      (Pieces(PieceType::kBishop, us) | Pieces(PieceType::kQueen, us)) &
      ~from_bb & ~to_bb;
  if (attacks::GetBishopAttacks(ksq, next_occ) & our_bq) {
    return true;
  }

  return false;
}

void ChessBoard::MakeNullMove(StateInfo& new_st) {
  new_st.prev_state = st_;
  new_st.zobrist_key = zobrist_key_;
  new_st.rule50 = rule50_;
  new_st.castling_rights = castling_rights_;
  new_st.ep_square = ep_square_;
  new_st.captured_piece = Piece::kNoPiece;
  st_ = &new_st;

  zobrist_key_ ^= zobrist::ColorKey(side_to_move_);
  if (ep_square_ != Square::kNone) {
    zobrist_key_ ^= zobrist::EnPassantKey(FileOf(ep_square_));
  }

  side_to_move_ = ~side_to_move_;
  ep_square_ = Square::kNone;
  rule50_++;
  game_ply_++;

  zobrist_key_ ^= zobrist::ColorKey(side_to_move_);
}

void ChessBoard::UnmakeNullMove() {
  side_to_move_ = ~side_to_move_;
  game_ply_--;
  rule50_ = st_->rule50;
  castling_rights_ = st_->castling_rights;
  ep_square_ = st_->ep_square;
  zobrist_key_ = st_->zobrist_key;
  st_ = st_->prev_state;
}

}  // namespace punch
