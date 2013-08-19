extern "C" __attribute__((visibility("default"))) int init_jsc();
#include <JavaScriptCore/JSObjectRef.h>
#include <stdio.h>
#include "plugin.h"
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/JSObjectRef.h>
#include "javascript_interface.h"
#include "dpl/foreach.h"
#include <dpl/scoped_array.h>
#include <dpl/thread.h>
#include <Ecore.h>
#include <iostream>
#include <glib.h>
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
    usleep(1000000);
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
  g_idle_add(iterate_ecore_main_loop, NULL);
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
    printf("loading callhistory lib\n");
    plugin =  Plugin::LoadFromFile("/usr/lib/wrt-plugins/tizen-callhistory/libwrt-plugins-tizen-callhistory.so");
    plugin->OnWidgetStart(0);
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

      JSValueRef result;
      JSValueRef exception = NULL;
      JSClassRef listenerDef = JSClassCreate(&listener_def);
      JSObjectRef listenerObj = JSObjectMake(gContext, listenerDef, gContext);
      JSValueRef arguments[] = {listenerObj};
      result = JSObjectCallAsFunction(gContext, static_cast<JSObjectRef>(functionObject->getObject()), static_cast<JSObjectRef>(objectInstance->getObject()), 1, arguments, &exception);
      if (exception)
        printf("Exception:%s\n", toString(gContext, exception).c_str());
      if (!result)
        printf("failure to call addChangeListener\n");
      else 
        printf("Call addChangeListener succeed:%s\n", toString(gContext, result).c_str());
      //printf("got class name from class template: %s\n", classRef->className().c_str());
    }
  }
};



  int init_jsc() {
    (new APIThread())->Run();
    //(new EcoreMainThread())->Run();
    start_ecore_main_loop(NULL);
    //pthread_t ecore_main_thread;
    //pthread_create(&ecore_main_thread, NULL,  start_ecore_main_loop, NULL);
    //pthread_detach(ecore_main_thread);
    //ecore_thread_run(start_ecore_main_loop, NULL, NULL, NULL);
    return 0;
  }
