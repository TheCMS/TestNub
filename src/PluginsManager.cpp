#include "PluginsManager.h"
#include "AutoTestHelperFuncs.h"
#include "InternalPlugin.h"


#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#ifdef WIN32

static int hndCnt=0;
void* dlopen(const char* path, int flags)
{
    (void) path;
    (void) flags;
    int* hnd= new int();
    *hnd = hndCnt++;
    return hnd;
}
void dlclose(void * hnd)
{
    (void)hnd;
}

void* genericFct(void * ) {return NULL;}

void* dlsym(void* hnd,const char* symbol)
{
    (void)hnd;
    (void)symbol;
    return (void*)genericFct;
}

char err_stub_msg[125]="stubbed dl implementation";

char* dlerror()
{
    return err_stub_msg;
}

#define RTLD_NOW (1<< 2)
#define RTLD_LOCAL (1<< 3)
#define RTLD_GLOBAL (1 << 4)

#else
#include <dlfcn.h>
#endif


PluginsManager* PluginsManager::s_pInstance = NULL;

PluginsManager::PluginsManager()
{
}

PluginsManager::~PluginsManager()
{
}

PluginsManager* PluginsManager::GetInstance()
{
    if (s_pInstance==NULL)
    {
        s_pInstance = new PluginsManager();
    }
    return s_pInstance;
}

void PluginsManager::Destroy()
{
    s_pInstance->unloadPlugins();
    delete s_pInstance;
    s_pInstance=NULL;
}

void PluginsManager::setModulesLoggerCb(autoTestLoggingCb cb)
{
    m_pModulesCb=cb;
}

void PluginsManager::setStdCb(autoTestLoggingCb cb)
{
    m_pLog=cb;
}

void PluginsManager::loadInternalPlugins()
{
    InternalPlugin* pPlug = new InternalPlugin();
    pPlug->setLoggingCb(m_pModulesCb);
    pPlug->startEventsThread();
    m_pPlugins.push_back(pPlug);
    m_szLibsPath.push_back("internal");

    m_initFcts.push_back(NULL);
    m_destroyFcts.push_back(NULL);
    m_dlHnds.push_back(NULL);

    m_pLog(ALL_INFO,"PMGR","Internals plugin loaded");
}

void PluginsManager::loadPluginsFromFile(const char* filePath)
{

    loadInternalPlugins();

    FILE* f =fopen(filePath,"rb");

    if (f==NULL)
    {
        m_pLog(ALL_ERROR,"PMGR","Cannot open plugin file %s.",filePath);
        return;
    }

    char* line=NULL;
    size_t len=0;
    ssize_t read;

    while ((read = getline(&line,&len,f)) != -1 )
    {

        loadPlugin(line);
        if (line)
        {
            free(line);
        }
        line = NULL;
    }
    fclose(f);

}

void PluginsManager::writeConsoleMessage()
{
    std::cout<<"**************************"<<std::endl;
    std::cout<<m_pPlugins.size()<<" plugins succesfully loaded."<<" Choose Plugin to activate."<<std::endl;

    for (unsigned int i=0;i<m_pPlugins.size();i++)
    {
        std::cout<<i<<" : "<<m_pPlugins[i]->getPluginName()<<std::endl;
    }
    std::cout<<"L : List Loaded Plugins and grammars."<<std::endl;
    std::cout<<"G : Detail Grammar. Requires Grammar Name as parameter."<<std::endl;
    std::cout<<"E : Exit."<<std::endl;
}

void PluginsManager::onEvtOccurred(const char* evtName, const char* evtOut)
{
   m_pModulesCb(ALL_INFO,"EVTS","Event received:[%s] %s ",evtName,evtOut);
}

