// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_locale.h"

#include <locale.h>

#include <stdlib.h>
#include <stdio.h>

#include <string>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

SysInfoLocale::SysInfoLocale(ContextAPI* api)
    : stopping_(false) {
  api_ = api;
}

SysInfoLocale::~SysInfoLocale() {
}

void SysInfoLocale::Get(picojson::value& error,
                       picojson::value& data) {
  // language
  if (!UpdateLanguage()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get language failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "language",
      picojson::value(language_));

  // timezone
  if (!UpdateCountry()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get timezone failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "country",
      picojson::value(country_));

  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value(""));
}

bool SysInfoLocale::UpdateLanguage() {
  std::string str;

  setlocale(LC_ALL, "");
  const struct lconv* const currentlocale = localeconv();
  std::string info = setlocale(LC_ALL, NULL);
  int pos = info.find('.', 0);
  str.assign(info, 0, pos);

  if (str.empty()) {
    return false;
  } else {
    language_ = str;
    return true;
  }
}

bool SysInfoLocale::UpdateCountry() {
  std::string str;

  FILE* fp = fopen("/etc/timezone", "r");
  std::string info;
  char* cinfo = NULL;
  size_t length = 100;

  getline(&cinfo, &length, fp);
  info = cinfo;
  free(cinfo);
  fclose(fp);

  int pos = info.find('/', 0);
  str.assign(info, pos + 1, info.length() - pos - 1);

  // FIXME (halton): Use city to get real country
  if (str.empty()) {
    return false;
  } else {
    country_ = str;
    return true;
  }
}

gboolean SysInfoLocale::OnUpdateTimeout(gpointer user_data) {
  SysInfoLocale* instance = static_cast<SysInfoLocale*>(user_data);

  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  std::string oldlanguage_ = instance->language_;
  std::string oldcountry_ = instance->country_;
  instance->UpdateLanguage();
  instance->UpdateCountry();

  if (oldlanguage_ != instance->language_ ||
      oldcountry_ != instance->country_) {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());

    system_info::SetPicoJsonObjectValue(data, "language",
        picojson::value(instance->language_));
    system_info::SetPicoJsonObjectValue(data, "country",
        picojson::value(instance->country_));
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("LOCALE"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}

