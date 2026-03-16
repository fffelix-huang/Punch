#include "time_manager.h"

#include <chrono>

#include "chess/types.h"
#include "search.h"

namespace punch {

TimeManager::TimeManager(const SearchLimits& limits, Color us) {
  start_time_ = std::chrono::steady_clock::now();

  if (!limits.infinite) {
    int64_t time_remaining =
        (us == Color::kWhite ? limits.wtime : limits.btime);
    if (time_remaining > 0) {
      time_limit_ = time_remaining / 30;
    }
  }

  if (limits.nodes_limit > 0) {
    nodes_limit_ = limits.nodes_limit;
  }
}

void TimeManager::CheckTime(Worker& worker) {
  uint64_t nodes_searched = worker.NodesSearched();

  if (nodes_limit_ > 0 && nodes_searched > nodes_limit_) {
    worker.Stop();
    return;
  }

  if (time_limit_ > 0 && (nodes_searched & 2047) == 0) {
    if (ElapsedMs() >= time_limit_) {
      worker.Stop();
      return;
    }
  }
}

int64_t TimeManager::ElapsedMs() const {
  auto end_time = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                               start_time_)
      .count();
}

}  // namespace punch
