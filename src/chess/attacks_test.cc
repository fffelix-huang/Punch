#include "chess/attacks.h"

#include <gtest/gtest.h>

#include "chess/bitboard.h"
#include "chess/types.h"

namespace punch::attacks::test {

class AttacksTest : public ::testing::Test {
 protected:
  // Ensure the magic tables are initialized before any tests run
  static void SetUpTestSuite() { attacks::Initialize(); }
};

// ============================================================================
// Rook Tests
// ============================================================================

TEST_F(AttacksTest, RookAttacksEmptyBoard) {
  Bitboard occ = 0ULL;
  Bitboard attacks = attacks::GetRookAttacks(Square::kE4, occ);
  Bitboard expected =
      (GetFileBitboard(File::kFileE) | GetRankBitboard(Rank::kRank4));
  ClearBit(expected, Square::kE4);

  EXPECT_EQ(attacks, expected) << "Rook rays on empty board are incorrect";
}

TEST_F(AttacksTest, RookAttacksFullyBlocked) {
  Bitboard occ = 0ULL;
  SetBit(occ, Square::kA2);
  SetBit(occ, Square::kB1);
  Bitboard attacks = attacks::GetRookAttacks(Square::kA1, occ);

  // Expected: Rays are stopped by the blockers (inclusive)
  Bitboard expected = 0ULL;
  SetBit(expected, Square::kA2);
  SetBit(expected, Square::kB1);

  EXPECT_EQ(attacks, expected) << "Rook should be fully blocked at the corner";
}

// ============================================================================
// Bishop Tests
// ============================================================================

TEST_F(AttacksTest, BishopAttacksEmptyBoard) {
  Bitboard occ = 0ULL;
  Bitboard attacks = attacks::GetBishopAttacks(Square::kD4, occ);

  Bitboard expected = 0ULL;
  // Diagonal 1: a1-h8 (excluding d4 itself)
  SetBit(expected, Square::kA1);
  SetBit(expected, Square::kB2);
  SetBit(expected, Square::kC3);
  SetBit(expected, Square::kE5);
  SetBit(expected, Square::kF6);
  SetBit(expected, Square::kG7);
  SetBit(expected, Square::kH8);

  // Diagonal 2: a7-g1 (excluding d4 itself)
  SetBit(expected, Square::kA7);
  SetBit(expected, Square::kB6);
  SetBit(expected, Square::kC5);
  SetBit(expected, Square::kE3);
  SetBit(expected, Square::kF2);
  SetBit(expected, Square::kG1);

  EXPECT_EQ(attacks, expected) << "Bishop rays on empty board are incorrect";
}

TEST_F(AttacksTest, BishopAttacksPartiallyBlocked) {
  Bitboard occ = 0ULL;

  // Place blockers on the 4 diagonal adjacent squares around c3
  SetBit(occ, Square::kC3 + Direction::kSouthWest);
  SetBit(occ, Square::kC3 + Direction::kNorthEast);
  SetBit(occ, Square::kC3 + Direction::kNorthWest);
  SetBit(occ, Square::kC3 + Direction::kSouthEast);

  // Add shadowed pieces behind the blockers to ensure rays don't penetrate
  SetBit(occ, Square::kC3 + Direction::kSouthEast + Direction::kSouthEast);
  SetBit(occ, Square::kC3 + Direction::kNorthWest + Direction::kNorthWest);

  Bitboard attacks = attacks::GetBishopAttacks(Square::kC3, occ);

  // Expected: Only the immediate blockers are attacked. Shadowed pieces are
  // safe.
  Bitboard expected = 0ULL;
  SetBit(expected, Square::kC3 + Direction::kSouthWest);
  SetBit(expected, Square::kC3 + Direction::kNorthEast);
  SetBit(expected, Square::kC3 + Direction::kNorthWest);
  SetBit(expected, Square::kC3 + Direction::kSouthEast);

  EXPECT_EQ(attacks, expected)
      << "Bishop rays should stop at the first blocker";
}

// ============================================================================
// Queen Tests
// ============================================================================

TEST_F(AttacksTest, QueenAttacksCombineRookAndBishop) {
  // Test Queen attacks with a pseudo-random occupancy configuration
  Bitboard occ = 0x123456789ABCDEF0ULL;
  Square sq = Square::kE4;

  Bitboard queen_attacks = attacks::GetQueenAttacks(sq, occ);
  Bitboard expected =
      attacks::GetRookAttacks(sq, occ) | attacks::GetBishopAttacks(sq, occ);

  EXPECT_EQ(queen_attacks, expected)
      << "Queen attacks must strictly be the union of Rook and Bishop";
}

TEST_F(AttacksTest, QueenAttacksFullyBlocked) {
  Bitboard occ = 0ULL;

  // Surround the Queen at e4 completely
  for (Direction d :
       {Direction::kNorth, Direction::kSouth, Direction::kEast,
        Direction::kWest, Direction::kNorthWest, Direction::kNorthEast,
        Direction::kSouthWest, Direction::kSouthEast}) {
    SetBit(occ, Square::kE4 + d);
  }

  Bitboard attacks = attacks::GetQueenAttacks(Square::kE4, occ);

  // Expected: Since it's fully surrounded, its attacks should exactly match the
  // occupancy
  EXPECT_EQ(attacks, occ)
      << "A fully surrounded Queen should only attack its immediate neighbors";
}

// ============================================================================
// Knight Tests
// ============================================================================

TEST_F(AttacksTest, KnightAttacksEdgeCases) {
  // Knight on the corner (h8)
  Bitboard attacks = attacks::GetKnightAttacks(Square::kH8);

  // Expected: Can only jump to f7 and g6
  Bitboard expected = 0ULL;
  SetBit(expected, Square::kF7);
  SetBit(expected, Square::kG6);

  EXPECT_EQ(attacks, expected);
}

TEST_F(AttacksTest, KnightAttacksCenter) {
  // Knight in the center (d4)
  Bitboard attacks = attacks::GetKnightAttacks(Square::kD4);

  Bitboard expected = 0ULL;
  SetBit(expected, Square::kC6);
  SetBit(expected, Square::kE6);
  SetBit(expected, Square::kB5);
  SetBit(expected, Square::kF5);
  SetBit(expected, Square::kB3);
  SetBit(expected, Square::kF3);
  SetBit(expected, Square::kC2);
  SetBit(expected, Square::kE2);

  EXPECT_EQ(attacks, expected);
}

// ============================================================================
// King Tests
// ============================================================================

TEST_F(AttacksTest, KingAttacksCorner) {
  Bitboard attacks = attacks::GetKingAttacks(Square::kA1);

  Bitboard expected = 0ULL;
  SetBit(expected, Square::kA2);
  SetBit(expected, Square::kB1);
  SetBit(expected, Square::kB2);

  EXPECT_EQ(attacks, expected);
}

// ============================================================================
// Pawn Tests
// ============================================================================

TEST_F(AttacksTest, PawnAttacksWhiteAndBlack) {
  // White pawn on d4 captures strictly "up" (North) diagonally
  Bitboard w_attacks = attacks::GetPawnAttacks(Square::kD4, Color::kWhite);
  Bitboard w_expected = 0ULL;
  SetBit(w_expected, Square::kD4 + Direction::kNorthWest);
  SetBit(w_expected, Square::kD4 + Direction::kNorthEast);

  EXPECT_EQ(w_attacks, w_expected);

  // Black pawn on d4 captures strictly "down" (South) diagonally
  Bitboard b_attacks = attacks::GetPawnAttacks(Square::kD4, Color::kBlack);
  Bitboard b_expected = 0ULL;
  SetBit(b_expected, Square::kD4 + Direction::kSouthWest);
  SetBit(b_expected, Square::kD4 + Direction::kSouthEast);

  EXPECT_EQ(b_attacks, b_expected);
}

}  // namespace punch::attacks::test
