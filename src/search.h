#ifndef PUNCH_SEARCH_H_
#define PUNCH_SEARCH_H_

#include <atomic>
#include <chrono>
#include <deque>
#include <iosfwd>

#include "chess/board.h"
#include "chess/types.h"
#include "transposition_table.h"

namespace punch {

struct SearchStack {
  int ply;

  std::array<Move, kMaxPly> pv_line;
  int pv_length;
};

struct SearchLimits {
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

struct Info {
  int depth;
  int seldepth;
  Value score;
  uint64_t nodes;
  int nps;
  int hashfull;
  int time_ms;
  std::string_view pv;

  friend std::ostream& operator<<(std::ostream& os, const Info& info) {
    os << "info depth " << info.depth << " seldepth " << info.seldepth
       << " score " << ValueToString(info.score) << " nodes " << info.nodes
       << " hashfull " << info.hashfull << " time " << info.time_ms;

    if (!info.pv.empty()) {
      os << " pv " << info.pv;
    }

    return os;
  }
};

class TimeManager;

class Worker {
 public:
  Worker(TranspositionTable& tt);
  ~Worker();

  inline void Stop() { stopped_.store(true, std::memory_order_relaxed); }

  void Search(const ChessBoard& board, const SearchLimits& limits);
  void IterativeDeepening(int depth_limit = kMaxPly);
  Value Negamax(SearchStack* ss, int depth, Value alpha, Value beta);
  Value QuiescenceSearch(SearchStack* ss, Value alpha, Value beta);

  constexpr uint64_t NodesSearched() const { return nodes_; }

 private:
  ChessBoard board_;
  TranspositionTable& tt_;
  SearchLimits limits_;
  std::unique_ptr<TimeManager> tm_;

  uint64_t nodes_;
  int seldepth_;
  std::atomic_bool stopped_{false};
};

}  // namespace punch

#endif  // PUNCH_SEARCH_H_
