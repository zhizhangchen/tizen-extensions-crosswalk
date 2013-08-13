/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
/**
 * @file        plugin.h
 * @author      Przemyslaw Dobrowolski (p.dobrowolsk@samsung.com)
 * @version     1.0
 * @brief       This file is the implementation file of plugin
 */
#ifndef WRT_SRC_PLUGIN_SERVICE_PLUGIN_H_
#define WRT_SRC_PLUGIN_SERVICE_PLUGIN_H_

#include <list>
#include <map>
#include <string>
#include <dpl/atomic.h>
#include <dpl/shared_ptr.h>
#include <dpl/noncopyable.h>
#include <wrt_plugin_export.h>
#include <Commons/JSObjectDeclaration.h>

class Plugin;
typedef DPL::SharedPtr<Plugin> PluginPtr;

class Plugin : private DPL::Noncopyable
{
  public:
    typedef JSObjectDeclaration Class;
    typedef JSObjectDeclarationPtr ClassPtr;
    typedef std::list<ClassPtr> ClassList;
    typedef DPL::SharedPtr<ClassList> ClassPtrList;

  private:
    ///< Plug-in identifier. Currently plug-in file name is used as the ID
    std::string m_fileName;

    ///< Handle for the plug-in library. A plug-in is a dynamic loadable library
    void* m_libHandle;

    // Plugin API
    on_widget_start_proc* m_apiOnWidgetStart;
    on_widget_init_proc* m_apiOnWidgetInit;
    on_widget_stop_proc* m_apiOnWidgetStop;
    on_frame_load_proc* m_apiOnFrameLoad;
    on_frame_unload_proc* m_apiOnFrameUnload;
    const ClassPtrList m_apiClassList;

    Plugin(const std::string &fileName,
            void *libHandle,
            on_widget_start_proc* apiOnWidgetStart,
            on_widget_init_proc* apiOnWidgetInit,
            on_widget_stop_proc* apiOnWidgetStop,
            on_frame_load_proc* apiOnFrameLoad,
            on_frame_unload_proc* apiOnFrameUnload,
            const ClassPtrList &apiClassList);

  public:
    virtual ~Plugin();

    // Loading
    static PluginPtr LoadFromFile(const std::string &fileName);

    // Filename
    std::string GetFileName() const;

    // API
    void OnWidgetStart(int widgetId);

    void OnWidgetInit(feature_mapping_interface_t *interface);

    void OnWidgetStop(int widgetId);

    void OnFrameLoad(java_script_context_t context);

    void OnFrameUnload(java_script_context_t context);

    const ClassPtrList GetClassList() const;
};

#endif // PLUGIN_H
