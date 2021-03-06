//
// Created by z on 2018/4/7.
//

#ifndef CUSTOM_BRIDGE_HANDLER_H
#define CUSTOM_BRIDGE_HANDLER_H

#include "closure-bridge-x86.h"
#include "hookzz.h"
#include "interceptor.h"
#include "zkit.h"

void context_end_invocation_bridge_handler(RegisterContext *reg_ctx, ClosureTrampolineEntry *entry);
void context_begin_invocation_bridge_handler(RegisterContext *reg_ctx, ClosureTrampolineEntry *entry);
void dynamic_binary_instrumentationn_bridge_handler(RegisterContext *reg_ctx, ClosureTrampolineEntry *entry);

#endif //CUSTOM_BRIDGE_HANDLER_H
