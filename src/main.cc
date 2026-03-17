#include <string_view>

#include "chess/attacks.h"
#include "chess/movegen.h"
#include "engine.h"
#include "uci.h"

int main(int argc, char* argv[]) {
  punch::attacks::Initialize();
  punch::movegen::Initialize();

  if (argc > 1 && std::string_view(argv[1]) == "bench") {
    punch::Engine engine;
    engine.Bench();
    return 0;
  }

  punch::uci::Loop();

  return 0;
}
