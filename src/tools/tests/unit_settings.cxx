#include <iostream>
#include <string>

#include "settings.h"

int main() {
  bool ok = true;

  ok = ok && (ensure_step_extension("detector") == "detector.stp");
  ok = ok && (ensure_step_extension("detector.stp") == "detector.stp");
  ok = ok && (ensure_step_extension("a.long.name.step") == "a.long.name.step.stp");

  if (!ok) {
    std::cerr << "settings unit checks failed\n";
    return 1;
  }

  return 0;
}
