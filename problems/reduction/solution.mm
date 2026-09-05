#include "mlxgym/metal_context.hpp"

namespace mlxgym {

// This file contains dispatch plumbing only. For a multipass solution, add
// additional entry points to kernel.metal and encode the extra passes here.
DispatchTiming encode_solution(MetalContext &context, PipelineHandle pipeline,
                               const Invocation &invocation) {
  return context.dispatch(pipeline, invocation);
}

} // namespace mlxgym
