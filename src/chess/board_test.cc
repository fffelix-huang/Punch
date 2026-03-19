#include "chess/board.h"

#include <gtest/gtest.h>

#include <string_view>

#include "chess/attacks.h"
#include "chess/movegen.h"
#include "chess/types.h"

namespace punch::test {

class BoardTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { attacks::Initialize(); }
};

TEST_F(BoardTest, InitialFen) {
  ChessBoard board;
  EXPECT_EQ(board.GetFen(), kInitialFen);
  EXPECT_EQ(board.SideToMove(), Color::kWhite);
  EXPECT_EQ(board.EpSquare(), Square::kNone);
  EXPECT_EQ(board.Castling(), CastlingRights::kAnyCastling);
  EXPECT_EQ(board.FullMoveNumber(), 1);
  EXPECT_EQ(board.GamePly(), 0);
}

TEST_F(BoardTest, LoadGetFen) {
  constexpr std::string_view fens[] = {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
      "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
      "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
      "5rk1/q6p/2p3bR/1pPp1rP1/1P1Pp3/P3B1Q1/1K3P2/R7 w - - 93 90",
      "4rrk1/1p1nq3/p7/2p1P1pp/3P2bp/3Q1Bn1/PPPB4/1K2R1NR w - - 40 21",
      "r3k2r/3nnpbp/q2pp1p1/p7/Pp1PPPP1/4BNN1/1P5P/R2Q1RK1 w kq - 0 16",
      "3Qb1k1/1r2ppb1/pN1n2q1/Pp1Pp1Pr/4P2p/4BP2/4B1R1/1R5K b - - 11 40",
      "4k3/3q1r2/1N2r1b1/3ppN2/2nPP3/1B1R2n1/2R1Q3/3K4 w - - 5 1",
      "1r6/1P4bk/3qr1p1/N6p/3pp2P/6R1/3Q1PP1/1R4K1 w - - 1 42",
  };

  ChessBoard board;
  for (std::string_view fen : fens) {
    board.LoadFen(fen);
    EXPECT_EQ(board.GetFen(), fen);
  }
}

TEST_F(BoardTest, PieceAccessors) {
  ChessBoard board(kInitialFen);
  EXPECT_EQ(board.PieceOn(Square::kE1), Piece::kWhiteKing);
  EXPECT_EQ(board.PieceOn(Square::kE8), Piece::kBlackKing);
  EXPECT_EQ(board.PieceOn(Square::kA1), Piece::kWhiteRook);
  EXPECT_EQ(board.PieceOn(Square::kA8), Piece::kBlackRook);

  EXPECT_EQ(board.Pieces(PieceType::kKing),
            (1ULL << Square::kE1) | (1ULL << Square::kE8));
  EXPECT_EQ(board.Us(Color::kWhite), 0xFFFFULL);
  EXPECT_EQ(board.Us(Color::kBlack), 0xFFFF000000000000ULL);
  EXPECT_EQ(board.Occupied(), 0xFFFF00000000FFFFULL);

  EXPECT_EQ(board.KingSquare(Color::kWhite), Square::kE1);
  EXPECT_EQ(board.KingSquare(Color::kBlack), Square::kE8);
}

TEST_F(BoardTest, NormalMove) {
  ChessBoard board(kInitialFen);
  const Key initial_key = board.GetHashKey();

  StateInfo st;
  Move m(Square::kE2, Square::kE4);

  board.MakeMove(m, st);
  EXPECT_EQ(board.PieceOn(Square::kE2), Piece::kNoPiece);
  EXPECT_EQ(board.PieceOn(Square::kE4), Piece::kWhitePawn);
  EXPECT_EQ(board.SideToMove(), Color::kBlack);
  EXPECT_EQ(board.GamePly(), 1);
  EXPECT_EQ(board.FullMoveNumber(), 1);
  EXPECT_EQ(board.EpSquare(), Square::kNone);

  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), kInitialFen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
}

TEST_F(BoardTest, CaptureMove) {
  ChessBoard board(
      "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
  const std::string initial_fen = board.GetFen();
  const Key initial_key = board.GetHashKey();

  StateInfo st;
  Move m(Square::kE4, Square::kD5);

  board.MakeMove(m, st);
  EXPECT_EQ(board.PieceOn(Square::kD5), Piece::kWhitePawn);
  EXPECT_EQ(board.PieceOn(Square::kE4), Piece::kNoPiece);

  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), initial_fen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
}

