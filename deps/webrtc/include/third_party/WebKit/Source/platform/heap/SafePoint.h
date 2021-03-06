// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SafePoint_h
#define SafePoint_h

#include "platform/heap/ThreadState.h"
#include "platform/wtf/ThreadingPrimitives.h"

namespace blink {

class SafePointScope final {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(SafePointScope);

 public:
  explicit SafePointScope(BlinkGC::StackState stack_state,
                          ThreadState* state = ThreadState::Current())
      : state_(state) {
    if (state_) {
      state_->EnterSafePoint(stack_state, this);
    }
  }

  ~SafePointScope() {
    if (state_)
      state_->LeaveSafePoint();
  }

 private:
  ThreadState* state_;
};

}  // namespace blink

#endif
