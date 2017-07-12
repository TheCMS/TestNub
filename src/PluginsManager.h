#ifndef __PLUGINS_MANAGER_H__
#define __PLUGINS_MANAGER_H__

#include "IAutoTestPlugin.h"
#include "AutoTestLogging.h"
#include <vector>

class PluginsManager : public IAutoEventsListener
{
private:
        PluginsManager();
        ~PluginsManager();
public:

static PluginsManager* GetInstance();
static void Destroy();


        void setModulesLoggerCb(autoTestLoggingCb cb);
        void setStdCb(autoTestLoggingCb cb);

        void loadPluginsFromFile(const char* filePath);
        void beginInterractiveMode();
        IAutoTestPlugin*    getPlugin(const char* pluginName);
        std::vector<IAutoTestPlugin*>&   getLoadedPlugins();

        void onEvtOccurred(const char* evtName, const char* evtOut);


private:

            void    unloadPlugins();
            void    loadPlugin(const char* libPath);

            void    loadInternalPlugins();

            void    writeConsoleMessage();

        autoTestLoggingCb m_pModulesCb;
        autoTestLoggingCb m_pLog;

        std::vector<IAutoTestPlugin*> m_pPlugins;
        std::vector<plugin_init_fct> m_initFcts;
        std::vector<plugin_destroy_fct> m_destroyFcts;
        std::vector<void*> m_dlHnds;
        std::vector<std::string> m_szLibsPath;

static  PluginsManager* s_pInstance;

};

#endif
