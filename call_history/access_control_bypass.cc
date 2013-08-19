
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

extern "C" __attribute__((visibility("default"))) 
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
