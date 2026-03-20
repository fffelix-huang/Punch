#ifndef PUNCH_CHESS_TYPES_H_
#define PUNCH_CHESS_TYPES_H_

#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace punch {

#if defined(__clang__)
#define PUNCH_ASSUME(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
#if __GNUC__ >= 13
#define PUNCH_ASSUME(cond) __attribute__((assume(cond)))
#else
#define PUNCH_ASSUME(cond)                \
  do {                                    \
    if (!(cond)) __builtin_unreachable(); \
  } while (0)
#endif
#elif defined(_MSC_VER)
#define PUNCH_ASSUME(cond) __assume(cond)
#else
#define PUNCH_ASSUME(cond)
#endif

using Key = uint64_t;
using Bitboard = uint64_t;

constexpr int kMaxPly = 250;

enum Color : uint8_t { kWhite, kBlack, kColorNb = 2 };

enum CastlingRights : uint8_t {
  kNoCastling,
  kWhiteOO,
  kWhiteOOO = kWhiteOO << 1,
  kBlackOO = kWhiteOO << 2,
  kBlackOOO = kWhiteOO << 3,

  kKingSide = kWhiteOO | kBlackOO,
  kQueenSide = kWhiteOOO | kBlackOOO,
  kWhiteCastling = kWhiteOO | kWhiteOOO,
  kBlackCastling = kBlackOO | kBlackOOO,
  kAnyCastling = kWhiteCastling | kBlackCastling,

  kCastlingRightNb = 16,
};

using Value = int;

constexpr Value kValueZero = 0;
constexpr Value kValueDraw = 0;
constexpr Value kValueNone = 32002;
constexpr Value kValueInf = 32001;
constexpr Value kValueMate = 32000;
constexpr Value kValueMateInMaxPly = kValueMate - kMaxPly;
constexpr Value kValueMatedInMaxPly = -kValueMateInMaxPly;

constexpr bool IsValid(Value value) { return value != kValueNone; }

constexpr bool IsMateValue(Value score) {
  return kValueMate - std::abs(score) <= kMaxPly;
}
constexpr Value MateIn(int ply) { return kValueMate - ply; }
constexpr Value MatedIn(int ply) { return -kValueMate + ply; }

inline std::string ValueToString(Value score) {
  assert(std::abs(score) <= kValueMate);
  if (IsMateValue(score)) {
    int plies = (score > 0) ? (kValueMate - score) : (-kValueMate - score);
    int moves = (plies + (plies > 0 ? 1 : -1)) / 2;
    return std::format("mate {}", moves);
  }
  return std::format("cp {}", score);
}

// clang-format off
enum PieceType : uint8_t {
  kNoPieceType, kPawn, kKnight, kBishop, kRook, kQueen, kKing,
  kAllPieces = 0,
  kPieceTypeNb = 8
};

enum Piece : uint8_t {
  kNoPiece,
  kWhitePawn = kPawn, kWhiteKnight, kWhiteBishop, kWhiteRook, kWhiteQueen, kWhiteKing,
  kBlackPawn = kPawn + 8, kBlackKnight, kBlackBishop, kBlackRook, kBlackQueen, kBlackKing,
  kPieceNb = 16
};
// clang-format on

// kNoPieceType, kPawn, kKnight, kBishop, kRook, kQueen, kKing,
constexpr Value kPieceValue[] = {Value(0),   Value(100), Value(290), Value(310),
                                 Value(500), Value(900), Value(0)};

using Depth = int;

// clang-format off
enum Square : uint8_t {
  kA1, kB1, kC1, kD1, kE1, kF1, kG1, kH1,
  kA2, kB2, kC2, kD2, kE2, kF2, kG2, kH2,
  kA3, kB3, kC3, kD3, kE3, kF3, kG3, kH3,
  kA4, kB4, kC4, kD4, kE4, kF4, kG4, kH4,
  kA5, kB5, kC5, kD5, kE5, kF5, kG5, kH5,
  kA6, kB6, kC6, kD6, kE6, kF6, kG6, kH6,
  kA7, kB7, kC7, kD7, kE7, kF7, kG7, kH7,
  kA8, kB8, kC8, kD8, kE8, kF8, kG8, kH8,
  kNone,

  kSquareZero = 0,
  kSquareNb   = 64
};
// clang-format on

enum Direction : int8_t {
  kNorth = 8,
  kEast = 1,
  kSouth = -kNorth,
  kWest = -kEast,

  kNorthEast = kNorth + kEast,
  kSouthEast = kSouth + kEast,
  kSouthWest = kSouth + kWest,
  kNorthWest = kNorth + kWest
};

enum File : uint8_t {
  kFileA,
  kFileB,
  kFileC,
  kFileD,
  kFileE,
  kFileF,
  kFileG,
  kFileH,
  kFileNb
};

enum Rank : uint8_t {
  kRank1,
  kRank2,
  kRank3,
  kRank4,
  kRank5,
  kRank6,
  kRank7,
  kRank8,
  kRankNb
};

constexpr Direction operator+(Direction d1, Direction d2) {
  return Direction(int(d1) + int(d2));
}
constexpr Direction operator*(int i, Direction d) {
  return Direction(i * int(d));
}
constexpr Direction operator*(Direction d, int i) {
  return Direction(int(d) * i);
}

