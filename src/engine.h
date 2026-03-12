#ifndef PUNCH_ENGINE_H_
#define PUNCH_ENGINE_H_

#include <deque>
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
  void StartSearch(SearchParams&& params);
  void StopSearch();
  void Quit();

  void Bench(size_t mb = 16);

  const OptionManager& GetOptions() const;
  OptionManager& GetOptions();

 private:
  void JoinThread();
  void PrepareSearch(SearchParams&& params);

  ChessBoard board_;
  SearchInfo search_info_;
  TranspositionTable tt_;
  std::deque<StateInfo> states_pool_;
  std::thread search_thread_;
  OptionManager options_;
};

}  // namespace punch

#endif  // PUNCH_ENGINE_H_
