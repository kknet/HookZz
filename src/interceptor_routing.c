#include "interceptor_routing.h"
#include "thread_support/thread_stack.h"

void interceptor_routing_begin(RegisterContext *reg_ctx, hook_entry_t *entry, void *next_hop_addr_PTR,
                               void *ret_addr_PTR) {
  // DEBUG_LOG("target %p call begin-invocation", entry->target_ptr);

  thread_stack_manager_t *thread_stack_manager = thread_stack_cclass(shared_instance)();
  call_stack_t *call_stack                     = call_stack_cclass(new)(thread_stack_manager);
  thread_stack_cclass(push_call_stack)(thread_stack_manager, call_stack);

  // call pre_call
  if (entry->pre_call) {
    PRECALL pre_call;
    HookEntryInfo entryInfo;
    entryInfo.hook_id        = entry->id;
    entryInfo.target_address = entry->target_address;
    pre_call                 = entry->pre_call;
    ThreadStackPublic tsp    = {thread_stack_manager->thread_id, thread_stack_manager->call_stacks->len};
    CallStackPublic csp      = {call_stack->call_id};
    (*pre_call)(reg_ctx, &tsp, &csp, &entryInfo);
  }

  // set next hop
  if (entry->replace_call) {
    *(zz_ptr_t *)next_hop_addr_PTR = entry->replace_call;
  } else {
    *(zz_ptr_t *)next_hop_addr_PTR = entry->on_invoke_trampoline;
  }

  if (entry->type == HOOK_TYPE_FUNCTION_via_PRE_POST || entry->type == HOOK_TYPE_FUNCTION_via_GOT) {
    call_stack->ret_addr      = *(zz_ptr_t *)ret_addr_PTR;
    *(zz_ptr_t *)ret_addr_PTR = entry->on_leave_trampoline;
  }
}

void interceptor_routing_end(RegisterContext *reg_ctx, hook_entry_t *entry, void *next_hop_addr_PTR) {
  // DEBUG_LOG("%p call end-invocation", entry->target_ptr);

  thread_stack_manager_t *thread_stack_manager = thread_stack_cclass(shared_instance)();

  call_stack_t *call_stack = thread_stack_cclass(pop_call_stack)(thread_stack_manager);

  // call post_call
  if (entry->post_call) {
    POSTCALL post_call;
    HookEntryInfo entryInfo;
    entryInfo.hook_id        = entry->id;
    entryInfo.target_address = entry->target_address;
    post_call                = entry->post_call;
    ThreadStackPublic tsp    = {thread_stack_manager->thread_id, thread_stack_manager->call_stacks->len};
    CallStackPublic csp      = {call_stack->call_id};
    (*post_call)(reg_ctx, &tsp, &csp, (const HookEntryInfo *)&entryInfo);
  }

  // set next hop
  *(zz_ptr_t *)next_hop_addr_PTR = call_stack->ret_addr;

  call_stack_cclass(destory)(call_stack);
}

void interceptor_routing_dynamic_binary_instrumentation(RegisterContext *reg_ctx, hook_entry_t *entry,
                                                        void *next_hop_addr_PTR) {
  // DEBUG_LOG("target %p call dynamic-binary-instrumentation-invocation", entry->target_ptr);

  if (entry->dbi_call) {
    DBICALL dbi_call;
    HookEntryInfo entryInfo;
    entryInfo.hook_id        = entry->id;
    entryInfo.target_address = entry->target_address;
    dbi_call                 = entry->dbi_call;
    (*dbi_call)(reg_ctx, (const HookEntryInfo *)&entryInfo);
  }

  *(zz_ptr_t *)next_hop_addr_PTR = entry->on_invoke_trampoline;
}

void interceptor_routing_begin_bridge_handler(RegisterContext *reg_ctx, ClosureTrampolineEntry *entry) {
  hook_entry_t *entry     = entry->carry_data;
  void *next_hop_addr_PTR = get_next_hop_addr_PTR(reg_ctx);
  void *ret_addr_PTR      = get_ret_addr_PTR(reg_ctx);
  interceptor_routing_begin(reg_ctx, entry, next_hop_addr_PTR, ret_addr_PTR);
  return;
}

void interceptor_routing_end_bridge_handler(RegisterContext *reg_ctx, ClosureTrampolineEntry *entry) {
  hook_entry_t *entry     = entry->carry_data;
  void *next_hop_addr_PTR = get_next_hop_addr_PTR(reg_ctx);
  interceptor_routing_end(reg_ctx, entry, next_hop_addr_PTR);
  return;
}

void interceptor_routing_dynamic_binary_instrumentation_bridge_handler(RegisterContext *reg_ctx,
                                                                       ClosureTrampolineEntry *entry) {
  hook_entry_t *entry     = entry->carry_data;
  void *next_hop_addr_PTR = get_next_hop_addr_PTR(reg_ctx);
  interceptor_routing_dynamic_binary_instrumentation(reg_ctx, entry, next_hop_addr_PTR);
  return;
}

void interceptor_routing_common_bridge_handler(RegisterContext *reg_ctx, ClosureTrampolineEntry *entry) {
  USER_CODE_CALL userCodeCall = entry->forward_code;

  // TODO: package as a function `beautiful_stack()`
  uintptr_t fp_reg;
  fp_reg                     = (uintptr_t)get_current_fp_reg();
  uintptr_t *none_symbol_PTR = (uintptr_t *)fp_reg + 1;
  uintptr_t none_symbol      = *none_symbol_PTR;
  uintptr_t *ret_addr_PTR    = get_ret_addr_PTR(reg_ctx);
  uintptr_t ret_addr         = *ret_addr_PTR;
  *none_symbol_PTR           = ret_addr;

  userCodeCall(reg_ctx, entry);
  *none_symbol_PTR = none_symbol;
  return;
}

#if DYNAMIC_CLOSURE_BRIDGE
void interceptor_routing_begin_dynamic_bridge_handler(RegisterContext *reg_ctx, DynamicClosureTrampoline *dcb_info) {
  hook_entry_t *entry     = dcb_info->carry_data;
  void *next_hop_addr_PTR = get_next_hop_addr_PTR(reg_ctx);
  void *ret_addr_PTR      = get_ret_addr_PTR(reg_ctx);
  interceptor_routing_begin(reg_ctx, entry, next_hop_addr_PTR, ret_addr_PTR);
  return;
}

void interceptor_routing_end_dynamic_bridge_handler(RegisterContext *reg_ctx, DynamicClosureTrampoline *dcb_info) {
  hook_entry_t *entry     = dcb_info->carry_data;
  void *next_hop_addr_PTR = get_next_hop_addr_PTR(reg_ctx);
  interceptor_routing_end(reg_ctx, entry, next_hop_addr_PTR);
  return;
}

void interceptor_routing_dynamic_common_bridge_handler(RegisterContext *reg_ctx, DynamicClosureTrampoline *dcb_info) {
  DYNAMIC_USER_CODE_CALL userCodeCall = dcb_info->forward_code;
  userCodeCall(reg_ctx, dcb_info);
  return;
}
#endif
