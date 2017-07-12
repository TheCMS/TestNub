#include "InternalPlugin.h"
#include "AutoTestHelperFuncs.h"
#include "AutoTestLogging.h"

#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>


InternalPlugin* InternalPlugin::s_pInstance=NULL;

bool InternalPlugin::s_bMainLoopRunning;


void internalLoggingFct(eAutoLogLevels logLevel, const char* ctx, const char* fmt, ...)
{
    char buffer[2048];
    va_list lst;
    va_start (lst,fmt);

    vsnprintf(buffer,2048,fmt,lst);

    va_end(lst);

    std::cout<<"["<<(int)logLevel<<"] ["<<ctx<<"] "<<buffer<<std::endl;
}

InternalPlugin::InternalPlugin():
    m_pLog(internalLoggingFct)
{
    createGrammar();
    s_pInstance=this;

}

InternalPlugin::~InternalPlugin()
{
    pthread_join(m_eventsThread,NULL);
    s_pInstance=NULL;
}

void InternalPlugin::printHelper()
{
    m_pLog(ALL_INFO,"IPLG","*************************");
    m_pLog(ALL_INFO,"IPLG","Internal commands list: ");
    m_pLog(ALL_INFO,"IPLG","exit: exits internal plugin interractive mode");
    m_pLog(ALL_INFO,"IPLG","help: prints this help message");

    m_pLog(ALL_INFO,"IPLG","startTimer $NAME$: starts a timer identified by a string");
    m_pLog(ALL_INFO,"IPLG","stopTimer $NAME$: stops the timer with the specified name and shows the passed time");
    m_pLog(ALL_INFO,"IPLG","flushTimer $NAME$: shows the passed time, but doesn't stop the timer");
    m_pLog(ALL_INFO,"IPLG","stopAllTimers: stops all existing timers and shows the passed time for each");
    m_pLog(ALL_INFO,"IPLG","waitMS $AMOUNT$: waits for a given amount of miliseconds");
    m_pLog(ALL_INFO,"IPLG","message $MESSAGE$: generates an event with the given message");
}

void    InternalPlugin::runCLI()
{
    bool bExit=false;

    printHelper();
    while (!bExit)
    {
        std::string command;
        std::getline(std::cin,command);

        std::vector<std::string> splittedArgs;

        splitOnce(command," ",splittedArgs);


        if (splittedArgs.size()>0)
        {
            std::string comm = splittedArgs[0];
            std::string arg ="";
            if (splittedArgs.size()>1)
            {
                arg=splittedArgs[1];
            }
            if (comm=="exit")
            {
                bExit=true;
            } else if (comm=="help")
            {
                printHelper();
            } else {


                int rez = runAction(comm.c_str(),arg.c_str());
                if (rez!=0)
                {
                    m_pLog(ALL_WARNING,"IPLG","Failed running command %s",comm.c_str());
                }
            }
        }
        usleep(1000*150);
    }
}

void  InternalPlugin::startEventsThread()
{
   pthread_mutex_init(&m_evtQueueMutex,NULL);
   s_bMainLoopRunning=true;
   pthread_create(&m_eventsThread,NULL,InternalPlugin::runEventsNotifierThread,NULL);
}

void  InternalPlugin::createGrammar()
{
    auto_grammar gramm;

    gramm.grammar_name="Internal";
    gramm.module_name="Internal";
    gramm.commands.push_back("startTimer");
    gramm.commands.push_back("stopTimer");
    gramm.commands.push_back("flushTimer");
    gramm.commands.push_back("stopAllTimers");
    gramm.commands.push_back("waitMS");
    gramm.commands.push_back("message");

    gramm.events.push_back("timerStarted");
    gramm.events.push_back("timerFlushed");
    gramm.events.push_back("timerStopped");
    gramm.events.push_back("waitDone");
    gramm.events.push_back("msgReceived");

    m_gramms.push_back(gramm);
}

std::vector<auto_grammar>& InternalPlugin::getModuleGrammars()
{

    return m_gramms;
}

void    InternalPlugin::setEventsListener(IAutoEventsListener* wpListener)
{
    m_wpListener=wpListener;
}

IActionResolver*  InternalPlugin::getActionResolver()
{
    return this;
}

void  InternalPlugin::setLoggingCb(autoTestLoggingCb cb)
{
    m_pLog=cb;
}

const char* InternalPlugin::getPluginName()
{
    return "Internal";
}


