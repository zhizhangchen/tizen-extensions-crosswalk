// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CALL_HISTORY_CONTEXT_H_
#define CALL_HISTORY_CONTEXT_H_

#include "common/extension_adapter.h"
#include <map>
#include <string>

#if defined(GENERIC_DESKTOP)
#include <gio/gio.h>
#endif

namespace picojson {
class value;
}

class CallHistoryContext {
 public:
  CallHistoryContext(ContextAPI* api);
  ~CallHistoryContext();

  void PlatformInitialize();
  void PlatformUninitialize();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

private:
  ContextAPI* api_;

#if defined(GENERIC_DESKTOP)
#endif
};

#endif  // CALL_HISTORY_CONTEXT_H_
