// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "call_history/call_history_context.h"
#include "common/picojson.h"
#include "plugin.h"
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/JSObjectRef.h>
#include "javascript_interface.h"
#include "dpl/foreach.h"
#include <dpl/scoped_array.h>
#include <dlfcn.h>
#include <Commons/WrtAccess/WrtAccess.h>
CXWalkExtension* xwalk_extension_init(int32_t api_version) {

  return ExtensionAdapter<CallHistoryContext>::Initialize();
}
static void _init() __attribute__((constructor));
void _init() {
  printf("############################Loading call history extension.\n");
  if (!dlopen("/usr/lib/libjsc-wrapper.so", RTLD_NOW|RTLD_GLOBAL))
    printf("Error loading jsc wrapper\n");
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
std::string toString(const JSStringRef& arg)
{
    Assert(arg);
    std::string result;
    size_t jsSize = JSStringGetMaximumUTF8CStringSize(arg);
    if (jsSize > 0) {
        ++jsSize;
        DPL::ScopedArray<char> buffer(new char[jsSize]);
        size_t written = JSStringGetUTF8CString(arg, buffer.Get(), jsSize);
        if (written > jsSize) {
            LogError("Conversion could not be fully performed.");
            return std::string();
        }
        result = buffer.Get();
    }

    return result;
}

/**
 * Converts JSValueRef to std::string
 * */
std::string toString(JSContextRef ctx, JSValueRef value) {
  Assert(ctx && value);
  std::string result;
  JSStringRef str = JSValueToStringCopy(ctx, value, NULL);
  result = toString(str);
  JSStringRelease(str);
  return result;
}


// This will be generated from call_history_api.js.
extern const char kSource_call_history_api[];

const char* CallHistoryContext::GetJavaScript() {
  printf("loading callhistory lib\n");
  PluginPtr plugin = Plugin::LoadFromFile("/usr/lib/wrt-plugins/tizen-callhistory/libwrt-plugins-tizen-callhistory.so");
  plugin->OnWidgetStart(0);
  WrtDeviceApis::Commons::AceFunction func;
  if (!(WrtDeviceApis::Commons::WrtAccessSingleton::Instance().checkAccessControl(func))) {
    printf("Function is not allowed to run\n");
  }       
  printf("loaded callhistory lib\n");
  Plugin::ClassPtrList list = plugin->GetClassList();
  printf("got callhistory lib\n");
  JSGlobalContextRef gContext = JSGlobalContextCreateInGroup(NULL, NULL);
  for (std::list<Plugin::ClassPtr>::iterator it = list->begin(); it != list->end(); it ++) {
    printf("calling class template of %s\n", (*it)->getName().c_str());
    //JSClassRef classRef = static_cast<JSClassRef>(const_cast<JSObjectDeclaration::ClassTemplate>((*it)->getClassTemplate()));
    JSObjectPtr objectInstance = JavaScriptInterfaceSingleton::Instance().
              createObject(gContext, *it);
    JavaScriptInterface::PropertiesList list  = JavaScriptInterfaceSingleton::Instance().getObjectPropertiesList(gContext, objectInstance);
    FOREACH(it, list) printf("property: %s\n", it->c_str());
    JSObjectPtr functionObject = JavaScriptInterfaceSingleton::Instance().getJSObjectProperty(gContext, objectInstance, "addChangeListener");

    JSValueRef  arguments[1];
    JSValueRef result;
    result = JSObjectCallAsFunction(gContext, static_cast<JSObjectRef>(functionObject->getObject()), static_cast<JSObjectRef>(objectInstance->getObject()), 0, arguments, NULL);
    toString(gContext, result);
    //printf("got class name from class template: %s\n", classRef->className().c_str());
  }
  return "exports.find =  function (successCallback, errorCallback, filter, sortMode, limit, offset) { }";
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

