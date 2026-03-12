#include "chess/attacks.h"
#include "chess/movegen.h"
#include "uci.h"

int main() {
  punch::attacks::Initialize();
  punch::movegen::Initialize();
  punch::uci::Loop();
  return 0;
}