void PluginsManager::beginInterractiveMode()
{
    // CMS to do
    while (true)
    {
        writeConsoleMessage();
        std::string option;
        std::getline(std::cin, option);

        if(option.length()>0)
        {
            char opt = option[0];
            std::vector<std::string> args;
            splitString(option," ",args);
            if (opt == 'E' || opt == 'e')
            {
                std::cout<<"Bye bye!"<<std::endl;
                break;
            } else if (opt == 'L' || opt == 'l')
            {
                std::cout<<"***********************************"<<std::endl;
                std::cout<<"Listing loaded modules and grammars:"<<std::endl;
                for (unsigned int i=0;i<m_pPlugins.size();i++)
                {
                    IAutoTestPlugin* pPlug = m_pPlugins[i];
                    std::vector<auto_grammar>& grams(pPlug->getModuleGrammars());
                    std::cout<<"Plugin "<<pPlug->getPluginName()<<": "<<grams.size()<<" grammars"<<std::endl;
                    for (unsigned int j=0;j<grams.size();j++)
                    {
                        auto_grammar& g(grams[j]);
                        std::cout<<j+1<<": "<<g.grammar_name<<" commands "<<g.commands.size()<<" events "<<g.events.size();
                    }
                }
                std::cout<<std::endl;

            } else if (opt == 'G' || opt == 'G')
            {
                if (args.size() > 1)
                {
                    // find grammar;

                    auto_grammar g;
                    bool bFound=false;
                    for (unsigned int i=0;i<m_pPlugins.size();i++)
                    {
                        std::vector<auto_grammar>& grams(m_pPlugins[i]->getModuleGrammars());
                        for (unsigned j=0;j<grams.size();j++)
                        {
                            if (grams[j].grammar_name == args[1])
                            {
                                bFound=true;
                                g = grams[j];
                                break;
                            }
                        }
                        if (bFound)
                        {
                            break;
                        }
                    }
                    if (!bFound)
                    {
                        std::cout<<"Grammar named "<<args[1]<<" NOT FOUND."<<std::endl;
                    } else {
                        std::cout<<"Listing possibilities for grammar"<<args[1]<<std::endl;
                        std::cout<<"**Commands:"<<std::endl;
                        for (unsigned int i=0;i<g.commands.size();i++)
                        {
                            std::cout<<g.commands[i]<<std::endl;
                        }
                        std::cout<<std::endl;
                        std::cout<<"**Events:"<<std::endl;
                        for (unsigned int i=0;i<g.events.size();i++)
                        {
                            std::cout<<g.events[i]<<std::endl;
                        }
                    }

                } else {
                    std::cout<<"Grammar detail requires grammar name as parameter"<<std::endl;
                }

            }  else {
                unsigned int value = atoi(option.c_str());
                if (value < m_pPlugins.size())
                {
                    std::cout<<"Activating CLI for plugin ["<< value <<"] "<<m_pPlugins[value]->getPluginName()<<std::endl;
                    m_pPlugins[value]->setEventsListener(this);
                    m_pPlugins[value]->runCLI();
                } else {
                    std::cout<<"Invalid option: "<<option<<". Please enter a value between 0 and "<<m_pPlugins.size()-1<<std::endl;
                }
            }
        } else {
            std::cout<<"Invalid input. Please pick a valid entry!"<<std::endl;
        }
    }

}

IAutoTestPlugin* PluginsManager::getPlugin(const char* pluginName)
{
    for (unsigned int i=0;i<m_pPlugins.size();i++)
    {
        IAutoTestPlugin* ptr  = m_pPlugins[i];
        if(strcmp(ptr->getPluginName(),pluginName)==0)
        {
            return ptr;
        }
    }
    return NULL;
}

std::vector<IAutoTestPlugin*>& PluginsManager::getLoadedPlugins()
{
    return m_pPlugins;
}

void PluginsManager::unloadPlugins()
{
    for (unsigned int i=0;i<m_pPlugins.size();i++)
    {
        if (m_destroyFcts[i]!=NULL)
        {
            m_destroyFcts[i]();
            dlclose(m_dlHnds[i]);
        }
    }
    m_pPlugins.clear();;
    m_initFcts.clear();
    m_destroyFcts.clear();
    m_dlHnds.clear();
    m_szLibsPath.clear();
}

void PluginsManager::loadPlugin(const char* libPath)
{
    std::string libPathStr= std::string(libPath);
    trim(libPathStr);
    if (libPathStr.length()<2)
        return;
    m_pLog(ALL_INFO,"PMGR","Loading plugin [%s]",libPathStr.c_str());


    void* hnd=dlopen(libPathStr.c_str(), RTLD_NOW);

    if ( hnd==NULL)
    {
        m_pLog(ALL_ERROR,"PMGR","Error loading plugin %s with error %s",libPathStr.c_str(),dlerror());
        return;
    }

    plugin_init_fct init =(plugin_init_fct) dlsym(hnd,XSTR(PLUGIN_INIT_FCT_NAME));
    if (init == NULL)
    {
        m_pLog(ALL_ERROR,"PMGR","Cannot find required init method: %s in plugin %s.",XSTR(PLUGIN_INIT_FCT_NAME),libPathStr.c_str());
        dlclose(hnd);
        return;
    }
    plugin_destroy_fct destr= (plugin_destroy_fct) dlsym(hnd,XSTR(PLUGIN_DESTROY_FCT_NAME));
    if (destr == NULL)
    {
        m_pLog(ALL_ERROR,"PMGR","Cannot find required destroy method: %s in plugin %s.",XSTR(PLUGIN_DESTROY_FCT_NAME),libPathStr.c_str());
        dlclose(hnd);
        return;
    }

    m_pLog(ALL_INFO,"PMGR","Pluginloaded. Beggining initialization.");
    IAutoTestPlugin* pPlug = init();
    if (pPlug==NULL)
    {
        m_pLog(ALL_ERROR,"PMGR","Plugin %s failed to initialize!",libPathStr.c_str());
        dlclose(hnd);
        return;
    }
    // success
    // keep references to variables

    m_pLog(ALL_INFO,"PMGR","Pluginallocated. Creating entries.");
    pPlug->setLoggingCb(m_pModulesCb);

    m_pPlugins.push_back(pPlug);
    m_initFcts.push_back(init);
    m_destroyFcts.push_back(destr);
    m_dlHnds.push_back(hnd);
    m_szLibsPath.push_back(libPath);
    m_pLog(ALL_INFO,"PMGR","Plugin finished intialization.");
}
