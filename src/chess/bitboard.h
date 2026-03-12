#ifndef PUNCH_CHESS_BITBOARD_H_
#define PUNCH_CHESS_BITBOARD_H_

#include <array>
#include <bit>
#include <cassert>

#include "chess/types.h"

namespace punch {

inline constexpr std::array<Bitboard, File::kFileNb> kFileBitboard = {
    0x0101010101010101ULL,  // File A
    0x0202020202020202ULL,  // File B
    0x0404040404040404ULL,  // File C
    0x0808080808080808ULL,  // File D
    0x1010101010101010ULL,  // File E
    0x2020202020202020ULL,  // File F
    0x4040404040404040ULL,  // File G
    0x8080808080808080ULL   // File H
};

inline constexpr std::array<Bitboard, Rank::kRankNb> kRankBitboard = {
    0x00000000000000FFULL,  // Rank 1
    0x000000000000FF00ULL,  // Rank 2
    0x0000000000FF0000ULL,  // Rank 3
    0x00000000FF000000ULL,  // Rank 4
    0x000000FF00000000ULL,  // Rank 5
    0x0000FF0000000000ULL,  // Rank 6
    0x00FF000000000000ULL,  // Rank 7
    0xFF00000000000000ULL   // Rank 8
};

constexpr void SetBit(Bitboard& bb, Square sq) noexcept {
  assert(sq <= Square::kH8);
  bb |= 1ULL << sq;
}

constexpr void ClearBit(Bitboard& bb, Square sq) noexcept {
  assert(sq <= Square::kH8);
  bb &= ~(1ULL << sq);
}

[[nodiscard]] constexpr Square PopLsb(Bitboard& bb) noexcept {
  assert(bb != Bitboard(0));
  int b = std::countr_zero(bb);
  bb &= bb - 1;
  return static_cast<Square>(b);
}

[[nodiscard]] constexpr int PopCount(Bitboard bb) noexcept {
  return std::popcount(bb);
}

[[nodiscard]] constexpr Bitboard SquareToBitboard(Square sq) noexcept {
  return (1ULL << sq);
}

template <typename... Squares>
[[nodiscard]] constexpr Bitboard SquaresToBitboard(Squares... sqs) noexcept {
  return (SquareToBitboard(static_cast<Square>(sqs)) | ...);
}

[[nodiscard]] constexpr Bitboard GetFileBitboard(File f) noexcept {
  return kFileBitboard[f];
}

[[nodiscard]] constexpr Bitboard GetRankBitboard(Rank r) noexcept {
  return kRankBitboard[r];
}

}  // namespace punch

#endif  // PUNCH_CHESS_BITBOARD_H_
