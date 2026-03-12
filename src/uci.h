#ifndef PUNCH_UCI_H_
#define PUNCH_UCI_H_

#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "chess/board.h"
#include "chess/types.h"
#include "search.h"

namespace punch::uci {

void Loop();
std::pair<std::string, std::vector<std::string>> ParsePosition(
    std::istringstream& is);
SearchParams ParseGo(std::istringstream& is);
Move ParseMove(const ChessBoard& board, std::string_view move_str);

}  // namespace punch::uci

#endif  // PUNCH_UCI_H_
