extern "C" __attribute__((visibility("default"))) int init_jsc();
extern "C" __attribute__((visibility("default"))) const char* add_change_listener(void*);
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
#include "jsc_wrapper.h"
#include "common/picojson.h"
#include <wrt-plugins-tizen/common/JSUtil.h>
static JSObjectPtr objectInstance;
static JSGlobalContextRef gContext;
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

std::string toString(JSContextRef ctx, JSValueRef value) {
  Assert(ctx && value);
  std::string result;
  JSStringRef str = JSValueToStringCopy(ctx, value, NULL);
  result = toString(str);
  JSStringRelease(str);
  return result;
}
JSValueRef addChangeListenerCallback (JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception){
  printf("#########################\nchange listener callback called!\n############################\n");
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
    if (_api) {
      std::string msg = "{ \"data\": " + toString(JSValueCreateJSONString(gContext, arguments[0], 2, 0));
      if (_reply_id != "")
        msg += (", \"_reply_id\": " + _reply_id);
      msg += "}";
      printf("%s\n", msg.c_str());
      ((ContextAPI*)_api)->PostMessage(msg.c_str());
    }
    else 
      printf("_api is null!\n");
  }
  return NULL;
}

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
#include <Ecore.h>
#include <sys/select.h>
#include <fcntl.h>
typedef int (*EcoreSelectType)(int nfds, fd_set *readfds, fd_set *writefds,
                               fd_set *exceptfds, struct timeval *timeout);
EcoreSelectType m_oldEcoreSelect;
int EcoreSelectInterceptor(int nfds,
                                 fd_set *readfds,
                                 fd_set *writefds,
                                 fd_set *exceptfds,
                                 struct timeval *timeout)
{
    // We have to check error code here and make another try because of some
    // glib error's.
    fd_set rfds, wfds, efds;
    memcpy(&rfds, readfds, sizeof(fd_set));
    memcpy(&wfds, writefds, sizeof(fd_set));
    memcpy(&efds, exceptfds, sizeof(fd_set));

    int ret = m_oldEcoreSelect(nfds,
                                                         readfds,
                                                         writefds,
                                                         exceptfds,
                                                         timeout);

    if (ret == -1) {
        // Check each descriptor to see if it is valid
        for (int i = 0; i < nfds; i++) {
            if (FD_ISSET(i,
                         readfds) ||
                FD_ISSET(i, writefds) || FD_ISSET(i, exceptfds))
            {
                // Try to get descriptor flags
                int result = fcntl(i, F_GETFL);

                if (result == -1) {
                    if (errno == EBADF) {
                        // This a bad descriptor. Remove all occurrences of it.
                        if (FD_ISSET(i, readfds)) {
                            LogPedantic(
                                "GLIB_LOOP_INTEGRATION_WORKAROUND: Bad read descriptor: "
                                << i);
                            FD_CLR(i, readfds);
                        }

                        if (FD_ISSET(i, writefds)) {
                            LogPedantic(
                                "GLIB_LOOP_INTEGRATION_WORKAROUND: Bad write descriptor: "
                                << i);
                            FD_CLR(i, writefds);
                        }

                        if (FD_ISSET(i, exceptfds)) {
                            LogPedantic(
                                "GLIB_LOOP_INTEGRATION_WORKAROUND: Bad exception descriptor: "
                                << i);
                            FD_CLR(i, exceptfds);
                        }
                    } else {
                        // Unexpected error
                        assert(0);
                    }
                }
            }
        }

        LogPedantic(
            "GLIB_LOOP_INTEGRATION_WORKAROUND: Bad read descriptor. Retrying with default select.");

        //Retry with old values and return new error
        memcpy(readfds, &rfds, sizeof(fd_set));
        memcpy(writefds, &wfds, sizeof(fd_set));
        memcpy(exceptfds, &efds, sizeof(fd_set));

        // Trying to do it very short
        timeval tm;
        tm.tv_sec = 0;
        tm.tv_usec = 10;

        if (timeout) {
            ret = select(nfds, readfds, writefds, exceptfds, &tm);
        } else {
            ret = select(nfds, readfds, writefds, exceptfds, NULL);
        }
    }

