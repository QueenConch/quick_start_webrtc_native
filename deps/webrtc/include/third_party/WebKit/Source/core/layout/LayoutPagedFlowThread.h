// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutPagedFlowThread_h
#define LayoutPagedFlowThread_h

#include "core/layout/LayoutMultiColumnFlowThread.h"

namespace blink {

// A flow thread for paged overflow. FIXME: The current implementation relies on
// the multicol implementation, but it in the long run it would be better to
// have what's common between LayoutMultiColumnFlowThread and
// LayoutPagedFlowThread in LayoutFlowThread, and have both of them inherit
// from that one.
class LayoutPagedFlowThread : public LayoutMultiColumnFlowThread {
 public:
  static LayoutPagedFlowThread* CreateAnonymous(
      Document&,
      const ComputedStyle& parent_style);

  LayoutBlockFlow* PagedBlockFlow() const {
    return ToLayoutBlockFlow(Parent());
  }

  // Return the number of pages. Will never be less than 1.
  int PageCount();

  bool IsLayoutPagedFlowThread() const override { return true; }
  const char* GetName() const override { return "LayoutPagedFlowThread"; }
  bool NeedsNewWidth() const override;
  void UpdateLogicalWidth() override;
  void UpdateLayout() override;

 private:
  bool DescendantIsValidColumnSpanner(
      LayoutObject* /*descendant*/) const override {
    return false;
  }
};

}  // namespace blink

#endif  // LayoutPagedFlowThread_h
