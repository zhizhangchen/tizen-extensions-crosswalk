// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "call_history/call_history_context.h"
#include "common/picojson.h"
#include <memory>
#include <wrt-plugins-tizen/callhistory/CallHistoryFactory.h>
#include <wrt-plugins-tizen/callhistory/ICallHistory.h>
#include <wrt-plugins-tizen/callhistory/ResponseDispatcher.h>
#include <dpl/thread.h>
#include <dlog.h>
using namespace DeviceAPI::CallHistory;
using namespace WrtDeviceApis::Commons;
class CallHistoryXWalkResponseDispatcher :
  public WrtDeviceApis::Commons::EventAnswerReceiver<EventFindCallHistory>,
  public WrtDeviceApis::Commons::EventAnswerReceiver<EventRemoveBatch>,
  public WrtDeviceApis::Commons::EventAnswerReceiver<EventRemoveAll>
{
  public:
    void OnAnswerReceived(const EventFindCallHistoryPtr& event){
      printf("found call history\n");
      printf("Entries: %u\n", event->getResult()->size());//->first()->getEntryId());
      printf("Entry ID: %u\n", event->getResult()->at(0)->getEntryId());//->first()->getEntryId());
      printf("Service ID: %s\n", event->getResult()->at(0)->getServiceId().c_str());//->first()->getEntryId());
      printf("Call type : %s\n", event->getResult()->at(0)->getCallType().c_str());//->first()->getEntryId());
      printf("Tags : %d\n", event->getResult()->at(0)->getTags()->size());//->first()->getEntryId());
      printf("Tag 0 : %s\n", event->getResult()->at(0)->getTags()->at(0).c_str());//->first()->getEntryId());
      printf("Remote parties : %d\n", event->getResult()->at(0)->getRemoteParties()->size());//->first()->getEntryId());
      printf("Remote party 0 : %s, person id 0:\n", event->getResult()->at(0)->getRemoteParties()->at(0)->getRemoteParty().c_str(),  event->getResult()->at(0)->getRemoteParties()->at(0)->getPersonId().c_str());//->first()->getEntryId());
      printf("Start time : %d\n", event->getResult()->at(0)->getStartTime());
      printf("Duration : %d\n", event->getResult()->at(0)->getDuration());
      printf("End Reason : %s\n", event->getResult()->at(0)->getEndReason().c_str());//->first()->getEntryId());
      printf("Direction: %s\n", event->getResult()->at(0)->getDirection().c_str());//->first()->getEntryId());
      printf("Recording: %d\n", event->getResult()->at(0)->getRecording()->size());//->first()->getEntryId());
      printf("Cost : %d\n", event->getResult()->at(0)->getCost());
      printf("Currency : %s\n", event->getResult()->at(0)->getCurrency().c_str());
      printf("Filter Mark : %d\n", event->getResult()->at(0)->getFilterMark());
    }
    void OnAnswerReceived(const EventRemoveBatchPtr& event){
      LOGD("removed batch\n");
    }
    void OnAnswerReceived(const EventRemoveAllPtr& event){
      LOGD("removed all\n");
    }
    static CallHistoryXWalkResponseDispatcher& getInstance() {
      static CallHistoryXWalkResponseDispatcher dispatcher;
      return dispatcher;
    }

  protected:
    CallHistoryXWalkResponseDispatcher():
      EventAnswerReceiver<EventFindCallHistory>(ThreadEnum::APPLICATION_THREAD),
      EventAnswerReceiver<EventRemoveBatch>(ThreadEnum::APPLICATION_THREAD),
      EventAnswerReceiver<EventRemoveAll>(ThreadEnum::APPLICATION_THREAD) {
      }
};

class CallHistoryXWalkController :
  public WrtDeviceApis::Commons::EventListener<EventCallHistoryListener>
{
  public:
    static CallHistoryXWalkController& getInstance() {
      static CallHistoryXWalkController controller;
      return controller;
    }

    void onAnswerReceived(const EventCallHistoryListenerPtr& event){
      printf("answer received!\n");
    }

  protected:
    CallHistoryXWalkController():WrtDeviceApis::Commons::EventListener<EventCallHistoryListener>(WrtDeviceApis::Commons::ThreadEnum::APPLICATION_THREAD){
    }
};

ICallHistoryPtr callHistory = CallHistoryFactory::getInstance().getCallHistoryObject();
CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<CallHistoryContext>::Initialize();
}

DPL::Thread* appThread = WrtDeviceApis::Commons::ThreadPool::getInstance().getThreadRef(WrtDeviceApis::Commons::ThreadEnum::APPLICATION_THREAD);
CallHistoryContext::CallHistoryContext(ContextAPI* api)
  : api_(api) {
    appThread->Run();
    WrtDeviceApis::Commons::ThreadPool::getInstance().getThreadRef(WrtDeviceApis::Commons::ThreadEnum::CALLHISTORY_THREAD)->Run();
    PlatformInitialize();
  }

