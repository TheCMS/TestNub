#ifndef __I_AUTO_TEST_PLUGIN_H__
#define __I_AUTO_TEST_PLUGIN_H__

#include "IAutoGrammar.h"
#include "IAutoCLIInfo.h"
#include "IActionResolver.h"
#include "IAutoEventsListener.h"
#include "AutoTestLogging.h"

#define PLUGIN_INIT_FCT_NAME initPlugin
#define PLUGIN_DESTROY_FCT_NAME destroyPlugin

#define XSTR(x) STRINGIFY(x)
#define STRINGIFY(x) #x

class IAutoTestPlugin
{
public:

    virtual ~IAutoTestPlugin(){}

    virtual     void    runCLI()=0;
    virtual     std::vector<auto_grammar>&   getModuleGrammars()=0;
    virtual     void    setEventsListener(IAutoEventsListener* wpListener)=0;
    virtual     IActionResolver*  getActionResolver()=0;
    virtual     void    setLoggingCb(autoTestLoggingCb )=0;
    virtual     const char*     getPluginName()=0;

};

typedef IAutoTestPlugin* (*plugin_init_fct)();
typedef void(*plugin_destroy_fct)();


#endif