TEST_F(BoardTest, CastlingMove) {
  ChessBoard board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  std::string initial_fen = board.GetFen();
  Key initial_key = board.GetHashKey();

  StateInfo st;

  // White Kingside
  Move m = Move::Make<MoveType::kCastling>(Square::kE1, Square::kG1);
  board.MakeMove(m, st);
  EXPECT_EQ(board.PieceOn(Square::kG1), Piece::kWhiteKing);
  EXPECT_EQ(board.PieceOn(Square::kF1), Piece::kWhiteRook);
  EXPECT_FALSE(board.Castling() & CastlingRights::kWhiteCastling);

  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), initial_fen);
  EXPECT_EQ(board.GetHashKey(), initial_key);

  board.LoadFen("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
  initial_fen = board.GetFen();
  initial_key = board.GetHashKey();

  // Black Queenside
  m = Move::Make<MoveType::kCastling>(Square::kE8, Square::kC8);
  board.MakeMove(m, st);
  EXPECT_EQ(board.PieceOn(Square::kC8), Piece::kBlackKing);
  EXPECT_EQ(board.PieceOn(Square::kD8), Piece::kBlackRook);

  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), initial_fen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
}

TEST_F(BoardTest, EnPassantMove) {
  ChessBoard board(
      "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
  const std::string initial_fen = board.GetFen();
  const Key initial_key = board.GetHashKey();

  StateInfo st;
  Move m = Move::Make<MoveType::kEnPassant>(Square::kE5, Square::kF6);

  board.MakeMove(m, st);
  EXPECT_EQ(board.PieceOn(Square::kF6), Piece::kWhitePawn);
  EXPECT_EQ(board.PieceOn(Square::kF5), Piece::kNoPiece);

  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), initial_fen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
}

TEST_F(BoardTest, PromotionMove) {
  ChessBoard board("8/4P3/8/8/8/8/8/k6K w - - 0 1");
  const std::string initial_fen = board.GetFen();
  const Key initial_key = board.GetHashKey();

  StateInfo st;
  Move m = Move::Make<MoveType::kPromotion>(Square::kE7, Square::kE8,
                                            PieceType::kQueen);

  board.MakeMove(m, st);
  EXPECT_EQ(board.PieceOn(Square::kE8), Piece::kWhiteQueen);

  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), initial_fen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
}

TEST_F(BoardTest, PromotionMoveWithCapture) {
  ChessBoard board("3r4/4P3/8/8/8/8/8/k6K w - - 0 1");
  const std::string initial_fen = board.GetFen();
  const Key initial_key = board.GetHashKey();

  StateInfo st;
  Move m = Move::Make<MoveType::kPromotion>(Square::kE7, Square::kD8,
                                            PieceType::kQueen);

  board.MakeMove(m, st);
  EXPECT_EQ(board.PieceOn(Square::kD8), Piece::kWhiteQueen);

  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), initial_fen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
}

