#include "chess/attacks.h"
#include "chess/movegen.h"
#include "uci.h"

int main(int argc, char* argv[]) {
  punch::attacks::Initialize();
  punch::movegen::Initialize();

  punch::uci::Loop(argc, argv);

  return 0;
}
