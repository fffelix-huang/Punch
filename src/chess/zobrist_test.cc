#include "chess/zobrist.h"

#include <gtest/gtest.h>

#include "chess/types.h"

namespace punch::zobrist::test {

namespace {

constexpr Key kZobristKeys[781] = {
#include "chess/zobrist.inc"
};

}  // namespace

TEST(ZobristTest, PieceKey) {
  EXPECT_EQ(PieceKey(Square::kA1, kNoPiece), Key(0));
  EXPECT_EQ(PieceKey(Square::kA1, kWhitePawn), kZobristKeys[(1 << 6) + 0]);
  EXPECT_EQ(PieceKey(Square::kH8, kBlackKing), kZobristKeys[(10 << 6) + 63]);
}

TEST(ZobristTest, CastlingKey) {
  EXPECT_EQ(CastlingKey(CastlingRights::kNoCastling), Key(0));

  EXPECT_EQ(CastlingKey(CastlingRights::kWhiteOO), kZobristKeys[768]);
  EXPECT_EQ(CastlingKey(CastlingRights::kWhiteOOO), kZobristKeys[769]);
  EXPECT_EQ(CastlingKey(CastlingRights::kBlackOO), kZobristKeys[770]);
  EXPECT_EQ(CastlingKey(CastlingRights::kBlackOOO), kZobristKeys[771]);

  Key expected_any_castling = kZobristKeys[768] ^ kZobristKeys[769] ^
                              kZobristKeys[770] ^ kZobristKeys[771];
  EXPECT_EQ(CastlingKey(kAnyCastling), expected_any_castling);
}

TEST(ZobristTest, EnPassantKeyReturnsCorrectFile) {
  EXPECT_EQ(EnPassantKey(kFileA), kZobristKeys[772 + 0]);
  EXPECT_EQ(EnPassantKey(kFileB), kZobristKeys[772 + 1]);
  EXPECT_EQ(EnPassantKey(kFileC), kZobristKeys[772 + 2]);
  EXPECT_EQ(EnPassantKey(kFileD), kZobristKeys[772 + 3]);
  EXPECT_EQ(EnPassantKey(kFileE), kZobristKeys[772 + 4]);
  EXPECT_EQ(EnPassantKey(kFileF), kZobristKeys[772 + 5]);
  EXPECT_EQ(EnPassantKey(kFileG), kZobristKeys[772 + 6]);
  EXPECT_EQ(EnPassantKey(kFileH), kZobristKeys[772 + 7]);
}

TEST(ZobristTest, ColorKeyFollowsPolyglotStandard) {
  EXPECT_EQ(ColorKey(kWhite), kZobristKeys[780]);
  EXPECT_EQ(ColorKey(kBlack), Key(0));
}

}  // namespace punch::zobrist::test
