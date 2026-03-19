#ifndef PUNCH_ENGINE_H_
#define PUNCH_ENGINE_H_

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <thread>

#include "chess/board.h"
#include "chess/types.h"
#include "search.h"
#include "transposition_table.h"
#include "uci_option.h"

namespace punch {

class Engine {
 public:
  Engine();
  ~Engine();

  void NewGame();
  void SetPosition(std::string_view fen,
                   std::span<const std::string> move_strs);
  void StartSearch(SearchLimits limits);
  void StopSearch();
  void WaitForSearch();
  void Quit();

  void Bench(size_t mb = 16);
  std::string Visualize() const;

  const OptionManager& GetOptions() const;
  OptionManager& GetOptions();

 private:
  void JoinThread();

  ChessBoard board_;
  std::deque<StateInfo> states_pool_;
  TranspositionTable tt_;
  OptionManager options_;
  std::thread search_thread_;
  std::unique_ptr<Worker> worker_;
};

}  // namespace punch

#endif  // PUNCH_ENGINE_H_