    return ret;
}
gboolean iterate_ecore_main_loop (gpointer user_data) {
    ecore_main_loop_iterate ();
    return TRUE;
}
void* start_ecore_main_loop(void*){
//void start_ecore_main_loop(void*, Ecore_Thread *th) {
  //DPL::MainSingleaton::instance();
  //ecore_init();
  printf("starting ecore main loop...\n");
  ecore_init();
  union ConvertPointer
    {
        Ecore_Select_Function pointer;
        EcoreSelectType function;
    } convert;

    convert.pointer = ecore_main_loop_select_func_get();
    m_oldEcoreSelect = convert.function;

    ecore_main_loop_select_func_set(&EcoreSelectInterceptor);

    // It is impossible that there exist watchers at this time
    // No need to add watchers
    LogPedantic("ECORE event handler registered");

  ecore_main_loop_glib_integrate();
  //ecore_main_loop_begin();
  /*while (1) {
    ecore_main_loop_iterate ();
    //usleep(1000000);
  }*/
  //g_idle_add(iterate_ecore_main_loop, NULL);
  g_timeout_add(100, iterate_ecore_main_loop, NULL);
  printf("ecore main returned\n");
}
class EcoreMainThread: public DPL::Thread {
protected:
  int ThreadEntry() {
    start_ecore_main_loop(NULL);
  }
};
PluginPtr plugin;
class APIThread: public DPL::Thread {
protected:
  int ThreadEntry() {
  }
};
DPL::WaitableEvent* wait = new DPL::WaitableEvent;
std::string id;
static void deleteMessage(void* api, void* message) {
  printf("Deleting message...\n");
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
  if (cmd == "find") {
    JSValueRef exception = NULL;
    JSObjectPtr functionObject = JavaScriptInterfaceSingleton::Instance().getJSObjectProperty(gContext, objectInstance, "find");
    JSObjectRef obj = JSObjectMakeFunctionWithCallback(gContext, NULL, find_cb);
    JSClassRef apiClass = JSClassCreate(&listener_def);
    JSObjectRef apiObj = JSObjectMake(gContext, apiClass, api);
    assert(JSObjectGetPrivate(apiObj));
    JSObjectSetProperty(gContext, obj, JSStringCreateWithUTF8CString("_api"), apiObj, kJSPropertyAttributeNone, &exception);
    JSObjectSetProperty(gContext, obj, JSStringCreateWithUTF8CString("_reply_id"), JSValueMakeString(gContext, JSStringCreateWithUTF8CString(v.get("_reply_id").to_str().c_str())), kJSPropertyAttributeNone, &exception);
    JSValueRef arguments[] = {obj};
    JSObjectCallAsFunction(gContext, static_cast<JSObjectRef>(functionObject->getObject()), static_cast<JSObjectRef>(objectInstance->getObject()), 1, arguments, &exception);
    if (exception)
      printf("Exception:%s\n", toString(gContext, exception).c_str());
  }
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
      printf("Call addChangeListener succeed:%s\n", toString(gContext, result).c_str());
      id = toString(gContext, result);
    }
    wait->Signal();
}

DPL::Thread* appThread = WrtDeviceApis::Commons::ThreadPool::getInstance().getThreadRef(WrtDeviceApis::Commons::ThreadEnum::APPLICATION_THREAD);

int init_jsc() {
  //(new EcoreMainThread())->Run();
  appThread->Run();
  start_ecore_main_loop(NULL);
  //pthread_t ecore_main_thread;
  //pthread_create(&ecore_main_thread, NULL,  start_ecore_main_loop, NULL);
  //pthread_detach(ecore_main_thread);
  //ecore_thread_run(start_ecore_main_loop, NULL, NULL, NULL);
  printf("loading callhistory lib\n");
  plugin =  Plugin::LoadFromFile("/usr/lib/wrt-plugins/tizen-callhistory/libwrt-plugins-tizen-callhistory.so");
  plugin->OnWidgetStart(0);
  printf("loaded callhistory lib\n");
  Plugin::ClassPtrList list = plugin->GetClassList();
  printf("got callhistory lib\n");
  gContext = JSGlobalContextCreateInGroup(NULL, NULL);
  for (std::list<Plugin::ClassPtr>::iterator it = list->begin(); it != list->end(); it ++) {
    printf("calling class template of %s\n", (*it)->getName().c_str());
    //JSClassRef classRef = static_cast<JSClassRef>(const_cast<JSObjectDeclaration::ClassTemplate>((*it)->getClassTemplate()));
    objectInstance = JavaScriptInterfaceSingleton::Instance().
              createObject(gContext, *it);
  }
  return 0;
}

void handleMessage(ContextAPI* api, const char* msg) {
  appThread->PushEvent(api, &processMessage, &deleteMessage, (void*)strdup(msg));
}
void handleSyncMessage(ContextAPI* api, const char* msg){
   printf("pushing event...\n");
   appThread->PushEvent(api, &processSyncMessage, &deleteMessage, NULL);
   DPL::WaitForSingleHandle(wait->GetHandle());
   api->SetSyncReply(id.c_str());
}
