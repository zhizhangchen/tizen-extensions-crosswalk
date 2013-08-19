// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "call_history/call_history_context.h"
#include "common/picojson.h"
#include <dlfcn.h>
/*void* create_ecore_main_loop(void*){
  pthread_t ecore_main_thread;
  pthread_create(&ecore_main_thread, NULL,  start_ecore_main_loop, NULL);
  int *status;
  pthread_join(ecore_main_thread, (void**) status);
}*/
CXWalkExtension* xwalk_extension_init(int32_t api_version) {

  printf("xwalk_extension_init returning\n");
  return ExtensionAdapter<CallHistoryContext>::Initialize();
}
static void * jsc_handle;
void load_lib() {
  if (!dlopen("/usr/lib/libaccess-control-bypass.so", RTLD_NOW|RTLD_GLOBAL))
    printf("Error loading libaccess-contorl-bypass.so:%s\n", dlerror());
  else 
    printf("#############Loaded libaccess-check-bypass.so!\n");
  if (!(jsc_handle = dlopen("/usr/lib/libjsc-wrapper.so", RTLD_NOW)))
    printf("Error loading jsc wrapper:%s\n", dlerror());
  else 
    printf("#############Loaded JSC wrapper!\n");
  if (!dlopen("/usr/lib/libwrt-plugin-loading.so", RTLD_NOW))
    printf("Error loading libwrt-plugin-loading.so\n");
  else 
    printf("#############Loaded libwrt-plugin-loading.so!\n");
}

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
void* load_jsc(void*) {
  printf("loading lib###################\n");
  load_lib();
  jsc_init_t init = (jsc_init_t) dlsym(jsc_handle, "init_jsc");
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
      std::cerr << "Cannot load symbol 'init': " << dlsym_error <<
          '\n';
      dlclose(jsc_handle);
  }
  else
    init();
}
const char* CallHistoryContext::GetJavaScript() {
  //pthread_t jsc_thread;
  //pthread_create(&jsc_thread, NULL,  load_jsc, NULL);
  //pthread_detach(jsc_thread);
  load_jsc(NULL);
  return "exports.addChangeListener =  function (listeerCB) {extension.postMessage('tizen.callhistory', {id:1})  }";
}

void CallHistoryContext::HandleMessage(const char* message) {
  picojson::value& v = *new picojson::value();

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();

}
void CallHistoryContext::HandleSyncMessage(const char* message) {
  picojson::value& v = *new picojson::value();

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring sysc message.\n";
    return;
  }

}

