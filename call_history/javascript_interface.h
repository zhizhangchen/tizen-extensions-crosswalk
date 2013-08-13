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
 * @file       JavaScriptInterface.h
 * @author     Przemyslaw Dobrowolski (p.dobrowolsk@samsung.com)
 * @author     Piotr Fatyga (p.fatyga@samsung.com)
 * @version    0.1
 * @brief
 */
#ifndef WRT_SRC_PLUGIN_SERVICE_WEBKIT_INTERFACE_H_
#define WRT_SRC_PLUGIN_SERVICE_WEBKIT_INTERFACE_H_

#include <string>
#include <vector>
#include <memory>
#include <list>
#include <dpl/noncopyable.h>
#include <dpl/singleton.h>
#include <Commons/JSObjectDeclaration.h>
#include <Commons/JSObject.h>

//forward declaration of JSConectexRef
extern "C" {
    typedef const struct OpaqueJSContext* JSContextRef;
    typedef struct OpaqueJSContext* JSGlobalContextRef;
    typedef struct OpaqueJSValue* JSObjectRef;
}

class JavaScriptInterface : DPL::Noncopyable
{
  public:

    typedef std::vector<std::string> PropertiesList;

    typedef std::list<JSObjectPtr> ObjectsList;
    typedef std::shared_ptr<ObjectsList> ObjectsListPtr;

  public:
    JSObjectPtr getGlobalObject(JSGlobalContextRef context) const;

    // object creation
    JSObjectPtr createObject(JSGlobalContextRef context,
                             const JSObjectDeclarationPtr& declaration);

    //properties
    void setObjectProperty(JSGlobalContextRef context,
                           const JSObjectPtr& parentObject,
                           const std::string &propertyName,
                           const JSObjectPtr& propertyObject);

    void removeObjectProperty(JSGlobalContextRef context,
                              const JSObjectPtr& parentObject,
                              const std::string &propertyName);

    PropertiesList getObjectPropertiesList(JSGlobalContextRef context,
                                           const JSObjectPtr& object) const;

    JSObjectPtr copyObjectToIframe(JSGlobalContextRef context,
                                   const JSObjectPtr& iframe,
                                   const std::string& name);

    ObjectsListPtr getIframesList(JSGlobalContextRef context) const;

    void removeIframes(JSGlobalContextRef context);

    void invokeGarbageCollector(JSGlobalContextRef context);

    JSObjectPtr getJSObjectProperty(JSGlobalContextRef ctx,
                            const JSObjectPtr& frame,
                            const std::string& name) const;

  private:
    JavaScriptInterface()
    {
    }

    JSObjectPtr makeJSFunctionObject(
            JSGlobalContextRef context,
            const std::string &name,
            js_function_impl functionImplementation) const;
    JSObjectPtr makeJSClassObject(
            JSGlobalContextRef context,
            JSObjectDeclaration::ConstClassTemplate classTemplate) const;
    JSObjectPtr makeJSObjectBasedOnInterface(
            JSGlobalContextRef context,
            const std::string &interfaceName) const;
    JSObjectPtr makeJSInterface(
            JSGlobalContextRef context,
            JSObjectDeclaration::ConstClassTemplate classTemplate,
            JSObjectDeclaration::ConstructorCallback constructorCallback) const;

    ObjectsListPtr getIframesList(JSContextRef context,
            JSObjectRef object) const;

    friend class DPL::Singleton<JavaScriptInterface>;
};

typedef DPL::Singleton<JavaScriptInterface> JavaScriptInterfaceSingleton;

#endif
