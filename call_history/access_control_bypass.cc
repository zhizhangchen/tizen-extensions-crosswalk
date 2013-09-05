
#include <JavaScriptCore/JSObjectRef.h>
#include <stdio.h>
#include "plugin.h"
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/JSObjectRef.h>
#include "javascript_interface.h"
#include "dpl/foreach.h"
#include <dpl/scoped_array.h>
#include <Commons/FunctionDeclaration.h>
#include <Commons/WrtAccess/WrtAccess.h>
#include <WidgetDB/IWidgetDB.h>
#include <WidgetDB/WidgetDBMgr.h>
#include <dpl/wrt-dao-ro/widget_dao_read_only.h>
#define PUBLIC_EXPORT __attribute__((visibility("default")))
  typedef enum
  {
    ACE_OK,                 // Operation succeeded
    ACE_INVALID_ARGUMENTS,  // Invalid input parameters
    ACE_INTERNAL_ERROR,     // ACE internal error
    ACE_ACE_UNKNOWN_ERROR   // Unexpected operation
  } ace_return_t;
  typedef enum
  {
    ACE_FALSE,
    ACE_TRUE
  } ace_bool_t;

extern "C" PUBLIC_EXPORT
ace_return_t ace_check_access(const ace_request_t* request, ace_bool_t* access) {
    *access = ACE_TRUE;
    printf("########################\nbypassing ace_check_access \n########################\n");
    return ACE_OK; 
  }
template <typename FunctionGetter,
          typename ArgumentsVerifier,
          typename ... Args>
WrtDeviceApis::Commons::AceSecurityStatus aceCheckAccess(
        const FunctionGetter& f,
        const char* functionName,
        Args && ... args)
{
    printf("########################\nbypassing access check of %s\n########################\n", functionName);
    return WrtDeviceApis::Commons::AceSecurityStatus::AccessGranted;
}       
bool WrtDeviceApis::Commons::WrtAccess::checkAccessControl(const AceFunction& aceFunction) const {
    printf("########################\nbypassing access controle check \n########################\n");
    return true;
}
namespace WrtDeviceApis {
  namespace WidgetDB {
    class XWalkTizenWidgetDB: public Api::IWidgetDB {
      private:
        int m_widgetId;
      public:
      explicit XWalkTizenWidgetDB(int widgetId):m_widgetId(widgetId) {
      }

      virtual int getWidgetId() const {
        return m_widgetId;
      }

      virtual std::string getLanguage() const {
        return "en_US";
      }

      virtual std::string getConfigValue(Api::ConfigAttribute attribute) const {
      }

      virtual std::string getUserAgent() const {
      }

      virtual Api::InstallationStatus checkInstallationStatus(
          const std::string& gid,
          const std::string& name,
          const std::string& version) const {
      }

      virtual Api::Features getWidgetFeatures() const {
      }

      virtual Api::Features getRegisteredFeatures() const {
      }

      virtual std::string getWidgetInstallationPath() const {
        return "/opt/usr/apps/8qsG1UI0Kh";
      }

      virtual std::string getWidgetPersistentStoragePath() const {
        return "/opt/usr/apps/8qsG1UI0Kh";
      }

      virtual std::string getWidgetTemporaryStoragePath() const {
        return "/opt/usr/apps/tmp/8qsG1UI0Kh";
      }



    };
    namespace Api {
      PUBLIC_EXPORT IWidgetDBPtr getWidgetDB(int widgetId)
      { 
            printf("########################\nbypassing getWidgetDB\n");
            return IWidgetDBPtr(new XWalkTizenWidgetDB(widgetId));
      }       
    } // Api
  } // WidgetDB
}; // WrtDeviceApis
namespace WrtDB PUBLIC_EXPORT {
  PUBLIC_EXPORT TizenPkgId WidgetDAOReadOnly::getTzAppId() const
  {
      printf("########################\nbypassing getTizenPkgId\n");
      return DPL::FromUTF8String("8qsG1UI0Kh");
  }

  namespace {
    TizenAppId getTizenAppIdByHandle (const DbWidgetHandle handle) {
      printf("########################\nbypassing getTizenAppIdByHandle\n");
    }
  }
}
