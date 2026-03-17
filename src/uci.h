#ifndef PUNCH_UCI_H_
#define PUNCH_UCI_H_

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include "chess/board.h"
#include "chess/types.h"
#include "search.h"

namespace punch::uci {

void Loop(int argc, char* argv[]);
std::pair<std::string, std::vector<std::string>> ParsePosition(
    std::istringstream& is);
SearchLimits ParseGo(std::istringstream& is);

}  // namespace punch::uci

#endif  // PUNCH_UCI_H_
