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
#include <stdint.h>
static JSObjectPtr objectInstance;
static JSGlobalContextRef gContext;
static PluginPtr plugin;
static DPL::WaitableEvent* wait = new DPL::WaitableEvent;
static DPL::Thread* appThread = WrtDeviceApis::Commons::ThreadPool::getInstance().getThreadRef(WrtDeviceApis::Commons::ThreadEnum::APPLICATION_THREAD);
static JSObjectRef entryTempl;

typedef struct {
  ContextAPI* api;
  std::string _reply_id;
} CallbackData;

static const JSClassDefinition api_def =
{
    0, // version
    kJSClassAttributeNone, // attributes
    "Api", // className
    NULL, // parentClass
    NULL, // staticValues
    NULL, // staticFunctions
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

static JSValueRef* picoArrayToJSValueArray(const picojson::value& value, JSValueRef cb_data);
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
static std::string toString(JSValueRef value) {
  return toString(gContext, value);
}
static JSValueRef stringToJSValue(const std::string& str) {
  return JSValueMakeString(gContext, JSStringCreateWithUTF8CString(str.c_str()));
}
static std::string toJSON(JSValueRef value) {
  return toString(JSValueCreateJSONString(gContext, value, 2, 0));
}
static JSObjectRef toJSObject(JSValueRef value) {
  return JSValueToObject(gContext, value, NULL);
}
static JSValueRef getProperty(JSObjectRef obj, const char* name) {
  return JSObjectGetProperty(gContext, obj, JSStringCreateWithUTF8CString(name), NULL);
}
static std::string getPropertyAsString(JSObjectRef obj, const char* name) {
  return toString(getProperty(obj, name));
}
static JSObjectRef getPropertyAsObject(JSObjectRef obj, const char* name) {
  return toJSObject(getProperty(obj, name));
}
static void setProperty(JSObjectRef obj, const char* name, JSValueRef value) {
  JSValueRef exception = NULL;
  JSObjectSetProperty(gContext, obj, JSStringCreateWithUTF8CString(name), value, kJSPropertyAttributeNone, &exception);
}
static void setStringProperty(JSObjectRef obj, const char* name, const std::string& value) {
  setProperty(obj, name, stringToJSValue(value));
}
static void setSubProperty(JSObjectRef obj, const char* propertyName, const char* name, JSValueRef value) {
  JSValueRef exception = NULL;
  setProperty(getPropertyAsObject(obj, propertyName), name, value);
}
JSValueRef general_cb (JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception){
  printf("#########################\ngeneral_cb called!\n############################\n");
  if (argumentCount > 0) {
    JSObjectRef resp = JSObjectMake(gContext, NULL, NULL);
    setProperty(resp, "data", arguments[0]);
    CallbackData* cb_data = (CallbackData*)JSObjectGetPrivate(getPropertyAsObject(function, "_cb_data"));
    if (!cb_data->_reply_id.empty()){
      setStringProperty(resp, "_reply_id", cb_data->_reply_id.c_str());
      if (JSIsArrayValue(ctx, arguments[0])) {
        JSObjectRef entryList = toJSObject(arguments[0]);
          if (JSGetArrayLength(gContext, entryList) > 0) {
            entryTempl = toJSObject(JSGetArrayElement(gContext, entryList, 0));
            printf("entryTempl address: %u\n", entryTempl);
            setProperty(entryTempl, "__obj_ref", JSValueMakeNumber(gContext, (uintptr_t)entryTempl));
          }
      }
    } else {
      setStringProperty(resp, "eventType", getPropertyAsString(function, "name"));
    }
    cb_data->api->PostMessage(toJSON(resp).c_str());
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

static picojson::value* parseMesssage(void* message) {
  picojson::value& v = *new picojson::value();
  std::string err;
  const char* msg = (const char*) message;
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    delete &v; 
    return NULL;
  }
  return &v;
}
static JSValueRef picoToJS(picojson::value& value, const char* key,  JSValueRef cb_data) {
    //arguments[i++] = JSValueMakeFromJSONString(gContext, it->to_str());
    if (value.is<picojson::value::object>()){
      JSObjectRef obj = JSObjectMake(gContext, NULL, NULL);
      FOREACH(it, value.get<picojson::value::object>()) {
        setProperty(obj, it->first.c_str(), picoToJS(it->second, it->first.c_str(), cb_data));
      }
      return obj;
    }
    else if (value.is<picojson::value::array>()){
      picojson::value::array picoArray = value.get<picojson::value::array>();
      return JSObjectMakeArray(gContext, picoArray.size(), picoArrayToJSValueArray(value, cb_data), NULL);
    }
    else if (value.is<std::string>()){
      std::string prefix = "__function__";
      if (value.to_str().substr(0, prefix.size()) == prefix) {
         JSObjectRef func =  JSObjectMakeFunctionWithCallback(gContext, JSStringCreateWithUTF8CString(key), general_cb);
         setProperty(func, "_cb_data", cb_data);
         return func;
      }
      else
        return stringToJSValue(value.to_str());
    }
    else if (value.is<picojson::null>()){
      return JSValueMakeNull(gContext);
    }
    else if (value.is<bool>()){
      return JSValueMakeBoolean(gContext, value.get<bool>());
    }
    else if (value.is<double>()){
      return JSValueMakeNumber(gContext, value.get<double>());
    }
    /*else if (value.is<char*>()){
      return stringToJSValue(value.get<char*>());
    }*/
}
static JSValueRef* picoArrayToJSValueArray(const picojson::value& value, JSValueRef cb_data) {
  picojson::value::array picoArray = value.get<picojson::value::array>();
  JSValueRef* jsValueArray = new JSValueRef[picoArray.size()];
  size_t i = 0;
  FOREACH(it, picoArray) {
    jsValueArray[i++] = picoToJS(*it, NULL, cb_data);
  }
  return jsValueArray;
}
static void processMessage(void* api, void* message) {
  picojson::value* v = parseMesssage(message);
  if (!v) return;
    JSObjectPtr functionObject = JavaScriptInterfaceSingleton::Instance().getJSObjectProperty(gContext, objectInstance, v->get("cmd").to_str().c_str());
    JSValueRef result;
    JSValueRef exception = NULL;
    CallbackData* cb_data = new CallbackData;
    cb_data->api = (ContextAPI*)api;
    if (v->contains("_reply_id"))
      cb_data->_reply_id = v->get("_reply_id").to_str();
    JSValueRef* arguments = NULL;
    picojson::value::array args = v->get("arguments").get<picojson::value::array>();
    if (args.size() == 1) {
      if (args[0].is<picojson::value::object>() && args[0].contains("__obj_ref")){
        arguments = new JSValueRef[1];
        arguments[0] = (JSValueRef)(uintptr_t)(args[0].get("__obj_ref").get<double>());
      }
    }
    if (!arguments)
      arguments = picoArrayToJSValueArray(v->get("arguments"), JSObjectMake(gContext, JSClassCreate(&api_def), cb_data));

    result = JSObjectCallAsFunction(
        gContext, 
        static_cast<JSObjectRef>(functionObject->getObject()),
        static_cast<JSObjectRef>(objectInstance->getObject()),
        args.size(),
        arguments,
        &exception);
    if (exception)
      printf("Exception:%s\n", toString(gContext, exception).c_str());
    if (!result)
      printf("failure to call %s\n", v->get("cmd").to_str().c_str());
    else 
      ((ContextAPI*)api)->SetSyncReply(toString(gContext, result).c_str());
}
static void processSyncMessage(void* api, void* message) {
  processMessage(api, message);
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
  appThread->PushEvent(api, &processSyncMessage, &deleteMessage, (void*) strdup(msg));
  DPL::WaitForSingleHandle(wait->GetHandle());
}