TEST_F(BoardTest, NullMove) {
  ChessBoard board(kInitialFen);
  Key initial_key = board.GetHashKey();
  StateInfo st;

  board.MakeNullMove(st);
  EXPECT_EQ(board.SideToMove(), Color::kBlack);
  EXPECT_EQ(board.EpSquare(), Square::kNone);
  EXPECT_EQ(board.GamePly(), 1);

  board.UnmakeNullMove();
  EXPECT_EQ(board.GetFen(), kInitialFen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
}

TEST_F(BoardTest, IsAttacked) {
  ChessBoard board(
      "r1bqkbnr/pppp1ppp/2n5/8/2BpP3/5N2/PPP2PPP/RNBQK2R w KQkq - 0 1");

  EXPECT_TRUE(board.IsAttacked(Square::kD4, Color::kWhite));
  EXPECT_FALSE(board.IsAttacked(Square::kE4, Color::kWhite));
  EXPECT_TRUE(board.IsAttacked(Square::kD5, Color::kWhite));
  EXPECT_TRUE(board.IsAttacked(Square::kF7, Color::kWhite));
  EXPECT_FALSE(board.IsAttacked(Square::kC5, Color::kWhite));

  EXPECT_TRUE(board.IsAttacked(Square::kD4, Color::kBlack));
  EXPECT_TRUE(board.IsAttacked(Square::kE3, Color::kBlack));
  EXPECT_TRUE(board.IsAttacked(Square::kH4, Color::kBlack));
  EXPECT_FALSE(board.IsAttacked(Square::kF5, Color::kBlack));
}

TEST_F(BoardTest, Checkers) {
  ChessBoard board("k7/8/8/8/8/8/1P6/K7 w - - 0 1");
  EXPECT_EQ(board.Checkers(), 0ULL);

  board.LoadFen("k7/8/8/8/8/2q5/8/K7 w - - 0 1");
  EXPECT_EQ(board.Checkers(), (1ULL << Square::kC3));
  EXPECT_EQ(board.PieceOn(Square::kC3), Piece::kBlackQueen);

  // Testing Knight at B3 checking King at A1
  board.LoadFen("k7/8/8/8/8/1n6/8/K7 w - - 0 1");
  EXPECT_EQ(board.Checkers(), (1ULL << Square::kB3));
  EXPECT_EQ(board.PieceOn(Square::kB3), Piece::kBlackKnight);

  // Testing multiple checkers
  board.LoadFen("k6b/8/8/8/8/rn6/2n5/K1r5 w - - 0 1");
  Bitboard checkers = 0ULL;
  SetBit(checkers, Square::kA3);
  SetBit(checkers, Square::kB3);
  SetBit(checkers, Square::kC1);
  SetBit(checkers, Square::kC2);
  SetBit(checkers, Square::kH8);
  EXPECT_EQ(board.Checkers(), checkers);
}

TEST_F(BoardTest, GivesCheck) {
  constexpr std::string_view fens[] = {
      // Normal Position
      "rnbqkbnr/pppp1ppp/8/4p3/4N3/8/PPPPPPPP/RNBQKB1R w KQkq - 0 1",
      "r1bqk2r/pppp1ppp/5n2/b7/2BQP3/2P5/PP3PPP/RNB1K2R w KQkq - 0 1",
      // Pawn Promotion
      "4k3/1P6/8/8/8/8/8/4K3 w - - 0 1",
      "8/2P1k3/8/8/8/8/8/4K3 w - - 0 1",
      "8/2P5/4k3/8/8/8/8/4K3 w - - 0 1",
      // En Passant
      "8/8/8/R2Pp2k/8/8/8/4K3 w - e6 0 1",
      "7k/8/8/3Pp3/8/8/8/B3K3 w - e6 0 1",
      "8/5k2/8/3Pp3/8/8/8/4K3 w - e6 0 1",
      "8/7k/8/3Pp3/8/8/8/4K3 w - e6 0 1",
      // Castling
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

    movegen::MoveList moves;
    movegen::GenerateLegalMoves<movegen::MoveGenType::kAll>(board, moves);

    for (Move m : moves) {
      bool result = board.GivesCheck(m);

      StateInfo st;
      board.MakeMove(m, st);

      bool expected = board.InCheck();

      board.UnmakeMove(m);

      EXPECT_EQ(board.GetFen(), fen);
      EXPECT_EQ(result, expected);
    }
  }
}

TEST_F(BoardTest, CastlingRightsUpdate) {
  ChessBoard board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  const std::string initial_fen = board.GetFen();
  const Key initial_key = board.GetHashKey();

  StateInfo st;

  // Capture rook
  Move m(Square::kA1, Square::kA8);
  board.MakeMove(m, st);
  EXPECT_FALSE(board.Castling() & CastlingRights::kWhiteOOO);
  EXPECT_FALSE(board.Castling() & CastlingRights::kBlackOOO);
  EXPECT_TRUE(board.Castling() & CastlingRights::kWhiteOO);
  EXPECT_TRUE(board.Castling() & CastlingRights::kBlackOO);
  board.UnmakeMove(m);
  EXPECT_EQ(board.GetFen(), initial_fen);
  EXPECT_EQ(board.GetHashKey(), initial_key);
  EXPECT_TRUE(board.Castling() & CastlingRights::kAnyCastling);
}

TEST_F(BoardTest, RepetitionDetection) {
  ChessBoard board(kInitialFen);
  const Key initial_key = board.GetHashKey();

  Move moves[] = {
      Move(Square::kG1, Square::kF3),
      Move(Square::kG8, Square::kF6),
      Move(Square::kF3, Square::kG1),
      Move(Square::kF6, Square::kG8),
  };
  StateInfo st[4];

  for (int i = 0; i < 4; ++i) {
    board.MakeMove(moves[i], st[i]);
  }

  EXPECT_EQ(board.GetHashKey(), initial_key);
  EXPECT_TRUE(board.IsRepetition());
}

}  // namespace punch::test
