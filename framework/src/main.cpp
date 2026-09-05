#include "mlxgym/metal_context.hpp"
#include "mlxgym/test_support.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#ifndef MLXGYM_TASK_NAME
#error MLXGYM_TASK_NAME must be defined
#endif
#ifndef MLXGYM_METALLIB_PATH
#error MLXGYM_METALLIB_PATH must be defined
#endif

int main(int argc, char **argv) {
  bool benchmark = false;
  std::string capture_path;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--benchmark")
      benchmark = true;
    else if (arg == "--capture" && i + 1 < argc)
      capture_path = argv[++i];
    else {
      std::cerr << "usage: " << argv[0] << " [--benchmark] [--capture PATH.gputrace]\n";
      return 2;
    }
  }

  try {
    mlxgym::MetalContext context;
    auto pipeline = context.load_pipeline(MLXGYM_METALLIB_PATH, "solve");
    if (!capture_path.empty())
      context.begin_capture(capture_path);
    const int result = mlxgym::run_task(MLXGYM_TASK_NAME, context, pipeline, benchmark);
    if (!capture_path.empty())
      context.end_capture();
    return result;
  } catch (const std::exception &error) {
    std::cerr << MLXGYM_TASK_NAME << ": " << error.what() << '\n';
    return 1;
  }
}
