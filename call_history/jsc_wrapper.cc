static int init_jsc() __attribute__((constructor));
#include <JavaScriptCore/JSObjectRef.h>
#include <stdio.h>
#include "plugin.h"
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/JSObjectRef.h>
#include "javascript_interface.h"
#include "dpl/foreach.h"
#include <dpl/scoped_array.h>
#include <dpl/thread.h>
#include <Commons/ThreadPool.h>
#include <Ecore.h>
#include <iostream>
#include <glib.h>
#include "common/picojson.h"
#include "common/extension_adapter.h"
static JSObjectPtr objectInstance;
static JSGlobalContextRef gContext;
static PluginPtr plugin;
static DPL::WaitableEvent* wait = new DPL::WaitableEvent;
DPL::Thread* appThread = WrtDeviceApis::Commons::ThreadPool::getInstance().getThreadRef(WrtDeviceApis::Commons::ThreadEnum::APPLICATION_THREAD);
static JSValueRef added_cb(JSContextRef context,
                                      JSObjectRef function,
                                      JSObjectRef thisObject,
                                      size_t argumentCount,
                                      const JSValueRef arguments[],
                                      JSValueRef *exception);
static JSValueRef addChangeListenerCallback(JSContextRef context,
                                      JSObjectRef function,
                                      JSObjectRef thisObject,
                                      size_t argumentCount,
                                      const JSValueRef arguments[],
                                      JSValueRef *exception);
static const JSStaticFunction listener_staticfuncs[] =
{
    { "onadded", added_cb, kJSPropertyAttributeReadOnly },
    { "onchanged", addChangeListenerCallback, kJSPropertyAttributeReadOnly },
    { "onremoved", addChangeListenerCallback, kJSPropertyAttributeReadOnly },
    { NULL, NULL, 0 }
};
static const JSClassDefinition listener_def =
{
    0, // version
    kJSClassAttributeNone, // attributes
    "Listener", // className
    NULL, // parentClass
    NULL, // staticValues
    listener_staticfuncs, // staticFunctions
    NULL, // initialize
    NULL, // finalize
    NULL, // hasProperty
    NULL, // getProperty
    NULL, // setProperty
    NULL, // deleteProperty
    NULL, // getPropertyNames
    NULL, // callAsFunction
    NULL, // callAsConstructor
    NULL, // hasInstance
    NULL // convertToType
};

static std::string toString(const JSStringRef& arg) {
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

static std::string toString(JSContextRef ctx, JSValueRef value) {
  Assert(ctx && value);
  std::string result;
  JSStringRef str = JSValueToStringCopy(ctx, value, NULL);
  result = toString(str);
  JSStringRelease(str);
  return result;
}

JSValueRef addChangeListenerCallback (JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception){
  printf("#########################\nchange listener callback called!\n############################\n");
  return NULL;
}

static JSValueRef added_cb(JSContextRef context,
                                      JSObjectRef function,
                                      JSObjectRef thisObject,
                                      size_t argumentCount,
                                      const JSValueRef arguments[],
                                      JSValueRef *exception)
{
  printf("#########################\nCall history Added!\n############################\n");
  return NULL;
}
static JSObjectRef entryTempl;
static JSValueRef find_cb(JSContextRef context,
                                      JSObjectRef function,
                                      JSObjectRef thisObject,
                                      size_t argumentCount,
                                      const JSValueRef arguments[],
                                      JSValueRef *exception)
{
  printf("#########################\nCall history Found!\n############################\n");
  printf("argumentCount: %d\n", argumentCount);
  void* _api;
  if (argumentCount > 0) {
    std::string _reply_id = toString(gContext, JSObjectGetProperty(gContext, function, JSStringCreateWithUTF8CString("_reply_id"), NULL));
    _api = JSObjectGetPrivate(JSValueToObject(gContext, JSObjectGetProperty(gContext, function, JSStringCreateWithUTF8CString("_api"), NULL), NULL));
    JSObjectRef entryList = JSValueToObject(gContext, arguments[0], NULL);
    if (JSGetArrayLength(gContext, entryList) > 0) {
      entryTempl = JSValueToObject(gContext, JSGetArrayElement(gContext, entryList, 0), NULL);
    }
    if (_api) {
      std::string msg = "{ \"data\": " + toString(JSValueCreateJSONString(gContext, arguments[0], 2, 0));
      if (_reply_id != "")
        msg += (", \"_reply_id\": " + _reply_id);
      msg += "}";
      ((ContextAPI*)_api)->PostMessage(msg.c_str());
    }
    else 
      printf("_api is null!\n");
  }
  return NULL;
}

static gboolean iterate_ecore_main_loop (gpointer user_data) {
    ecore_main_loop_iterate ();
    return TRUE;
}

static void deleteMessage(void* api, void* message) {
  if (message)
    free(message);
}

static void processMessage(void* api, void* message) {
  picojson::value& v = *new picojson::value();
  std::string err;
  const char* msg = (const char*) message;
  printf("handling message: %s\n", msg);
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }
  std::string cmd = v.get("cmd").to_str();
  JSValueRef exception = NULL;
  JSObjectPtr functionObject;
  JSValueRef arguments[1];
  if (cmd == "find") {
    functionObject = JavaScriptInterfaceSingleton::Instance().getJSObjectProperty(gContext, objectInstance, "find");
    JSObjectRef obj = JSObjectMakeFunctionWithCallback(gContext, NULL, find_cb);
    JSClassRef apiClass = JSClassCreate(&listener_def);
    JSObjectRef apiObj = JSObjectMake(gContext, apiClass, api);
    assert(JSObjectGetPrivate(apiObj));
    JSObjectSetProperty(gContext, obj, JSStringCreateWithUTF8CString("_api"), apiObj, kJSPropertyAttributeNone, &exception);
    JSObjectSetProperty(gContext, obj, JSStringCreateWithUTF8CString("_reply_id"), JSValueMakeString(gContext, JSStringCreateWithUTF8CString(v.get("_reply_id").to_str().c_str())), kJSPropertyAttributeNone, &exception);
    arguments[0] = {obj};
  }
  else if (cmd == "remove") {
    functionObject = JavaScriptInterfaceSingleton::Instance().getJSObjectProperty(gContext, objectInstance, "remove");
    LogDebug("contructing remove parameter, entry" << v.get("entry").serialize());
    JSObjectSetProperty(gContext, entryTempl, JSStringCreateWithUTF8CString("uid"), JSValueMakeString(gContext, JSStringCreateWithUTF8CString(v.get("entry").get("uid").to_str().c_str())), kJSPropertyAttributeNone, NULL);
    arguments[0] = {entryTempl};
    LogDebug("constructed remove parameter");
  }
  JSObjectCallAsFunction(gContext, static_cast<JSObjectRef>(functionObject->getObject()), static_cast<JSObjectRef>(objectInstance->getObject()), 1, arguments, &exception);
  if (exception)
    printf("Exception:%s\n", toString(gContext, exception).c_str());
}

