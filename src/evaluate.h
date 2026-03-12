#ifndef PUNCH_EVALUATE_H_
#define PUNCH_EVALUATE_H_

#include "chess/board.h"
#include "chess/types.h"

namespace punch::eval {

Value Evaluate(const ChessBoard& board);

}  // namespace punch::eval

#endif  // PUNCH_EVALUATE_H_
