
#include <JavaScriptCore/JSObjectRef.h>
#include <stdio.h>

/*JS_EXPORT JSClassRef JSClassCreate(const JSClassDefinition* definition) {
  printf("Creating JS Class %s\n", definition->className);
  return NULL;
}
static void _init() __attribute__((constructor));
void _init() {
    printf("############################Loading hack.\n");
}*/
#include <Commons/FunctionDeclaration.h>
#include <Commons/WrtAccess/WrtAccess.h>

bool WrtDeviceApis::Commons::WrtAccess::checkAccessControl(const AceFunction& aceFunction) const {
    printf("########################\nbypassing access controle check \n########################\n");
    return true;
}

template <typename FunctionGetter,
          typename ArgumentsVerifier,
          typename ... Args>
WrtDeviceApis::Commons::AceSecurityStatus aceCheckAccess(
        const FunctionGetter& f,
        const char* functionName,
        Args && ... args)
{
/*    using namespace WrtDeviceApis::Commons;

    AceFunction aceFunction = f(functionName);
      
    ArgumentsVerifier argsVerify;
    argsVerify(aceFunction, args ...);
    
    if (!(WrtAccessSingleton::Instance().checkAccessControl(aceFunction))) {
        LogDebug("Function is not allowed to run");
        return AceSecurityStatus::AccessDenied;
    }
    LogDebug("Function accepted!");*/

    printf("########################\nbypassing access check of %s\n########################\n", functionName);
    return WrtDeviceApis::Commons::AceSecurityStatus::AccessGranted;
}       

extern "C" {
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

  ace_return_t ace_check_access(const ace_request_t* request, ace_bool_t* access) {
    *access = ACE_TRUE;
    printf("########################\nbypassing ace_check_access %s\n########################\n");
    return ACE_OK; 
  }
}