CallHistoryContext::~CallHistoryContext() {
  PlatformUninitialize();
}

const char CallHistoryContext::name[] = "tizen.callhistory";

// This will be generated from call_history_api.js.
extern const char kSource_call_history_api[];

const char* CallHistoryContext::GetJavaScript() {
  return kSource_call_history_api;
}

static void deleteMessage(void* api, void* message) {
  delete (picojson::value*)message;
}
static std::string long2string(long l){
  std::string number;
  std::stringstream strstream;
  strstream << l;
  strstream >> number;
  return number;
}
static void processMessage(void* api, void* message) {
  picojson::value* msg = (picojson::value*)message;
  if (msg->get("cmd").to_str() ==  "find") {
    EventFindCallHistoryPtr event(new EventFindCallHistory());
    //event->setForSynchronousCall();
    event->setForAsynchronousCall(&CallHistoryXWalkResponseDispatcher::getInstance());
    callHistory->find(event);
    /*if (event->getResult()->size() == 0)
      return;*/
    /*printf("Entries: %u\n", event->getResult()->size());//->first()->getEntryId());
      printf("Entry ID: %u\n", event->getResult()->at(0)->getEntryId());//->first()->getEntryId());
      printf("Service ID: %s\n", event->getResult()->at(0)->getServiceId().c_str());//->first()->getEntryId());
      printf("Call type : %s\n", event->getResult()->at(0)->getCallType().c_str());//->first()->getEntryId());
      printf("Tags : %d\n", event->getResult()->at(0)->getTags()->size());//->first()->getEntryId());
      printf("Tag 0 : %s\n", event->getResult()->at(0)->getTags()->at(0).c_str());//->first()->getEntryId());
      printf("Remote parties : %d\n", event->getResult()->at(0)->getRemoteParties()->size());//->first()->getEntryId());
      printf("Remote party 0 : %s, person id 0:\n", event->getResult()->at(0)->getRemoteParties()->at(0)->getRemoteParty().c_str(),  event->getResult()->at(0)->getRemoteParties()->at(0)->getPersonId().c_str());//->first()->getEntryId());
      printf("Start time : %d\n", event->getResult()->at(0)->getStartTime());
      printf("Duration : %d\n", event->getResult()->at(0)->getDuration());
      printf("End Reason : %s\n", event->getResult()->at(0)->getEndReason().c_str());//->first()->getEntryId());
      printf("Direction: %s\n", event->getResult()->at(0)->getDirection().c_str());//->first()->getEntryId());
      printf("Recording: %d\n", event->getResult()->at(0)->getRecording()->size());//->first()->getEntryId());
      printf("Cost : %d\n", event->getResult()->at(0)->getCost());
      printf("Currency : %s\n", event->getResult()->at(0)->getCurrency().c_str());
      printf("Filter Mark : %d\n", event->getResult()->at(0)->getFilterMark());*/
    /*picojson::value result = picojson::value(picojson::object());
      picojson::object& output_map = result.get<picojson::object>();
      output_map["remote_party"] = picojson::value(event->getResult()->at(0)->getRemoteParties()->at(0)->getRemoteParty());
      output_map["uid"] = picojson::value(static_cast<double>(event->getResult()->at(0)->getEntryId()));
      output_map["_reply_id"] = picojson::value(msg->get("_reply_id"));
      ((ContextAPI*)api)->PostMessage(result.serialize().c_str());*/
  }
  else if (msg->get("cmd").to_str() ==  "remove") {
    printf("removing %s\n", msg->get("uid").to_str().c_str());
    callHistory->remove(static_cast<long>(msg->get("uid").get<double>()));
  }

}
DPL::WaitableEvent* wait = new DPL::WaitableEvent;
static void processSyncMessage(void* api, void* message) {
  picojson::value* msg = (picojson::value*)message;
  if (msg->get("cmd").to_str() ==  "addChangeListener") {
    EventCallHistoryListenerEmitterPtr emitter(new EventCallHistoryListenerEmitter);
    emitter->setListener(&CallHistoryXWalkController::getInstance());
    long id = callHistory->addListener(emitter);
    ((ContextAPI*)api)->SetSyncReply(long2string(id).c_str());
  }
  wait->Signal();
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
  appThread->PushEvent(api_, &processMessage , &deleteMessage,  &v);

}
void CallHistoryContext::HandleSyncMessage(const char* message) {
  picojson::value& v = *new picojson::value();

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring sysc message.\n";
    return;
  }

  appThread->PushEvent(api_, &processSyncMessage, &deleteMessage, &v);
  DPL::WaitForSingleHandle(wait->GetHandle());
}

