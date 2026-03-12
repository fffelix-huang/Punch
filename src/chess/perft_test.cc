#include "chess/perft.h"

#include <gtest/gtest.h>

#include <string>

#include "chess/attacks.h"
#include "chess/board.h"
#include "chess/movegen.h"

namespace punch::test {

struct PerftParams {
  std::string fen;
  int depth;
  uint64_t expected_nodes;
};

class PerftTest : public ::testing::TestWithParam<PerftParams> {
 protected:
  static void SetUpTestSuite() {
    attacks::Initialize();
    movegen::Initialize();
  }
};

TEST_P(PerftTest, CorrectNodeCount) {
  const auto& params = GetParam();
  ChessBoard board(params.fen);
  EXPECT_EQ(Perft(board, params.depth), params.expected_nodes)
      << "Failed Perft Test!"
      << "\n  FEN: " << params.fen << "\n  Depth: " << params.depth
      << "\n  Nodes: " << params.expected_nodes;
}

INSTANTIATE_TEST_SUITE_P(
    FenPositions, PerftTest,
    ::testing::Values(
        // clang-format off
        // From https://www.chessprogramming.org/Perft_Results
        PerftParams{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609ULL},
        PerftParams{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603ULL},
        PerftParams{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083ULL},
        PerftParams{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292ULL},
        PerftParams{"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 5, 15833292ULL}, // Mirror
        PerftParams{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487ULL},
        PerftParams{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594ULL},
        // From https://github.com/paulsonkoly/chess-3/blob/main/debug/standard.epd
        PerftParams{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609ULL},
        PerftParams{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603ULL},
        PerftParams{"4k3/8/8/8/8/8/8/4K2R w K - 0 1", 6, 764643ULL},
        PerftParams{"4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 846648ULL},
        PerftParams{"4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 6, 899442ULL},
        PerftParams{"r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 6, 1001523ULL},
        PerftParams{"4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 6, 2788982ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 6, 3517770ULL},
        PerftParams{"8/8/8/8/8/8/6k1/4K2R w K - 0 1", 6, 185867ULL},
        PerftParams{"8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 6, 413018ULL},
        PerftParams{"4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 6, 179869ULL},
        PerftParams{"r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 6, 367724ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 5, 7594526ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 5, 8153719ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 5, 7736373ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 5, 7878456ULL},
        PerftParams{"1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 5, 8198901ULL},
        PerftParams{"2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 5, 7710115ULL},
        PerftParams{"r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 5, 7848606ULL},
        PerftParams{"4k3/8/8/8/8/8/8/4K2R b K - 0 1", 6, 899442ULL},
        PerftParams{"4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 6, 1001523ULL},
        PerftParams{"4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 6, 764643ULL},
        PerftParams{"r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 6, 846648ULL},
        PerftParams{"4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 6, 3517770ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 6, 2788982ULL},
        PerftParams{"8/8/8/8/8/8/6k1/4K2R b K - 0 1", 6, 179869ULL},
        PerftParams{"8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 6, 367724ULL},
        PerftParams{"4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 6, 185867ULL},
        PerftParams{"r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 6, 413018ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 5, 7594526ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 5, 8198901ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 5, 7710115ULL},
        PerftParams{"r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 5, 7848606ULL},
        PerftParams{"1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 5, 8153719ULL},
        PerftParams{"2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 5, 7736373ULL},
        PerftParams{"r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 5, 7878456ULL},
        PerftParams{"8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 6, 8107539ULL},
        PerftParams{"8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 6, 2594412ULL},
        PerftParams{"8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 5, 1198299ULL},
        PerftParams{"K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 6, 588695ULL},
        PerftParams{"k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 6, 688780ULL},
        PerftParams{"8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 6, 8503277ULL},
        PerftParams{"8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 6, 3147566ULL},
        PerftParams{"8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 6, 4405103ULL},
        PerftParams{"K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 6, 688780ULL},
        PerftParams{"k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 6, 588695ULL},
        PerftParams{"B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 5, 1320507ULL},
        PerftParams{"8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 5, 1713368ULL},
        PerftParams{"k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 6, 7881673ULL},
        PerftParams{"K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 6, 7382896ULL},
        PerftParams{"B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 6, 9250746ULL},
        PerftParams{"8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 5, 1591064ULL},
        PerftParams{"k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 6, 7382896ULL},
        PerftParams{"K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 6, 7881673ULL},
        PerftParams{"7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 5, 2161211ULL},
        PerftParams{"R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 4, 771461ULL},
        PerftParams{"7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 5, 2161211ULL},
        PerftParams{"R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 4, 771368ULL},
        PerftParams{"6kq/8/8/8/8/8/8/7K w - - 0 1", 6, 391507ULL},
        PerftParams{"6KQ/8/8/8/8/8/8/7k b - - 0 1", 6, 391507ULL},
        PerftParams{"K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 6, 3370175ULL},
        PerftParams{"6qk/8/8/8/8/8/8/7K b - - 0 1", 6, 419369ULL},
        PerftParams{"6KQ/8/8/8/8/8/8/7k b - - 0 1", 6, 391507ULL},
        PerftParams{"K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 6, 3370175ULL},
        PerftParams{"8/8/8/8/8/K7/P7/k7 w - - 0 1", 6, 6249ULL},
        PerftParams{"8/8/8/8/8/7K/7P/7k w - - 0 1", 6, 6249ULL},
        PerftParams{"K7/p7/k7/8/8/8/8/8 w - - 0 1", 6, 2343ULL},
        PerftParams{"7K/7p/7k/8/8/8/8/8 w - - 0 1", 6, 2343ULL},
        PerftParams{"8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 6, 34834ULL},
        PerftParams{"8/8/8/8/8/K7/P7/k7 b - - 0 1", 6, 2343ULL},
        PerftParams{"8/8/8/8/8/7K/7P/7k b - - 0 1", 6, 2343ULL},
        PerftParams{"K7/p7/k7/8/8/8/8/8 b - - 0 1", 6, 6249ULL},
        PerftParams{"7K/7p/7k/8/8/8/8/8 b - - 0 1", 6, 6249ULL},
        PerftParams{"8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 6, 34822ULL},
        PerftParams{"8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 6, 11848ULL},
        PerftParams{"4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 6, 11848ULL},
        PerftParams{"8/8/7k/7p/7P/7K/8/8 w - - 0 1", 6, 10724ULL},
        PerftParams{"8/8/k7/p7/P7/K7/8/8 w - - 0 1", 6, 10724ULL},
        PerftParams{"8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 6, 53138ULL},
        PerftParams{"8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 6, 157093ULL},
        PerftParams{"8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 6, 158065ULL},
        PerftParams{"k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 6, 20960ULL},
        PerftParams{"8/8/7k/7p/7P/7K/8/8 b - - 0 1", 6, 10724ULL},
        PerftParams{"8/8/k7/p7/P7/K7/8/8 b - - 0 1", 6, 10724ULL},
        PerftParams{"8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 6, 53138ULL},
        PerftParams{"8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 6, 158065ULL},
        PerftParams{"8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 6, 157093ULL},
        PerftParams{"k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 6, 21104ULL},
        PerftParams{"7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 6, 32191ULL},
        PerftParams{"7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 6, 30980ULL},
        PerftParams{"k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 6, 41874ULL},
        PerftParams{"k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 6, 29679ULL},
        PerftParams{"k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 6, 41874ULL},
        PerftParams{"k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 6, 29679ULL},
        PerftParams{"k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 6, 22886ULL},
        PerftParams{"k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 6, 28662ULL},
        PerftParams{"7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 6, 32167ULL},
        PerftParams{"7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 6, 30749ULL},
        PerftParams{"k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 6, 41874ULL},
        PerftParams{"k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 6, 29679ULL},
        PerftParams{"k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 6, 41874ULL},
        PerftParams{"k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 6, 29679ULL},
        PerftParams{"k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 6, 22579ULL},
        PerftParams{"k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 6, 28662ULL},
        PerftParams{"7k/8/8/p7/1P6/8/8/7K w - - 0 1", 6, 41874ULL},
        PerftParams{"7k/8/p7/8/8/1P6/8/7K w - - 0 1", 6, 29679ULL},
        PerftParams{"7k/8/8/1p6/P7/8/8/7K w - - 0 1", 6, 41874ULL},
        PerftParams{"7k/8/1p6/8/8/P7/8/7K w - - 0 1", 6, 29679ULL},
        PerftParams{"k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 6, 55338ULL},
        PerftParams{"k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 6, 55338ULL},
        PerftParams{"3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 6, 199002ULL},
        PerftParams{"7k/8/8/p7/1P6/8/8/7K b - - 0 1", 6, 41874ULL},
        PerftParams{"7k/8/p7/8/8/1P6/8/7K b - - 0 1", 6, 29679ULL},
        PerftParams{"7k/8/8/1p6/P7/8/8/7K b - - 0 1", 6, 41874ULL},
        PerftParams{"7k/8/1p6/8/8/P7/8/7K b - - 0 1", 6, 29679ULL},
        PerftParams{"k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 6, 55338ULL},
        PerftParams{"k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 6, 55338ULL},
        PerftParams{"3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 6, 199002ULL},
        PerftParams{"8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 6, 1030499ULL},
        PerftParams{"n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 5, 2193768ULL},
        PerftParams{"8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 5, 1533145ULL},
        PerftParams{"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 5, 3605103ULL},
        PerftParams{"8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 6, 1030499ULL},
        PerftParams{"n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 5, 2193768ULL},
        PerftParams{"8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 5, 1533145ULL},
        PerftParams{"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 5, 3605103ULL},
        PerftParams{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624ULL},
        // Custom
        PerftParams{"8/PPPPPPPP/3K4/8/8/3k4/pppppppp/8 w - - 0 1", 4, 1994108ULL}
        // clang-format on
        ));

}  // namespace punch::test