constexpr Square operator+(Square s, Direction d) {
  return Square(int(s) + int(d));
}
constexpr Square operator-(Square s, Direction d) {
  return Square(int(s) - int(d));
}
constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }
constexpr Square& operator-=(Square& s, Direction d) { return s = s - d; }

// Toggle color
constexpr Color operator~(Color c) { return Color(c ^ Color::kBlack); }

constexpr void SetCastlingRights(CastlingRights& a, CastlingRights b) {
  a = static_cast<CastlingRights>(static_cast<uint8_t>(a) |
                                  static_cast<uint8_t>(b));
}

constexpr void ClearCastlingRights(CastlingRights& a, CastlingRights b) {
  a = static_cast<CastlingRights>(static_cast<uint8_t>(a) &
                                  static_cast<uint8_t>(~b));
}

constexpr CastlingRights operator&(Color c, CastlingRights cr) {
  return CastlingRights((c == Color::kWhite ? CastlingRights::kWhiteCastling
                                            : CastlingRights::kBlackCastling) &
                        cr);
}

constexpr Square MakeSquare(File f, Rank r) { return Square((r << 3) + f); }
constexpr Piece MakePiece(Color c, PieceType pt) {
  return Piece((c << 3) + pt);
}

constexpr PieceType TypeOf(Piece pc) { return PieceType(pc & 7); }
constexpr File FileOf(Square s) { return File(s & 7); }
constexpr Rank RankOf(Square s) { return Rank(s >> 3); }
constexpr Color ColorOf(Piece pc) {
  assert(pc != Piece::kNoPiece);
  return Color(pc >> 3);
}

constexpr Direction PawnDirection(Color c) {
  return c == Color::kWhite ? Direction::kNorth : Direction::kSouth;
}

// Based on a congruential pseudo-random number generator
constexpr Key MakeKey(uint64_t seed) {
  return seed * 6364136223846793005ULL + 1442695040888963407ULL;
}

enum MoveType : uint16_t {
  kNormal,
  kPromotion = 1 << 14,
  kEnPassant = 2 << 14,
  kCastling = 3 << 14
};

// A move needs 16 bits to be stored
//
// bit  0- 5: destination square (from 0 to 63)
// bit  6-11: origin square (from 0 to 63)
// bit 12-13: 4 types of promotion pieces
// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
// NOTE: en passant bit is set only when a pawn can be captured
//
// Special cases are Move::none() and Move::null(). We can sneak these in
// because in any normal move the destination square and origin square are
// always different, but Move::none() and Move::null() have the same origin and
// destination square.

class Move {
 public:
  Move() = default;
  constexpr explicit Move(uint16_t d) : data(d) {}

  constexpr Move(Square from, Square to) : data((from << 6) + to) {}

  template <MoveType T>
  static constexpr Move Make(Square from, Square to,
                             PieceType pt = PieceType::kKnight) {
    return Move(T + ((pt - PieceType::kKnight) << 12) + (from << 6) + to);
  }

  constexpr Square FromSquare() const {
    assert(IsOk());
    return Square((data >> 6) & 0x3F);
  }

  constexpr Square ToSquare() const {
    assert(IsOk());
    return Square(data & 0x3F);
  }

  // Same as ToSquare() but without assertion, for branchless code paths
  // where the result is masked/ignored when move is not ok
  constexpr Square ToSquareUnchecked() const { return Square(data & 0x3F); }

  constexpr MoveType TypeOf() const { return MoveType(data & (3 << 14)); }

  constexpr PieceType PromotionType() const {
    return PieceType(((data >> 12) & 3) + PieceType::kKnight);
  }

  constexpr bool IsOk() const {
    return Move::None().data != data && Move::Null().data != data;
  }

  static constexpr Move Null() { return Move(65); }
  static constexpr Move None() { return Move(0); }

  constexpr bool operator==(const Move& m) const { return data == m.data; }
  constexpr bool operator!=(const Move& m) const { return data != m.data; }

  constexpr explicit operator bool() const { return data != 0; }

  constexpr std::uint16_t Raw() const { return data; }

  std::string ToString() const {
    if (!IsOk()) {
      return "(none)";
    }

    Square from = FromSquare();
    Square to = ToSquare();

    std::string s;
    s.reserve(5);

    s += static_cast<char>('a' + static_cast<int>(FileOf(from)));
    s += static_cast<char>('1' + static_cast<int>(RankOf(from)));
    s += static_cast<char>('a' + static_cast<int>(FileOf(to)));
    s += static_cast<char>('1' + static_cast<int>(RankOf(to)));

    if (TypeOf() == MoveType::kPromotion) {
      static constexpr std::string_view kPromotionChars = "nbrq";
      int idx = static_cast<int>(PromotionType()) -
                static_cast<int>(PieceType::kKnight);
      if (idx >= 0 && idx < 4) {
        s += kPromotionChars[idx];
      }
    }

    return s;
  }

  struct MoveHash {
    size_t operator()(const Move& m) const { return MakeKey(m.data); }
  };

 protected:
  std::uint16_t data;
};

}  // namespace punch

#endif  // PUNCH_CHESS_TYPES_H_
