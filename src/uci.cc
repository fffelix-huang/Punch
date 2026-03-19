#include "uci.h"

#include <deque>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "chess/board.h"
#include "chess/movegen.h"
#include "engine.h"
#include "search.h"
#include "version.h"

namespace punch::uci {

void Loop(int argc, char* argv[]) {
  Engine engine;

  if (argc > 1 && std::string_view(argv[1]) == "bench") {
    engine.Bench();
    return;
  }

  std::string line, token;

  while (std::getline(std::cin, line)) {
    if (line.empty()) {
      continue;
    }

    std::istringstream is(line);
    token.clear();
    is >> std::skipws >> token;

    if (token == "uci") {
      std::cout << "id name Punch " << GetVersionString() << "\n";
      std::cout << "id author Ting-Hsuan Huang\n";
      std::cout << engine.GetOptions() << "\n";
      std::cout << "uciok" << std::endl;
    } else if (token == "isready") {
      std::cout << "readyok" << std::endl;
    } else if (token == "bench") {
      engine.Bench();
    } else if (token == "ucinewgame") {
      engine.NewGame();
    } else if (token == "position") {
      auto [fen, move_strs] = ParsePosition(is);
      engine.SetPosition(fen, move_strs);
    } else if (token == "go") {
      engine.StopSearch();
      engine.StartSearch(ParseGo(is));
    } else if (token == "stop") {
      engine.StopSearch();
    } else if (token == "quit") {
      engine.Quit();
      break;
    } else if (token == "setoption") {
      std::string token, name, value;

      is >> token;  // "name"

      while (is >> token && token != "value") {
        name += (name.empty() ? "" : " ") + token;
      }

      while (is >> token) {
        value += (value.empty() ? "" : " ") + token;
      }

      if (!engine.GetOptions().Set(name, value)) {
        std::cout << "No such option: " << name << std::endl;
      }
    } else if (token == "d") {
      std::cout << engine.Visualize() << std::endl;
    }
  }
}

std::pair<std::string, std::vector<std::string>> ParsePosition(
    std::istringstream& is) {
  std::string token, fen;
  is >> token;

  if (token == "startpos") {
    fen = kInitialFen;
    is >> token;
  } else if (token == "fen") {
    while (is >> token && token != "moves") {
      fen += token + " ";
    }
  }

  std::vector<std::string> move_strs;
  std::string move_str;

  while (is >> move_str) {
    move_strs.push_back(move_str);
  }

  return {fen, move_strs};
}

SearchLimits ParseGo(std::istringstream& is) {
  std::string token;
  SearchLimits limits;

  while (is >> token) {
    if (token == "wtime") {
      is >> limits.wtime;
    } else if (token == "btime") {
      is >> limits.btime;
    } else if (token == "winc") {
      is >> limits.winc;
    } else if (token == "binc") {
      is >> limits.binc;
    } else if (token == "movestogo") {
      is >> limits.movestogo;
    } else if (token == "depth") {
      is >> limits.depth_limit;
    } else if (token == "nodes") {
      is >> limits.nodes_limit;
    } else if (token == "movetime") {
      is >> limits.movetime;
    } else if (token == "infinite") {
      limits.infinite = true;
    }
  }

  return limits;
}

}  // namespace punch::uci
