#include "search.h"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/types.h"
#include "evaluate.h"
#include "move_picker.h"
#include "time_manager.h"
#include "transposition_table.h"

namespace punch {

Worker::Worker(TranspositionTable& tt) : tt_(tt) {}
Worker::~Worker() = default;

Value Worker::QuiescenceSearch(SearchStack* ss, Value alpha, Value beta) {
  int ply = ss->ply;
  nodes_++;
  seldepth_ = std::max(seldepth_, ply);
  ss->pv_length = 0;

  tm_->CheckTime(*this);

  if (stopped_) {
    return kValueNone;
  }

  // 1. Draw Detection
  if (ply > 0 && board_.IsDraw()) {
    return kValueDraw;
  }

  // 2. TT Probe
  Key key = board_.GetHashKey();
  TtEntry* entry = tt_.Probe(key);
  Move tt_move = Move::None();

  if (entry->key == key) {
    tt_move = entry->move;
    Value tt_score = ValueFromTt(entry->score, ply);

    if (entry->bound == Bound::kExact) {
      return tt_score;
    }
    if (entry->bound == Bound::kLowerBound && tt_score >= beta) {
      return tt_score;
    }
    if (entry->bound == Bound::kUpperBound && tt_score <= alpha) {
      return tt_score;
    }
  }

  // 3. Static Evaluation
  Value static_eval = eval::Evaluate(board_);
  if (static_eval >= beta) {
    return beta;
  }
  alpha = std::max(alpha, static_eval);

  MovePicker<movegen::MoveGenType::kCaptures> picker(board_, ss, tt_move);

  if (picker.NumMoves() == 0) {
    return static_eval;
  }

  Move m;
  Move best_move = Move::None();
  Value best_score = static_eval;

  while ((m = picker.NextMove()) != Move::None()) {
    StateInfo st;
    board_.MakeMove(m, st);

    Value score = -QuiescenceSearch(ss + 1, -beta, -alpha);

    board_.UnmakeMove(m);

    if (stopped_) {
      return kValueNone;
    }

    if (score > best_score) {
      best_move = m;
      best_score = score;

      if (score >= beta) {
        break;
      }
    }
    alpha = std::max(alpha, score);
  }

  Bound bound = (best_score >= beta) ? Bound::kLowerBound : Bound::kUpperBound;
  tt_.Store(key, best_move, 0, best_score, bound, ply);

  return best_score;
}

Value Worker::Negamax(SearchStack* ss, int depth, Value alpha, Value beta) {
  Color us = board_.SideToMove();
  int ply = ss->ply;
  nodes_++;
  seldepth_ = std::max(seldepth_, ply);
  ss->pv_length = 0;

  tm_->CheckTime(*this);

  if (stopped_) {
    return kValueNone;
  }

  // 1. Draw Detection
  if (ply > 0 && board_.IsDraw()) {
    return kValueDraw;
  }

  // 2. TT Probe
  Value original_alpha = alpha;
  Key key = board_.GetHashKey();

  TtEntry* entry = tt_.Probe(key);
  Move tt_move = Move::None();

  if (entry->key == key) {
    tt_move = entry->move;

    if (entry->depth >= depth && ply > 0) {
      Value tt_score = ValueFromTt(entry->score, ply);

      if (entry->bound == Bound::kExact) {
        return tt_score;
      }
      if (entry->bound == Bound::kLowerBound && tt_score >= beta) {
        return tt_score;
      }
      if (entry->bound == Bound::kUpperBound && tt_score <= alpha) {
        return tt_score;
      }
    }
  }

  // 3. Null Move Pruning
  Bitboard non_pawn_materials =
      board_.Us(us) & (~board_.Pieces(PieceType::kPawn, us));
  if (depth >= 3 && !board_.InCheck() && PopCount(non_pawn_materials) > 0) {
    const int R = 2 + depth / 3;

    StateInfo st;
    board_.MakeNullMove(st);

    Value score = -Negamax(ss + 1, depth - 1 - R, -beta, -beta + 1);

    board_.UnmakeNullMove();

    if (stopped_) {
      return kValueNone;
    }

    if (score >= beta) {
      return score;
    }
  }

  // 4. Quiescence Search
  if (depth <= 0) {
    return QuiescenceSearch(ss, alpha, beta);
  }

  depth = std::min(depth, kMaxPly - 1);

  MovePicker<movegen::MoveGenType::kAll> picker(board_, ss, tt_move);

  if (picker.NumMoves() == 0) {
    return board_.InCheck() ? MatedIn(ply) : kValueDraw;
  }

  Move m;
  Move best_move = Move::None();
  Value best_score = -kValueInf;

  while ((m = picker.NextMove()) != Move::None()) {
    StateInfo st;
    board_.MakeMove(m, st);

    Value score = -Negamax(ss + 1, depth - 1, -beta, -alpha);

    board_.UnmakeMove(m);

    if (stopped_) {
      return kValueNone;
    }

    if (score > best_score) {
      best_move = m;
      best_score = score;

      if (score > alpha) {
        alpha = score;

        ss->pv_line[0] = m;
        int next_pv_length = (ss + 1)->pv_length;

        if (next_pv_length > 0) {
          std::copy((ss + 1)->pv_line.begin(),
                    (ss + 1)->pv_line.begin() + next_pv_length,
                    ss->pv_line.begin() + 1);
        }

        ss->pv_length = next_pv_length + 1;

        if (alpha >= beta) {
          if (!board_.IsCapture(m)) {
            if (m != ss->killers[0]) {
              ss->killers[1] = ss->killers[0];
              ss->killers[0] = m;
            }
          }
          break;
        }
      }
    }
  }

  Bound bound = (best_score >= beta)            ? Bound::kLowerBound
                : (best_score > original_alpha) ? Bound::kExact
                                                : Bound::kUpperBound;
  tt_.Store(key, best_move, depth, best_score, bound, ply);

  return best_score;
}

void Worker::IterativeDeepening(int depth_limit) {
  SearchStack ss[kMaxPly + 1];
  for (int ply = 0; ply < kMaxPly; ++ply) {
    ss[ply].ply = ply;
  }

  Move best_move = Move::None();

  Value alpha = -kValueInf;
  Value beta = kValueInf;
  Value delta = 40;

  for (int depth = 1; depth <= depth_limit; ++depth) {
    int fail_count = 0;
    Value score;

    // Aspiration Window
    while (true) {
      score = Negamax(ss, depth, alpha, beta);

      if (stopped_) {
        break;
      }

      if (score <= alpha) {
        beta = (alpha + beta) / 2;
        alpha = std::max(score - delta, -kValueInf);
        delta += delta / 2;
      } else if (score >= delta) {
        alpha = (alpha + beta) / 2;
        beta = std::min(score + delta, kValueInf);
        delta += delta / 2;
      } else {
        delta = 40;
        alpha = std::max(score - delta, -kValueInf);
        beta = std::min(score + delta, kValueInf);
        break;
      }

      if (++fail_count > 3) {
        alpha = -kValueInf;
        beta = kValueInf;
      }
    }

    if (stopped_) {
      break;
    }

    if (ss[0].pv_length > 0) {
      best_move = ss[0].pv_line[0];
    }

    int64_t elapsed_ms = std::max<int64_t>(tm_->ElapsedMs(), 1);

    std::string pv;
    for (int i = 0; i < ss[0].pv_length; ++i) {
      pv += ss[0].pv_line[i].ToString() + " ";
    }
    if (!pv.empty()) {
      pv.pop_back();
    }

    Info info;
    info.depth = depth;
    info.seldepth = seldepth_;
    info.score = score;
    info.nodes = nodes_;
    info.time_ms = elapsed_ms;
    info.nps = (nodes_ * 1000) / elapsed_ms;
    info.hashfull = tt_.Hashfull();
    info.pv = pv;

    std::cout << info << std::endl;
  }

  std::cout << "bestmove " << best_move.ToString() << std::endl;
}

void Worker::Search(const ChessBoard& board, const SearchLimits& limits) {
  board_ = board;
  tm_ = std::make_unique<TimeManager>(std::move(limits), board_.SideToMove());

  nodes_ = 0;
  seldepth_ = 0;
  stopped_ = false;

  IterativeDeepening(limits.depth_limit);
}

}  // namespace punch
