// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "call_history/call_history_context.h"
#include "common/picojson.h"
#include <dlfcn.h>
#include "jsc_wrapper.h"
CXWalkExtension* xwalk_extension_init(int32_t api_version) {

  return ExtensionAdapter<CallHistoryContext>::Initialize();
}
static void * jsc_handle;

CallHistoryContext::CallHistoryContext(ContextAPI* api)
  : api_(api) {
  PlatformInitialize();
}

CallHistoryContext::~CallHistoryContext() {
  PlatformUninitialize();
}

const char CallHistoryContext::name[] = "tizen.callhistory";

// This will be generated from call_history_api.js.
extern const char kSource_call_history_api[];

typedef int (*jsc_init_t)();
typedef char* (*add_change_listener_t)(void*);

template <class FuncType>
FuncType loadFunc(const char* funcName) {
  FuncType func = (FuncType) dlsym(jsc_handle, funcName);
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
      std::cerr << "Cannot load symbol 'init': " << dlsym_error <<
          '\n';
      dlclose(jsc_handle);
  }
  return func;
}

void* load_lib(const char* path, int flag) {
  void* handle = NULL;
  if (!(handle = dlopen(path, flag)))
    printf("Error loading %s:%s\n", path, dlerror());
  else 
    printf("#############Loaded %s\n", path);
  return handle;
}

const char* CallHistoryContext::GetJavaScript() {
  load_lib("/usr/lib/libaccess-control-bypass.so", RTLD_NOW|RTLD_GLOBAL);
  jsc_handle = load_lib("/usr/lib/libjsc-wrapper.so", RTLD_NOW);
  jsc_init_t init = loadFunc<jsc_init_t>("init_jsc");
  if (init)
    init();
  return kSource_call_history_api;
}

typedef void* (*handle_msg_t)(ContextAPI* api, const char* msg);
void CallHistoryContext::HandleMessage(const char* message) {
  /*picojson::value& v = *new picojson::value();

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }
  std::string cmd = v.get("cmd").to_str();*/
  handle_msg_t handle_msg = loadFunc<handle_msg_t>("handleMessage");
  handle_msg(api_, message);

}
void CallHistoryContext::HandleSyncMessage(const char* message) {
  picojson::value& v = *new picojson::value();
  printf("HandleSyncMessage\n");

  std::string err;
  /*picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring sysc message.\n";
    return;
  }
  std::string cmd = v.get("cmd").to_str();*/
  typedef void* (*handle_sync_msg_t)(ContextAPI* api, const char* msg);
  handle_msg_t handle_sync_msg = loadFunc<handle_msg_t>("handleSyncMessage");
  handle_sync_msg(api_, message);
  /*if (cmd == "addChangeListener") {
    printf("Calling add_change_listener...\n");
    add_change_listener_t add_change_listener = loadFunc<add_change_listener_t>("add_change_listener");
    if (add_change_listener)
      api_->SetSyncReply(add_change_listener(api_));
  } */

}

