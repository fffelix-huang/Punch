#ifndef PUNCH_CHESS_BOARD_H_
#define PUNCH_CHESS_BOARD_H_

#include <bit>
#include <string>
#include <string_view>

#include "chess/bitboard.h"
#include "chess/types.h"
#include "chess/zobrist.h"

namespace punch {

constexpr std::string_view kInitialFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct StateInfo {
  Key zobrist_key;
  StateInfo* prev_state;

  int rule50;

  CastlingRights castling_rights;
  Square ep_square;
  Piece captured_piece;
};

class ChessBoard {
 public:
  ChessBoard() { LoadFen(kInitialFen); }
  ChessBoard(const ChessBoard&) = default;
  ChessBoard(std::string_view fen) { LoadFen(fen); }
  ChessBoard& operator=(const ChessBoard&) = default;

  void LoadFen(std::string_view fen);
  [[nodiscard]] std::string GetFen() const;

  [[nodiscard]] inline Color SideToMove() const noexcept {
    return side_to_move_;
  }
  [[nodiscard]] inline int GetRule50Ply() const noexcept { return rule50_; }
  [[nodiscard]] inline int FullMoveNumber() const noexcept {
    return (game_ply_ / 2) + 1;
  }
  [[nodiscard]] inline int GamePly() const noexcept { return game_ply_; }
  [[nodiscard]] inline Square EpSquare() const { return ep_square_; }
  [[nodiscard]] inline CastlingRights Castling() const noexcept {
    return castling_rights_;
  }
  [[nodiscard]] inline Key GetHashKey() const { return zobrist_key_; }

  [[nodiscard]] inline Piece PieceOn(Square sq) const { return board_[sq]; }
  [[nodiscard]] inline Bitboard Pieces(PieceType pt) const {
    return by_type_[pt];
  }
  [[nodiscard]] inline Bitboard Pieces(PieceType pt, Color c) const {
    return by_type_[pt] & by_color_[c];
  }
  [[nodiscard]] inline Bitboard Us(Color c) const { return by_color_[c]; }
  [[nodiscard]] inline Bitboard Occupied() const noexcept {
    return by_color_[Color::kWhite] | by_color_[Color::kBlack];
  }
  [[nodiscard]] inline Square KingSquare(Color c) const noexcept {
    return static_cast<Square>(std::countr_zero(Pieces(PieceType::kKing, c)));
  }

  [[nodiscard]] inline bool IsCapture(Move m) const {
    const Square& to = m.ToSquare();
    return PieceOn(to) != Piece::kNoPiece || m.TypeOf() == MoveType::kPromotion;
  }

  [[nodiscard]] bool IsRepetition() const noexcept {
    StateInfo* p = st_;
    int check_count = rule50_;

    if (!p || !p->prev_state || check_count < 2) {
      return false;
    }
    p = p->prev_state;
    check_count--;

    while (p && p->prev_state && p->prev_state->prev_state &&
           check_count >= 2) {
      p = p->prev_state->prev_state;
      check_count -= 2;

      if (p->zobrist_key == zobrist_key_) {
        return true;
      }
    }

    return false;
  }

  [[nodiscard]] bool IsInsufficientMaterial() const {
    if (Pieces(PieceType::kPawn) || Pieces(PieceType::kRook) ||
        Pieces(PieceType::kQueen)) {
      return false;
    }

    for (PieceType pt : {PieceType::kKnight, PieceType::kBishop}) {
      for (Color c : {Color::kWhite, Color::kBlack}) {
        if (PopCount(Pieces(pt, c)) > 1) {
          return false;
        }
      }
    }

    return true;
  }

  [[nodiscard]] bool IsDraw() const {
    return GetRule50Ply() >= 100 || IsRepetition() || IsInsufficientMaterial();
  }

  void MakeMove(Move m, StateInfo& new_st);
  void UnmakeMove(Move m);

  void MakeNullMove(StateInfo& new_st);
  void UnmakeNullMove();

  [[nodiscard]] bool IsAttacked(Square sq, Color by_color) const;

  [[nodiscard]] Bitboard Checkers() const noexcept;
  [[nodiscard]] bool InCheck() const noexcept { return Checkers() != 0ULL; }

 private:
  inline void PutPiece(Square sq, Piece pc) {
    board_[sq] = pc;
    SetBit(by_type_[TypeOf(pc)], sq);
    SetBit(by_color_[ColorOf(pc)], sq);
    zobrist_key_ ^= zobrist::PieceKey(sq, pc);
  }

  inline void RemovePiece(Square sq) {
    Piece pc = board_[sq];
    ClearBit(by_type_[TypeOf(pc)], sq);
    ClearBit(by_color_[ColorOf(pc)], sq);
    board_[sq] = Piece::kNoPiece;
    zobrist_key_ ^= zobrist::PieceKey(sq, pc);
  }

  inline void MovePiece(Square from, Square to) {
    Piece pc = board_[from];
    Bitboard from_to = (1ULL << from) | (1ULL << to);
    by_type_[TypeOf(pc)] ^= from_to;
    by_color_[ColorOf(pc)] ^= from_to;
    board_[from] = Piece::kNoPiece;
    board_[to] = pc;
    zobrist_key_ ^= zobrist::PieceKey(from, pc) ^ zobrist::PieceKey(to, pc);
  }

  Piece board_[Square::kSquareNb];
  Bitboard by_type_[PieceType::kPieceTypeNb];
  Bitboard by_color_[Color::kColorNb];

  Color side_to_move_;
  CastlingRights castling_rights_;
  Square ep_square_;
  int rule50_;
  int game_ply_;
  Key zobrist_key_;
  StateInfo* st_ = nullptr;
};

}  // namespace punch

#endif  // PUNCH_CHESS_BOARD_H_
