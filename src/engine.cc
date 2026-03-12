#include "engine.h"

#include <chrono>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

#include "bench.h"
#include "chess/movegen.h"
#include "chess/types.h"
#include "uci_option.h"

namespace punch {

Engine::Engine() {
  search_info_.tt = &tt_;

  NewGame();

  options_.Add(std::make_unique<SpinOption>(
      "Hash", 16, 1, 1024, [this](int mb_size) { tt_.Resize(mb_size); }));
  options_.Add(std::make_unique<ButtonOption>("Clear Hash", [this]() {
    tt_.Clear();
    search_info_.age = 0;
  }));
}

Engine::~Engine() { JoinThread(); }

void Engine::JoinThread() {
  search_info_.stopped = true;
  if (search_thread_.joinable()) {
    search_thread_.join();
  }
}

void Engine::NewGame() {
  JoinThread();
  states_pool_.clear();
  board_.LoadFen(kInitialFen);
  tt_.Clear();
  search_info_.age = 0;
}

void Engine::SetPosition(std::string_view fen,
                         std::span<const std::string> move_strs) {
  JoinThread();
  board_.LoadFen(fen);
  states_pool_.clear();

  auto string_to_move = [](const ChessBoard& board,
                           std::string_view str) -> Move {
    movegen::MoveList moves;
    movegen::GenerateLegalMoves<movegen::MoveGenType::kAll>(board, moves);

    for (Move m : moves) {
      if (m.ToString() == str) {
        return m;
      }
    }

    return Move::None();
  };

  for (std::string_view move_str : move_strs) {
    Move m = string_to_move(board_, move_str);
    if (m.IsOk()) {
      states_pool_.emplace_back();
      board_.MakeMove(m, states_pool_.back());
    }
  }
}

void Engine::PrepareSearch(SearchParams&& params) {
  search_info_.Apply(std::move(params));
  search_info_.age++;
  search_info_.Reset();
}

void Engine::StartSearch(SearchParams&& params) {
  JoinThread();
  PrepareSearch(std::move(params));
  search_thread_ =
      std::thread(Search, std::ref(board_), std::ref(search_info_));
}

void Engine::StopSearch() { JoinThread(); }

void Engine::Quit() { JoinThread(); }

void Engine::Bench(size_t mb) {
  constexpr int kBenchDepth = 6;

  JoinThread();

  tt_.Resize(mb);
  tt_.Clear();
  search_info_.age = 0;

  const size_t num_positions = bench::kBenchPositions.size();
  uint64_t total_nodes = 0;
  auto start_time = std::chrono::steady_clock::now();

  for (size_t i = 0; i < num_positions; ++i) {
    std::string_view fen = bench::kBenchPositions[i];
    std::cout << "Position: " << (i + 1) << "/" << num_positions << " (" << fen
              << ")" << std::endl;

    SetPosition(fen, {});
    board_.LoadFen(fen);
    states_pool_.clear();

    PrepareSearch(SearchParams{.depth_limit = kBenchDepth});
    Search(board_, search_info_);

    total_nodes += search_info_.nodes;

    std::cout << std::endl;
  }

  auto end_time = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     end_time - start_time)
                     .count();

  std::cout << "===========================\n";
  std::cout << "Total time (ms) : " << elapsed << "\n";
  std::cout << "Nodes searched  : " << total_nodes << "\n";
  std::cout << "Nodes/second    : " << (total_nodes * 1000) / (elapsed + 1)
            << std::endl;
}

const OptionManager& Engine::GetOptions() const { return options_; }
OptionManager& Engine::GetOptions() { return options_; }

}  // namespace punch
