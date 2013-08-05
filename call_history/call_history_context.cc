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
using namespace DeviceAPI::CallHistory;
class HistoryThread: public DPL::Thread {
    protected:
    /*virtual int ThreadEntry() {
        ICallHistoryPtr callHistory(CallHistoryFactory::getInstance().getCallHistoryObject());
        printf("Got call history ptr\n");
        EventFindCallHistoryPtr event(new EventFindCallHistory());
        event->setForSynchronousCall();
        callHistory->find(event);
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
    }*/
};
CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<CallHistoryContext>::Initialize();
}

DPL::Thread* callHistoryThread = new HistoryThread();
CallHistoryContext::CallHistoryContext(ContextAPI* api)
  : api_(api) {
  printf("Creating call history context\n");
  callHistoryThread->Run();
  PlatformInitialize();
}

CallHistoryContext::~CallHistoryContext() {
  PlatformUninitialize();
}

const char CallHistoryContext::name[] = "tizen.callhistory";

// This will be generated from call_history_api.js.
extern const char kSource_call_history_api[];

const char* CallHistoryContext::GetJavaScript() {
  printf("Getting callhistory javascript \n");
  return kSource_call_history_api;
}

static void deleteMessage(void* api, void* message) {
    printf("Deleting message...\n");
    delete (picojson::value*)message;
}
static void processMessage(void* api, void* message) {
    picojson::value* msg = (picojson::value*)message;
    if (msg->get("cmd").to_str() ==  "find") {
        ICallHistoryPtr callHistory(CallHistoryFactory::getInstance().getCallHistoryObject());
        printf("Got call history ptr\n");
        EventFindCallHistoryPtr event(new EventFindCallHistory());
        event->setForSynchronousCall();
        callHistory->find(event);
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
        picojson::value result = picojson::value(picojson::object());
        picojson::object& output_map = result.get<picojson::object>();
        output_map["remote_party"] = picojson::value(event->getResult()->at(0)->getRemoteParties()->at(0)->getRemoteParty());
        output_map["_reply_id"] = picojson::value(msg->get("_reply_id"));
        ((ContextAPI*)api)->PostMessage(result.serialize().c_str());
    }

}
DPL::WaitableEvent* wait = new DPL::WaitableEvent;
static void processSyncMessage(void* api, void* message) {
    picojson::value* msg = (picojson::value*)message;
    if (msg->get("cmd").to_str() ==  "addChangeListener") {
        ((ContextAPI*)api)->SetSyncReply("123456");
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
  callHistoryThread->PushEvent(api_, &processMessage , &deleteMessage,  &v);

}
void CallHistoryContext::HandleSyncMessage(const char* message) {
  printf("handling sync message: %s\n", message);
  picojson::value& v = *new picojson::value();

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring sysc message.\n";
    return;
  }

  callHistoryThread->PushEvent(api_, &processSyncMessage, &deleteMessage, &v);
  DPL::WaitForSingleHandle(wait->GetHandle());
}

