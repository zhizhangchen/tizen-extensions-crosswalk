// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_context.h"
#include "common/picojson.h"

#if defined(TIZEN_MOBILE)
#include <bluetooth.h>
#endif

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
#if defined(TIZEN_MOBILE)
  int init = bt_initialize();
  if (init != BT_ERROR_NONE)
    g_printerr("\n\nCouldn't initialize Bluetooth module.");
  else {
    bt_adapter_state_e bt_state = BT_ADAPTER_DISABLED;
    bt_adapter_get_state(&bt_state);

    // FIXME(jeez): As soon as we have fixed adapter.setPowered(true/false)
    // we should stop calling bt_adapter_enable() at this point.
    if (bt_state == BT_ADAPTER_DISABLED) {
      int ret = bt_adapter_enable();
      if (ret != BT_ERROR_NONE)
        g_printerr("\n\nFailed to enable bluetooth adapter [%d].", ret);
    }
  }
#endif

  return ExtensionAdapter<BluetoothContext>::Initialize();
}

BluetoothContext::BluetoothContext(ContextAPI* api)
    : api_(api) {
  PlatformInitialize();
}

const char BluetoothContext::name[] = "tizen.bluetooth";

extern const char kSource_bluetooth_api[];

const char* BluetoothContext::GetJavaScript() {
  return kSource_bluetooth_api;
}

void BluetoothContext::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "DiscoverDevices")
    HandleDiscoverDevices(v);
  else if (cmd == "StopDiscovery")
    HandleStopDiscovery(v);
  else if (cmd == "SetAdapterProperty")
    HandleSetAdapterProperty(v);
  else if (cmd == "CreateBonding")
    HandleCreateBonding(v);
  else if (cmd == "DestroyBonding")
    HandleDestroyBonding(v);
}

void BluetoothContext::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring Sync message.\n";
    return;
  }

  SetSyncReply(v);
}

void BluetoothContext::HandleDiscoverDevices(const picojson::value& msg) {
  discover_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_)
    g_dbus_proxy_call(adapter_proxy_, "StartDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStartedThunk, this);
}

void BluetoothContext::HandleStopDiscovery(const picojson::value& msg) {
  stop_discovery_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_)
    g_dbus_proxy_call(adapter_proxy_, "StopDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStoppedThunk, this);
}

void BluetoothContext::OnDiscoveryStarted(GObject*, GAsyncResult* res) {
  GError* error = 0;

  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(discover_callback_id_);

  int errorCode = 0;
  if (!result) {
    g_printerr ("Error discovering: %s\n", error->message);
    g_error_free(error);
    errorCode = 1;
  }

  o["error"] = picojson::value(static_cast<double>(errorCode));

  picojson::value v(o);
  PostMessage(v);

  if (result)
    g_variant_unref(result);
}

void BluetoothContext::OnDiscoveryStopped(GObject* source, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  int errorCode = 0;
  if (!result) {
    g_printerr ("Error discovering: %s\n", error->message);
    g_error_free(error);
    errorCode = 1;
  }

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(stop_discovery_callback_id_);
  o["error"] = picojson::value(static_cast<double>(errorCode));
  picojson::value v(o);
  PostMessage(v);
}

void BluetoothContext::FlushPendingMessages() {
  // Flush previous pending messages.
  if (!queue_.empty()) {
    MessageQueue::iterator it;
    for (it = queue_.begin(); it != queue_.end(); ++it)
      api_->PostMessage((*it).serialize().c_str());
  }
}

void BluetoothContext::PostMessage(picojson::value v) {
  // If the JavaScript 'context' hasn't been initialized yet (i.e. the C++
  // backend was loaded and it is already executing but
  // tizen.bluetooth.getDefaultAdapter() hasn't been called so far), we need to
  // queue the PostMessage calls and defer them until the default adapter is set
  // on the JS side. That will guarantee the correct callbacks will be called,
  // and on the right order, only after tizen.bluetooth.getDefaultAdapter() is
  // called.
  if (!is_js_context_initialized_) {
    queue_.push_back(v);
    return;
  }

  FlushPendingMessages();
  api_->PostMessage(v.serialize().c_str());
}

void BluetoothContext::SetSyncReply(picojson::value v) {
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetDefaultAdapter")
    api_->SetSyncReply(HandleGetDefaultAdapter(v).serialize().c_str());

  FlushPendingMessages();
}
