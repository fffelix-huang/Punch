#ifndef PUNCH_CHESS_ZOBRIST_H_
#define PUNCH_CHESS_ZOBRIST_H_

#include <array>
#include <cstdint>

#include "chess/types.h"

namespace punch::zobrist {

namespace internal {

// Polyglot random keys.
// http://hgm.nubati.net/book_format.html
// Piece positions: 2 * 6 * 64 = 768
// Castlings: 4
// En passant: 8
// Color: 1
// Total: 781

inline constexpr Key kZobristKeys[781] = {
#include "chess/zobrist.inc"
};

constexpr int kPieceToPolyglotIndex[16] = {-1, 1, 3, 5, 7, 9, 11, -1,
                                           -1, 0, 2, 4, 6, 8, 10, -1};

constexpr std::array<Key, 16> kCastlingTable = []() {
  std::array<Key, 16> table{};
  constexpr size_t offset = 768;
  for (int i = 0; i < 16; ++i) {
    Key key = 0;
    if (i & CastlingRights::kWhiteOO) {
      key ^= kZobristKeys[offset + 0];
    }
    if (i & CastlingRights::kWhiteOOO) {
      key ^= kZobristKeys[offset + 1];
    }
    if (i & CastlingRights::kBlackOO) {
      key ^= kZobristKeys[offset + 2];
    }
    if (i & CastlingRights::kBlackOOO) {
      key ^= kZobristKeys[offset + 3];
    }
    table[i] = key;
  }
  return table;
}();

}  // namespace internal

[[nodiscard]] inline constexpr Key PieceKey(Square sq, Piece p) noexcept {
  if (p == Piece::kNoPiece) {
    return Key(0);
  }
  size_t piece_index = internal::kPieceToPolyglotIndex[p];
  return internal::kZobristKeys[(piece_index << 6) + sq];
}

[[nodiscard]] inline constexpr Key CastlingKey(CastlingRights cr) noexcept {
  return internal::kCastlingTable[cr];
}

[[nodiscard]] inline constexpr Key EnPassantKey(File f) noexcept {
  return internal::kZobristKeys[772 + f];
}

[[nodiscard]] inline constexpr Key ColorKey(Color c) noexcept {
  return c == Color::kWhite ? internal::kZobristKeys[780] : Key(0);
}

}  // namespace punch::zobrist

#endif  // PUNCH_CHESS_ZOBRIST_H_
