#ifndef PUNCH_CHESS_MOVEGEN_H_
#define PUNCH_CHESS_MOVEGEN_H_

#include "chess/board.h"
#include "utils/static_vector.h"

namespace punch::movegen {

constexpr int kMaxMoves = 256;

enum PieceGenType : uint8_t {
  kGenPawn = 1 << 0,
  kGenKnight = 1 << 1,
  kGenBishop = 1 << 2,
  kGenRook = 1 << 3,
  kGenQueen = 1 << 4,
  kGenKing = 1 << 5,
  kGenAll = 0x3F
};

enum class MoveGenType : uint8_t { kAll, kCaptures, kQuiets };

using MoveList = StaticVector<Move, kMaxMoves>;

/**
 * @brief Precomputes lookup tables required for move generation.
 * This function must be called once at application startup.
 */
void Initialize();

/**
 * @brief Generates all legal moves for the side currently to move.
 * @tparam T The type of moves to generate (defaults to kAll).
 * @param board The current chess board state.
 * @param movelist The destination list for generated legal moves.
 * @param pieces Optional bitmask to restrict generation to specific piece
 * types.
 */
template <MoveGenType T = MoveGenType::kAll>
void GenerateLegalMoves(const ChessBoard& board, MoveList& movelist,
                        PieceGenType pieces = kGenAll);

}  // namespace punch::movegen

#endif  // PUNCH_CHESS_MOVEGEN_H_
