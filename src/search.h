#ifndef PUNCH_SEARCH_H_
#define PUNCH_SEARCH_H_

#include <atomic>
#include <chrono>

#include "chess/board.h"
#include "chess/types.h"
#include "transposition_table.h"

namespace punch {

struct SearchParams {
  int64_t wtime = -1;
  int64_t btime = -1;
  int64_t winc = 0;
  int64_t binc = 0;
  int64_t movestogo = 0;
  int64_t movetime = -1;
  int depth_limit = kMaxPly;
  uint64_t nodes_limit = 0;
  int64_t time_limit = -1;
  bool infinite = false;
};

struct SearchInfo {
  SearchParams params;
  TranspositionTable* tt;
  uint8_t age = 0;

  uint64_t nodes = 0;
  int ply = 0;
  int depth = 0;
  int seldepth = 0;
  Value score = 0;
  Move pv_table[kMaxPly][kMaxPly];
  int pv_length[kMaxPly];

  std::chrono::steady_clock::time_point start_time;
  std::atomic_bool stopped{false};

  void Apply(SearchParams&& search_params) {
    params = std::move(search_params);
  }

  void Reset() {
    start_time = std::chrono::steady_clock::now();
    nodes = 0;
    depth = 0;
    seldepth = 0;
    score = 0;
    stopped = false;

    for (int i = 0; i < kMaxPly; ++i) {
      pv_length[i] = 0;
      for (int j = 0; j < kMaxPly; ++j) {
        pv_table[i][j] = Move::None();
      }
    }
  }

  void UpdateStatus() {
    if (params.nodes_limit > 0 && nodes >= params.nodes_limit) {
      stopped = true;
    }
    if (!params.infinite && params.time_limit != -1 && (nodes & 2047) == 0) {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - start_time)
                         .count();
      if (elapsed >= params.time_limit) {
        stopped = true;
      }
    }
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const SearchInfo& search_info);
};

Value QuiescenceSearch(ChessBoard& board, SearchInfo& search_info, Value alpha,
                       Value beta);
Value Negamax(ChessBoard& board, SearchInfo& search_info, int depth,
              Value alpha, Value beta);

void Search(ChessBoard& board, SearchInfo& search_info);

}  // namespace punch

#endif  // PUNCH_SEARCH_H_
