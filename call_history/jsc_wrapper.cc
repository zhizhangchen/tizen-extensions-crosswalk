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
static JSGlobalContextRef gContext;
static PluginPtr plugin;
static PluginPtr tizen_plugin;
//static DPL::WaitableEvent* wait =  new DPL::WaitableEvent;
static DPL::WaitableEvent* wait;
static DPL::Thread* appThread = WrtDeviceApis::Commons::ThreadPool::getInstance().getThreadRef(WrtDeviceApis::Commons::ThreadEnum::APPLICATION_THREAD);
static JSObjectRef entryTempl;
static std::map<std::string, JSObjectPtr> objectInstances;

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

static JSValueRef* picoArrayToJSValueArray(const picojson::value& value, ContextAPI* api);
static std::string toString(const JSStringRef& arg) {
    //Assert(arg);
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
void setObjRef(JSObjectRef obj) {
  if (JSIsArrayValue(gContext, obj)) {
    size_t len = JSGetArrayLength(gContext, obj);
    for (size_t i = 0; i < len; i++) {
      JSObjectRef elem = toJSObject(JSGetArrayElement(gContext, obj, i));
      setProperty(elem, "__obj_ref", JSValueMakeNumber(gContext, (uintptr_t)elem));
    }
  }
  else if (JSValueIsObject(gContext, obj)) {
      setProperty(obj, "__obj_ref", JSValueMakeNumber(gContext, (uintptr_t)obj));
  }
}
JSValueRef general_cb (JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception){
  printf("#########################\ngeneral_cb called!\n############################\n");
  CallbackData* cb_data = (CallbackData*)JSObjectGetPrivate(getPropertyAsObject(function, "_cb_data"));
  JSObjectRef resp = JSObjectMake(gContext, NULL, NULL);
  setProperty(resp, "arguments", JSObjectMakeArray(gContext, argumentCount, arguments, NULL));
  if (!cb_data->_reply_id.empty()){
    setStringProperty(resp, "_reply_id", cb_data->_reply_id.c_str());
    for (size_t i = 0; i < argumentCount; i ++)
      setObjRef(toJSObject(arguments[i]));
  }
  else {
    setStringProperty(resp, "eventType", getPropertyAsString(function, "name"));
  }
  cb_data->api->PostMessage(toJSON(resp).c_str());
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
static bool startsWith(std::string str, std::string prefix) {
  return str.substr(0, prefix.size()) == prefix;
}
static JSValueRef picoToJS(picojson::value& value, const char* key,  ContextAPI* api) {
    if (value.is<picojson::value::object>()){
      if (value.contains("__obj_ref"))
        return  (JSValueRef)(uintptr_t)(value.get("__obj_ref").get<double>());
      JSObjectRef obj = JSObjectMake(gContext, NULL, NULL);
      FOREACH(it, value.get<picojson::value::object>()) {
        setProperty(obj, it->first.c_str(), picoToJS(it->second, it->first.c_str(), api));
      }
      return obj;
    }
    else if (value.is<picojson::value::array>()){
      picojson::value::array picoArray = value.get<picojson::value::array>();
      return JSObjectMakeArray(gContext, picoArray.size(), picoArrayToJSValueArray(value, api), NULL);
    }
    else if (value.is<std::string>()){
      if (startsWith(value.to_str(), "__function__")) {
        std::string prefix = "__function__";
        JSObjectRef func =  JSObjectMakeFunctionWithCallback(gContext, JSStringCreateWithUTF8CString(key), general_cb);
        CallbackData * cb_data = new CallbackData;
        cb_data->api = api;
        cb_data->_reply_id = value.to_str().substr(prefix.size() + 1);
        setProperty(func, "_cb_data", JSObjectMake(gContext, JSClassCreate(&api_def), cb_data));
        return func;
      }
      else if (startsWith(value.to_str(), "__undefined__")) {
          return JSValueMakeUndefined(gContext);
      }
      else if (startsWith(value.to_str(), "__NaN__")) {
          printf("############################# Making NaN!\n");
          return JSValueMakeNumber(gContext, std::numeric_limits<double>::quiet_NaN());
      }
      else
        return stringToJSValue(value.to_str());
    }
    else if (value.is<picojson::null>()){
      printf("############################# Making null value!\n");
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
static JSValueRef* picoArrayToJSValueArray(const picojson::value& value, ContextAPI* api) {
  picojson::value::array picoArray = value.get<picojson::value::array>();
  JSValueRef* jsValueArray = new JSValueRef[picoArray.size()];
  size_t i = 0;
  FOREACH(it, picoArray) {
    jsValueArray[i++] = picoToJS(*it, NULL, api);
  }
  return jsValueArray;
}
static JSValueRef callJSFunc(void* api, void* message, JSValueRef* exception) {
  printf("callJSFunc: %s\n", message);
  picojson::value* v = parseMesssage(message);
  JSObjectPtr objectInstance = objectInstances[v->get("api").to_str()];
  if (!v) return NULL;
    JSValueRef result;
    JSValueRef* arguments = NULL;
    picojson::value::array args = v->get("arguments").get<picojson::value::array>();
    arguments = picoArrayToJSValueArray(v->get("arguments"), (ContextAPI*)api);

    std::string cmd = v->get("cmd").to_str();
    if (cmd == "__constructor__") {
      result = JSObjectCallAsConstructor(
          gContext, 
          static_cast<JSObjectRef>(objectInstance->getObject()),
          args.size(),
          arguments,
          exception);
    }
    else {
      JSObjectPtr functionObject = JavaScriptInterfaceSingleton::Instance().getJSObjectProperty(gContext, objectInstance, cmd.c_str());
      result = JSObjectCallAsFunction(
          gContext, 
          static_cast<JSObjectRef>(functionObject->getObject()),
          static_cast<JSObjectRef>(objectInstance->getObject()),
          args.size(),
          arguments,
          exception);
    }
    if (JSValueIsObject(gContext, result))
      setObjRef(toJSObject(result));
    return result;
}
static void processMessage(void* api, void* message) {
  JSValueRef exception;
  callJSFunc(api, message, &exception);
}
static void processSyncMessage(void* api, void* message) {
  JSValueRef exception;
  JSValueRef result = callJSFunc(api, message, &exception);
  if (!result)
    printf("result is null to process message: %s\n", message);
  JSObjectRef resp = JSObjectMake(gContext, NULL, NULL);
  if (exception) {
    printf("exception:%s\n", toString(exception).c_str());
    setProperty(resp, "exception", exception);
  }
  if (result)
    setProperty(resp, "result", result);
  ((ContextAPI*)api)->SetSyncReply(strdup(toJSON(resp).c_str()));
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
  tizen_plugin =  Plugin::LoadFromFile("/usr/lib/wrt-plugins/tizen-tizen/libwrt-plugins-tizen-tizen.so");
  plugin->OnWidgetStart(0);
  tizen_plugin->OnWidgetStart(0);
  printf("loaded callhistory lib\n");
  Plugin::ClassPtrList list = plugin->GetClassList();
  printf("got callhistory lib\n");
  gContext = JSGlobalContextCreateInGroup(NULL, NULL);
  for (std::list<Plugin::ClassPtr>::iterator it = list->begin(); it != list->end(); it ++) {
    printf("calling class template of %s\n", (*it)->getName().c_str());
    objectInstances[(*it)->getName()] = JavaScriptInterfaceSingleton::Instance().
              createObject(gContext, *it);
  }
  list = tizen_plugin->GetClassList();
  printf("got tizen class list\n");
  for (std::list<Plugin::ClassPtr>::iterator it = list->begin(); it != list->end(); it ++) {
    printf("calling class template of %s\n", (*it)->getName().c_str());
    if ((*it)->getName() == "tizen")
      continue;
    objectInstances[(*it)->getName()] = JavaScriptInterfaceSingleton::Instance().
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
  wait =  new DPL::WaitableEvent;
  appThread->PushEvent(api, &processSyncMessage, &deleteMessage, (void*) strdup(msg));
  DPL::WaitForSingleHandle(wait->GetHandle());
  delete wait;
  //wait->Reset();
}

EXTERN_C PUBLIC_EXPORT const char* get_object_properties(){
  picojson::value::object object;
  FOREACH(itr, objectInstances) {
    if (JSObjectIsConstructor(gContext, static_cast<JSObjectRef>(itr->second->getObject()))) {
      object[itr->first] = picojson::value("__constructor__");
      continue;
    }
    picojson::value::array properties;
    FOREACH(it, JavaScriptInterfaceSingleton::Instance().getObjectPropertiesList(gContext, itr->second)) {
      properties.push_back(picojson::value(*it));
    }
    object[itr->first] = picojson::value(properties);
  }
  return strdup(picojson::value(object).serialize().c_str());
}