int InternalPlugin::runAction(const char* cmdName, const char* arg)
{
    std::string arg_str(arg);

    if (strcmp(cmdName,"startTimer")==0)
    {
      // add a timer
      if (arg_str.length()>0)
      {
          arg_str = removeSequence(arg_str,"\"");
          bool bFound=false;
          for (unsigned int i=0;i<m_timers.size();i++)
          {
              std::pair<std::string, long>& cur(m_timers[i]);
              if (cur.first==arg_str)
              {
                  bFound=true;
                  break;
              }
          }
          if (bFound)
          {
              m_pLog(ALL_ERROR,"IPLG","startTimer: timer with name %s already started. Ignoring.",arg_str.c_str());
              return 1;
          } else {
              m_pLog(ALL_INFO,"IPLG","Starting timer named %s",arg_str.c_str());
              long curTime = getCurrentTimeMillis();
              std::pair<std::string, long> timerEntry;
              timerEntry.first = arg_str;
              timerEntry.second=curTime;
              m_timers.push_back(timerEntry);


              queueEvent("timerStarted",arg_str);

              return 0;
          }
      } else {
          m_pLog(ALL_ERROR,"IPLG","startTimer command provided with no arguments. Ignoring.");
          return 1;
      }      
    } else if (strcmp(cmdName,"stopTimer")==0)
    {
        // stop a timer
        if (arg_str.length()>0)
        {
            arg_str = removeSequence(arg_str,"\"");
            int foundIdx=-1;
            for (unsigned int i=0;i< m_timers.size();i++)
            {
                std::pair<std::string,long> &cur(m_timers[i]);
                if (cur.first==arg_str)
                {
                    foundIdx=i;
                    break;
                }
            }
            if (foundIdx>=0)
            {
                std::pair<std::string,long> &cur(m_timers[foundIdx]);
                long deltaT = getCurrentTimeMillis() - cur.second;

                std::string evtMsg("timer ");
                evtMsg= evtMsg.append(cur.first).append(" logged duration in ms: ").append(std::to_string(deltaT));
                queueEvent("timerStopped",evtMsg);

                // remove timer;
                m_timers[foundIdx]=m_timers[m_timers.size()-1];
                m_timers.pop_back();

            } else {
                m_pLog(ALL_ERROR,"IPLG","stopTimer: timer with name %s not found.",arg_str.c_str());
            }
        } else {
            m_pLog(ALL_ERROR,"IPLG","stopTimer command provided with no arguments. Ignoring.");
            return 1;
        }

    } else if (strcmp(cmdName,"flushTimer")==0)
    {

        // stop a timer
        if (arg_str.length()>0)
        {

            arg_str = removeSequence(arg_str,"\"");
            int foundIdx=-1;

            for (unsigned int i=0;i< m_timers.size();i++)
            {
                std::pair<std::string,long> &cur(m_timers[i]);
                if (cur.first==arg_str)
                {
                    foundIdx=i;
                    break;
                }
            }

            if (foundIdx>=0)
            {

                std::pair<std::string,long> &cur(m_timers[foundIdx]);
                long deltaT = getCurrentTimeMillis() - cur.second;

                std::string evtMsg("timer ");
                evtMsg= evtMsg.append(cur.first).append(" logged duration in ms: ").append(std::to_string(deltaT));

                queueEvent("timerFlushed",evtMsg);


            } else {
                m_pLog(ALL_ERROR,"IPLG","flushTimer: timer with name %s not found.",arg_str.c_str());
                return 1;
            }
        } else {
            m_pLog(ALL_ERROR,"IPLG","flushTimer command provided with no arguments. Ignoring.");
            return 1;
        }

    } else if (strcmp(cmdName,"stopAllTimers")==0) {

        for (unsigned int i=0;i<m_timers.size();i++)
        {
            std::pair<std::string,long> &cur(m_timers[i]);
            long deltaT = getCurrentTimeMillis()- cur.second;
            std::string evtMsg("timer ");
            evtMsg= evtMsg.append(cur.first).append(" logged duration in ms: ").append(std::to_string(deltaT));
            queueEvent("timerStopped",evtMsg);
        }
        m_timers.clear();

    } else if (strcmp(cmdName,"waitMS")==0) {

       if(arg_str.length()>0)
       {
            arg_str = removeSequence(arg_str,"\"");
            int read;
            int parsed=sscanf(arg_str.c_str(),"%d",&read);
            if (parsed==1)
            {
                usleep(1000* read);
                std::string evtMsg("waited ");
                evtMsg=evtMsg.append(std::to_string(read)).append(" ms");
                queueEvent("waitDone",evtMsg);
            } else {
                m_pLog(ALL_ERROR,"IPLG","waitMS: error parsing argument %s. Expecting int. Ignoring.",arg_str.c_str());
            }

       } else {
           m_pLog(ALL_ERROR,"IPLG","waitMS command provided with no arguments. Ignoring.");
           return 1;
       }
    }  else if (strcmp(cmdName,"message")==0) {

       if (arg_str.length()>0)
       {
            queueEvent("msgReceived",arg_str);
       } else {
            m_pLog(ALL_ERROR,"IPLG","message command provided with no arguments. Ignoring.");
            return 1;
       }

    }  else {
        m_pLog(ALL_ERROR,"IPLG","Untreated command for Internal plugin : %s",cmdName);
        return 1;
    }
    return 0;
}


void    InternalPlugin::queueEvent(std::string evt, std::string arg)
{
    pthread_mutex_lock(&m_evtQueueMutex);
    std::pair<std::string,std::string> cur;

    cur.first=evt;
    cur.second=arg;

    m_eventsQueue.push_back(cur);

    pthread_mutex_unlock(&m_evtQueueMutex);
}

void  InternalPlugin::fireEvents()
{
    pthread_mutex_lock(&m_evtQueueMutex);

    if (m_wpListener==NULL && m_eventsQueue.size()>0)
    {
        m_pLog(ALL_WARNING,"IPLG","No listener registered for internal plugin. Discarding %d events.",m_eventsQueue.size());
        m_eventsQueue.clear();
        pthread_mutex_unlock(&m_evtQueueMutex);
        return;
    }
    for (unsigned int i=0;i<m_eventsQueue.size();i++)
    {
        std::pair<std::string,std::string>& cur(m_eventsQueue[i]);
        // avoid exiting component with mutex locked
        pthread_mutex_unlock(&m_evtQueueMutex);
        m_wpListener->onEvtOccurred(cur.first.c_str(),cur.second.c_str());
        // reacquire mutex
        pthread_mutex_lock(&m_evtQueueMutex);
    }
    m_eventsQueue.clear();

    pthread_mutex_unlock(&m_evtQueueMutex);

}


void* InternalPlugin::runEventsNotifierThread(void*)
{
    if (s_pInstance!=NULL)
    {
        while (s_bMainLoopRunning)
        {
            s_pInstance->fireEvents();
            usleep(150*1000);
        }
    }
    return NULL;
}

