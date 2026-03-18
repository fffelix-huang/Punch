#include "engine.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "bench.h"
#include "chess/movegen.h"
#include "chess/types.h"
#include "uci_option.h"

namespace punch {

Engine::Engine() {
  worker_ = std::make_unique<Worker>(tt_);

  board_.LoadFen(kInitialFen);

  options_.Add(std::make_unique<SpinOption>("Threads", 1, 1, 1));
  options_.Add(std::make_unique<SpinOption>(
      "Hash", 16, 1, 1024, [this](int mb_size) { tt_.Resize(mb_size); }));
  options_.Add(
      std::make_unique<ButtonOption>("Clear Hash", [this]() { tt_.Clear(); }));
}

Engine::~Engine() { JoinThread(); }

void Engine::JoinThread() {
  worker_->Stop();
  WaitForSearch();
}

void Engine::NewGame() {
  JoinThread();
  states_pool_.clear();
  board_.LoadFen(kInitialFen);
  tt_.Clear();
  worker_->Clear();
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

void Engine::StartSearch(SearchLimits limits) {
  JoinThread();
  search_thread_ = std::thread(&Worker::Search, worker_.get(), board_, limits);
}

void Engine::StopSearch() { JoinThread(); }

void Engine::Quit() { JoinThread(); }

void Engine::WaitForSearch() {
  if (search_thread_.joinable()) {
    search_thread_.join();
  }
}

void Engine::Bench(size_t mb) {
  constexpr int kBenchDepth = 7;

  JoinThread();

  tt_.Resize(mb);
  tt_.Clear();

  const size_t num_positions = bench::kBenchPositions.size();
  uint64_t total_nodes = 0;
  auto start_time = std::chrono::steady_clock::now();

  for (size_t i = 0; i < num_positions; ++i) {
    std::string_view fen = bench::kBenchPositions[i];
    std::cout << "Position: " << (i + 1) << "/" << num_positions << " (" << fen
              << ")" << std::endl;

    board_.LoadFen(fen);
    tt_.NewSearch();

    StartSearch(SearchLimits{.depth_limit = kBenchDepth});
    WaitForSearch();

    total_nodes += worker_->NodesSearched();

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