static void processSyncMessage(void* api, void* message) {
    JSObjectPtr functionObject = JavaScriptInterfaceSingleton::Instance().getJSObjectProperty(gContext, objectInstance, "addChangeListener");
    JSValueRef result;
    JSValueRef exception = NULL;
    JSClassRef listenerDef = JSClassCreate(&listener_def);
    JSObjectRef listenerObj = JSObjectMake(gContext, listenerDef, gContext);
    assert(JSObjectGetPrivate(listenerObj));
    JSValueRef arguments[] = {listenerObj};
    result = JSObjectCallAsFunction(gContext, static_cast<JSObjectRef>(functionObject->getObject()), static_cast<JSObjectRef>(objectInstance->getObject()), 1, arguments, &exception);
    if (exception)
      printf("Exception:%s\n", toString(gContext, exception).c_str());
    if (!result)
      printf("failure to call addChangeListener\n");
    else  {
      ((ContextAPI*)api)->SetSyncReply(toString(gContext, result).c_str());
    }
    wait->Signal();
}

int init_jsc() {
  appThread->Run();
  printf("starting ecore main loop...\n");
  ecore_init();
  g_timeout_add(100, iterate_ecore_main_loop, NULL);
  printf("ecore main returned\n");
  printf("loading callhistory lib\n");
  plugin =  Plugin::LoadFromFile("/usr/lib/wrt-plugins/tizen-callhistory/libwrt-plugins-tizen-callhistory.so");
  plugin->OnWidgetStart(0);
  printf("loaded callhistory lib\n");
  Plugin::ClassPtrList list = plugin->GetClassList();
  printf("got callhistory lib\n");
  gContext = JSGlobalContextCreateInGroup(NULL, NULL);
  for (std::list<Plugin::ClassPtr>::iterator it = list->begin(); it != list->end(); it ++) {
    printf("calling class template of %s\n", (*it)->getName().c_str());
    objectInstance = JavaScriptInterfaceSingleton::Instance().
              createObject(gContext, *it);
  }
  return 0;
}

#define PUBLIC_EXPORT __attribute__((visibility("default")))
#define EXTERN_C extern "C"
EXTERN_C PUBLIC_EXPORT void handle_msg(ContextAPI* api, const char* msg) {
  appThread->PushEvent(api, &processMessage, &deleteMessage, (void*)strdup(msg));
}
  
EXTERN_C PUBLIC_EXPORT void handle_sync_msg(ContextAPI* api, const char* msg){
  appThread->PushEvent(api, &processSyncMessage, &deleteMessage, NULL);
  DPL::WaitForSingleHandle(wait->GetHandle());
}
