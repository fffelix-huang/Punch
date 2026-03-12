#include "search.h"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "chess/board.h"
#include "chess/movegen.h"
#include "chess/types.h"
#include "evaluate.h"
#include "movepicker.h"
#include "transposition_table.h"

namespace punch {

std::ostream& operator<<(std::ostream& os, const SearchInfo& info) {
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now - info.start_time)
                     .count();

  os << "info depth " << info.depth << " seldepth " << info.seldepth
     << " score " << ValueToString(info.score) << " nodes " << info.nodes
     << " nps " << (info.nodes * 1000 / (elapsed + 1)) << " hashfull "
     << info.tt->Hashfull(info.age) << " time " << elapsed;

  if (info.pv_length[0] > 0) {
    os << " pv";
    for (int i = 0; i < info.pv_length[0]; ++i) {
      os << " " << info.pv_table[0][i].ToString();
    }
  }

  return os;
}

Value QuiescenceSearch(ChessBoard& board, SearchInfo& search_info, Value alpha,
                       Value beta) {
  int ply = search_info.ply;
  search_info.nodes++;
  search_info.seldepth = std::max(search_info.seldepth, search_info.ply);
  search_info.pv_length[ply] = 0;
  search_info.UpdateStatus();

  if (search_info.stopped) {
    return kValueNone;
  }

  // 1. Draw Detection
  if (ply > 0 && board.IsDraw()) {
    return kValueDraw;
  }

  // 2. TT Probe
  Key key = board.GetHashKey();
  TtEntry* entry = search_info.tt->Probe(key);
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
  Value static_eval = eval::Evaluate(board);
  if (static_eval >= beta) {
    return beta;
  }
  alpha = std::max(alpha, static_eval);

  MovePicker<movegen::MoveGenType::kCaptures> picker(board, tt_move);

  if (picker.NumMoves() == 0) {
    return static_eval;
  }

  Move m;
  Move best_move = Move::None();
  Value best_score = static_eval;

  while ((m = picker.NextMove()) != Move::None()) {
    StateInfo st;
    board.MakeMove(m, st);
    search_info.ply++;

    Value score = -QuiescenceSearch(board, search_info, -beta, -alpha);

    search_info.ply--;
    board.UnmakeMove(m);

    if (search_info.stopped) {
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
  search_info.tt->Store(key, best_move, 0, best_score, bound, search_info.age,
                        ply);

  return best_score;
}

Value Negamax(ChessBoard& board, SearchInfo& search_info, int depth,
              Value alpha, Value beta) {
  int ply = search_info.ply;
  search_info.nodes++;
  search_info.seldepth = std::max(search_info.seldepth, ply);
  search_info.pv_length[ply] = 0;
  search_info.UpdateStatus();

  if (search_info.stopped) {
    return kValueNone;
  }

  // 1. Draw Detection
  if (ply > 0 && board.IsDraw()) {
    return kValueDraw;
  }

  // 2. TT Probe
  Value original_alpha = alpha;
  Key key = board.GetHashKey();

  TtEntry* entry = search_info.tt->Probe(key);
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

  // 3. Quiescence Search
  if (depth <= 0) {
    return QuiescenceSearch(board, search_info, alpha, beta);
  }

  depth = std::min(depth, kMaxPly - 1);

  MovePicker<movegen::MoveGenType::kAll> picker(board, tt_move);

  if (picker.NumMoves() == 0) {
    return board.InCheck() ? MatedIn(ply) : kValueDraw;
  }

  Move m;
  Move best_move = Move::None();
  Value best_score = -kValueInf;

  while ((m = picker.NextMove()) != Move::None()) {
    StateInfo st;
    board.MakeMove(m, st);
    search_info.ply++;

    Value score = -Negamax(board, search_info, depth - 1, -beta, -alpha);

    search_info.ply--;
    board.UnmakeMove(m);

    if (search_info.stopped) {
      return kValueNone;
    }

    if (score > best_score) {
      best_move = m;
      best_score = score;

      if (score > alpha) {
        alpha = score;

        search_info.pv_table[ply][ply] = m;
        int next_ply = ply + 1;
        int next_pv_length = search_info.pv_length[next_ply];

        if (next_pv_length > 0) {
          std::copy(search_info.pv_table[next_ply] + next_ply,
                    search_info.pv_table[next_ply] + next_ply + next_pv_length,
                    search_info.pv_table[ply] + ply + 1);
        }

        search_info.pv_length[ply] = next_pv_length + 1;

        if (alpha >= beta) {
          break;
        }
      }
    }
  }

  Bound bound = (best_score >= beta)            ? Bound::kLowerBound
                : (best_score > original_alpha) ? Bound::kExact
                                                : Bound::kUpperBound;
  search_info.tt->Store(key, best_move, depth, best_score, bound,
                        search_info.age, ply);

  return best_score;
}

void Search(ChessBoard& board, SearchInfo& search_info) {
  search_info.Reset();

  if (!search_info.params.infinite) {
    int64_t time_remaining =
        (board.SideToMove() == Color::kWhite ? search_info.params.wtime
                                             : search_info.params.btime);
    if (time_remaining != -1) {
      search_info.params.time_limit = time_remaining / 30;
    }
  }

  Move best_move = Move::None();

  for (int depth = 1; depth <= search_info.params.depth_limit; ++depth) {
    Value score = Negamax(board, search_info, depth, -kValueInf, kValueInf);

    if (search_info.stopped) {
      break;
    }

    search_info.depth = depth;
    search_info.score = score;
    best_move = search_info.pv_table[0][0];

    std::cout << search_info << std::endl;
  }

  std::cout << "bestmove " << best_move.ToString() << std::endl;
}

}  // namespace punch
