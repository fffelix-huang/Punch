#ifndef PUNCH_TIME_MANAGER_H_
#define PUNCH_TIME_MANAGER_H_

#include <chrono>

#include "chess/types.h"
#include "search.h"

namespace punch {

class TimeManager {
 public:
  TimeManager(const SearchLimits& limits, Color us);

  void CheckTime(Worker& worker);
  int64_t ElapsedMs() const;

 private:
  std::chrono::steady_clock::time_point start_time_;
  int64_t time_limit_ = -1;
  uint64_t nodes_limit_ = 0;
};

}  // namespace punch

#endif  // PUNCH_TIME_MANAGER_H_
