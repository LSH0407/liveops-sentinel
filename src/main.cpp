#include "app/App.h"
#include <spdlog/spdlog.h>
#include <iostream>
namespace ui { int RunApp(); }
int run_console(); // 있으면 호출

int main() {
#ifdef ENABLE_GUI
  return ui::RunApp();
#else
  std::cout << "LiveOps Sentinel Console Mode\n";
  if (run_console) return run_console();
  return 0;
#endif
}
