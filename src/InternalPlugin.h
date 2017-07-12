
#ifndef _INTERNAL_PLUGIN_H_
#define _INTERNAL_PLUGIN_H_

#include "IAutoTestPlugin.h"
#include "IActionResolver.h"


#include <pthread.h>
#include <vector>
#include <string>

class InternalPlugin: public IAutoTestPlugin, public IActionResolver
{
public:

            InternalPlugin();

    virtual ~InternalPlugin();

    virtual     void    runCLI();
    virtual     std::vector<auto_grammar>&   getModuleGrammars();
    virtual     void    setEventsListener(IAutoEventsListener* wpListener);
    virtual     IActionResolver*  getActionResolver();
    virtual     void    setLoggingCb(autoTestLoggingCb );
    virtual     const char*     getPluginName();

                void  startEventsThread();
    virtual int runAction(const char* cmdName, const char* arg);

private:

    static  InternalPlugin* s_pInstance;
    void  fireEvents();
    void  queueEvent(std::string evt, std::string arg);


    void  createGrammar();
    void  printHelper();

    static  void*   runEventsNotifierThread(void*);

    static  bool    s_bMainLoopRunning;

    pthread_t   m_eventsThread;
    pthread_mutex_t m_evtQueueMutex;

    std::vector<std::pair<std::string, std::string> > m_eventsQueue;
    std::vector<std::pair<std::string, long> > m_timers;

    autoTestLoggingCb           m_pLog;
    IAutoEventsListener*        m_wpListener;
    std::vector<auto_grammar>   m_gramms;
};

#endif
