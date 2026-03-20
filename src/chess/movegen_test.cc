#include "chess/movegen.h"

#include <gtest/gtest.h>

#include <algorithm>

#include "chess/attacks.h"
#include "chess/board.h"
#include "chess/types.h"

namespace punch::movegen::test {

class MoveGenTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    attacks::Initialize();
    movegen::Initialize();
  }
};

TEST_F(MoveGenTest, InitialPositionMoveCount) {
  ChessBoard board(kInitialFen);
  MoveList moves;
  GenerateLegalMoves(board, moves);
  EXPECT_EQ(moves.size(), 20UL);
}

TEST_F(MoveGenTest, DoubleCheckKingMovesOnly) {
  // White king at E1 in double check by black rook at E8 and black knight at D3
  ChessBoard board("k3r3/8/8/8/8/3n4/8/4K3 w - - 0 1");
  MoveList moves;
  GenerateLegalMoves(board, moves);
  ASSERT_GT(moves.size(), 0UL);
  // Only king can move out of double check
  for (Move m : moves) {
    EXPECT_EQ(TypeOf(board.PieceOn(m.FromSquare())), PieceType::kKing);
  }
}

TEST_F(MoveGenTest, AbsolutePin) {
  // White rook pinning black queen to king
  ChessBoard board("4k3/4q3/8/8/8/8/8/4R2K b - - 0 1");
  MoveList moves;
  GenerateLegalMoves(board, moves);

  bool queen_moved = false;
  for (Move m : moves) {
    if (TypeOf(board.PieceOn(m.FromSquare())) == PieceType::kQueen) {
      // Queen must stay on the file to protect king
      EXPECT_EQ(FileOf(m.ToSquare()), File::kFileE);
      queen_moved = true;
    }
  }
  EXPECT_TRUE(queen_moved);
}

TEST_F(MoveGenTest, EnPassantLegal) {
  ChessBoard board(
      "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
  MoveList moves;
  GenerateLegalMoves(board, moves);

  bool ep_found = false;
  for (Move m : moves) {
    if (m.TypeOf() == MoveType::kEnPassant) {
      EXPECT_EQ(m.FromSquare(), Square::kE5);
      EXPECT_EQ(m.ToSquare(), Square::kF6);
      ep_found = true;
    }
  }
  EXPECT_TRUE(ep_found);
}

TEST_F(MoveGenTest, EnPassantPinnedDiagonally) {
  ChessBoard board("6bk/8/8/3Pp3/8/1K6/8/8 w - e6 0 1");
  MoveList moves;
  GenerateLegalMoves(board, moves);

  bool ep_found = false;
  for (Move m : moves) {
    if (m.TypeOf() == MoveType::kEnPassant) {
      ep_found = true;
      EXPECT_EQ(m.FromSquare(), Square::kD5);
      EXPECT_EQ(m.ToSquare(), Square::kE6);
    }
  }
  EXPECT_TRUE(ep_found);
}

TEST_F(MoveGenTest, EnPassantPinnedIllegal) {
  ChessBoard board("7k/8/8/2KPp2r/8/8/8/8 b - e6 0 1");
  MoveList moves;
  GenerateLegalMoves(board, moves);

  for (Move m : moves) {
    EXPECT_NE(m.TypeOf(), MoveType::kEnPassant);
  }
}

TEST_F(MoveGenTest, CastlingLegal) {
  ChessBoard board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  MoveList moves;
  GenerateLegalMoves(board, moves);

  bool oo_found = false;
  bool ooo_found = false;
  for (Move m : moves) {
    if (m.TypeOf() == MoveType::kCastling) {
      if (m.ToSquare() == Square::kG1) {
        oo_found = true;
      }
      if (m.ToSquare() == Square::kC1) {
        ooo_found = true;
      }
    }
  }
  EXPECT_TRUE(oo_found);
  EXPECT_TRUE(ooo_found);
}

TEST_F(MoveGenTest, Checkmate) {
  ChessBoard board("k6b/8/8/8/8/rn6/2n5/K1r5 w - - 0 1");
  MoveList moves;
  GenerateLegalMoves(board, moves);
  EXPECT_EQ(moves.size(), 0UL);
}

TEST_F(MoveGenTest, NoDuplicateMoves) {
  constexpr std::string_view fens[] = {
      "rnbqkbnr/pppp1ppp/8/4p3/4N3/8/PPPPPPPP/RNBQKB1R w KQkq - 0 1",
      "r1bqk2r/pppp1ppp/5n2/b7/2BQP3/2P5/PP3PPP/RNB1K2R w KQkq - 0 1",
      "4k3/1P6/8/8/8/8/8/4K3 w - - 0 1",
      "8/2P1k3/8/8/8/8/8/4K3 w - - 0 1",
      "8/2P5/4k3/8/8/8/8/4K3 w - - 0 1",
      "8/8/8/R2Pp2k/8/8/8/4K3 w - e6 0 1",
      "7k/8/8/3Pp3/8/8/8/B3K3 w - e6 0 1",
      "8/5k2/8/3Pp3/8/8/8/4K3 w - e6 0 1",
      "8/7k/8/3Pp3/8/8/8/4K3 w - e6 0 1",
      "5k2/8/8/8/8/8/8/4K2R w K - 0 1",
      "5k2/8/8/8/8/8/5P2/4K2R w - - 0 1",
      "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1",
      "3k4/8/8/8/8/8/3P4/R3K3 w Q - 0 1",
      "4k2r/8/8/8/8/8/8/5K2 b k - 0 1",
      "4k2r/5p2/8/8/8/8/8/5K2 b k - 0 1",
      "r3k3/8/8/8/8/8/8/3K4 b q - 0 1",
      "r3k3/3p4/8/8/8/8/8/3K4 b q - 0 1",
  };

  ChessBoard board;

  for (std::string_view fen : fens) {
    board.LoadFen(fen);

    movegen::MoveList all_moves;
    movegen::GenerateLegalMoves<movegen::MoveGenType::kAll>(board, all_moves);

    movegen::MoveList capture_moves;
    movegen::GenerateLegalMoves<movegen::MoveGenType::kCaptures>(board,
                                                                 capture_moves);
    movegen::MoveList quiet_moves;
    movegen::GenerateLegalMoves<movegen::MoveGenType::kQuiets>(board,
                                                               quiet_moves);

    EXPECT_EQ(all_moves.size(), capture_moves.size() + quiet_moves.size());

    movegen::MoveList combined = capture_moves;
    for (Move m : quiet_moves) {
      combined.push_back(m);
    }

    auto cmp = [](Move a, Move b) -> bool { return a.Raw() < b.Raw(); };

    std::ranges::sort(all_moves, cmp);
    std::ranges::sort(combined, cmp);

    for (size_t i = 0; i < all_moves.size(); ++i) {
      EXPECT_EQ(all_moves[i], combined[i]);
    }
  }
}

}  // namespace punch::movegen::test
