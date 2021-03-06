// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_

#if defined(GENERIC_DESKTOP)
#include <gio/gio.h>
#elif defined(TIZEN_MOBILE)
#include <glib.h>
#endif
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

enum SystemInfoNetworkType {
  SYSTEM_INFO_NETWORK_NONE = 0,
  SYSTEM_INFO_NETWORK_2G,
  SYSTEM_INFO_NETWORK_2_5G,
  SYSTEM_INFO_NETWORK_3G,
  SYSTEM_INFO_NETWORK_4G,
  SYSTEM_INFO_NETWORK_WIFI,
  SYSTEM_INFO_NETWORK_ETHERNET,
  SYSTEM_INFO_NETWORK_UNKNOWN
};

#if defined(GENERIC_DESKTOP)
#define G_CALLBACK_1(METHOD, SENDER, ARG0)                                   \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {  \
    return reinterpret_cast<SysInfoNetwork*>(userdata)->METHOD(sender, res); \
  }                                                                          \
                                                                             \
  void METHOD(SENDER, ARG0);
#endif

class SysInfoNetwork {
 public:
  explicit SysInfoNetwork(ContextAPI* api);
  ~SysInfoNetwork();
  void Get(picojson::value& error, picojson::value& data);
  inline void StartListening() {
#if defined(TIZEN_MOBILE)
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoNetwork::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
#endif
  }
  inline void StopListening() {
#if defined(TIZEN_MOBILE)
    if (timeout_cb_id_ > 0) {
      g_source_remove(timeout_cb_id_);
    }
#endif
}

 private:
  void PlatformInitialize();

  bool Update(picojson::value& error);
  void SetData(picojson::value& data);
  std::string ToNetworkTypeString(SystemInfoNetworkType type);

  ContextAPI* api_;
  SystemInfoNetworkType type_;

#if defined(GENERIC_DESKTOP)
  G_CALLBACK_1(OnNetworkManagerCreated, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnActiveConnectionCreated, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnDevicesCreated, GObject*, GAsyncResult*);

  void UpdateActiveConnection(GVariant* value);
  void UpdateActiveDevice(GVariant* value);
  void UpdateDeviceType(GVariant* value);
  void SendUpdate(guint new_device_type);

  static void OnNetworkManagerSignal(GDBusProxy* proxy, gchar* sender,
      gchar* signal, GVariant* parameters, gpointer user_data);
  static void OnActiveConnectionsSignal(GDBusProxy* proxy, gchar* sender,
      gchar* signal, GVariant* parameters, gpointer user_data);

  SystemInfoNetworkType ToNetworkType(guint device_type);

  std::string active_connection_;
  std::string active_device_;
  guint device_type_;
#elif defined(TIZEN_MOBILE)
  static gboolean OnUpdateTimeout(gpointer user_data);
  int timeout_cb_id_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_
