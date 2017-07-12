#include "TestRunner.h"
#include "IActionResolver.h"
#include "AutoTestHelperFuncs.h"
#include <unistd.h>

#include <iostream>


TestRunner::TestRunner(std::vector<test_action>& actions):
    m_bFinishing(false),
    m_actions(actions)
{

}
TestRunner::~TestRunner()
{

}

void TestRunner::setLoggingCb(autoTestLoggingCb cb)
{
    m_pLog=cb;
}


void* runEventsListeningLoop(void* arg)
{
    TestRunner* pRunner = (TestRunner*)arg;

    int* result = new int();

    long lastTime= getCurrentTimeMillis();
    while (!(pRunner->evtQueueEmpty() && pRunner->testFinishing())) // quit when all events were received and the queue is empty
    {
        long deltaT= getCurrentTimeMillis()- lastTime;
        *result= *result + pRunner->handleEventsQueue(deltaT);

        usleep(150*1000);

    }

    return result;
}


int TestRunner::runTests()
{

    int err =pthread_mutex_init(&m_eventsLock,NULL);

    if (err!=0)
    {
        m_pLog(ALL_FATAL,"TSTR","Failed to init events mutex with err %d. Aborting!",err);
        return -1;
    }

    err = pthread_create(&m_evtsThread,NULL,runEventsListeningLoop,this);

    if (err!=0)
    {
        m_pLog(ALL_FATAL,"TSTR","Failed to init events loop with err %d. Aborting!",err);
        return -1;
    }

    int rez=0;

    m_pLog(ALL_INFO,"TSTR","Start performing %d actions.",m_actions.size());

    std::string curTest="";

    for (unsigned int i=0;i<m_actions.size();i++)
    {
        test_action& cur = m_actions[i];

        if (curTest != cur.testName)
        {
            if (curTest.length()>0)
            {
                m_pLog(ALL_SPECIAL,"TSTR","Test %s DONE.",curTest.c_str());
                m_pLog(ALL_SPECIAL,"TSTR","************END**************");
            }
            m_pLog(ALL_SPECIAL,"TSTR","*************BEGIN*************");
            m_pLog(ALL_SPECIAL,"TSTR","Starting executing test %s from %s.\n",cur.testName.c_str(),cur.fileName.c_str());
            curTest=cur.testName;
        }
        if (cur.actionType==AT_CMD)
        {


            int r = handleRunCmd(cur);
            if (r==0)
            {
                m_pLog(ALL_SPECIAL,"TSTR","[%d] [%s]:[%d] cmd %s ... SUCCESS ",i,curTest.c_str(),cur.actionLine,cur.actionStr.c_str());
            } else {
                m_pLog(ALL_SPECIAL,"TSTR","[%d] [%s]:[%d] cmd %s ... FAILED ",i,curTest.c_str(),cur.actionLine,cur.actionStr.c_str());
            }
            rez += r;
        } else if (cur.actionType==AT_EVT)
        {
            m_pLog(ALL_INFO,"TSTR","[%d] Test %s, Line %d, Adding event to be expected %s. ",i,cur.testName.c_str(),cur.actionLine,cur.actionStr.c_str());
            int r = handleRunExpectEvt(cur);
            if (r==0)
            {
                m_pLog(ALL_SPECIAL,"TSTR","[%d] [%s]:[%d] add evt %s ... SUCCESS ",i,curTest.c_str(),cur.actionLine,cur.actionStr.c_str());
            } else {
                m_pLog(ALL_SPECIAL,"TSTR","[%d] [%s]:[%d] add evt %s ... FAILED ",i,curTest.c_str(),cur.actionLine,cur.actionStr.c_str());
            }
            rez += r;

        } else if (cur.actionType==AT_CMD_AND_WAIT)
        {
            m_pLog(ALL_INFO,"TSTR","[%d] Test %s, Line %d, Adding event response for command %s. ",i,cur.testName.c_str(),cur.actionLine,cur.actionStr.c_str());
            rez+=handleRunExpectEvt(cur);
        }
    }

    rez += handleTestsFinish();

    m_pLog(ALL_SPECIAL,"TSTR","Final actions results: %d Passed %d Failed.",m_actions.size()-rez,rez);
    return rez;
}


int TestRunner::handleRunCmd(test_action& cmd)
{

    IActionResolver* pResolver = (IActionResolver*)cmd.interpretor;

    int rez=pResolver->runAction(cmd.command.cmd_name.c_str(),cmd.command.arg.c_str());

    return rez;
}

int TestRunner::handleRunExpectEvt(test_action& evt)
{
    pthread_mutex_lock(&m_eventsLock);
        m_pLog(ALL_VERBOSE,"TSTR","Adding event named %s, expected out %s , timeout %d\n",evt.evt.evt_name.c_str(),evt.evt.evt_out.c_str(),evt.evt.timeout);
        m_waitedEvents.push_back(evt.evt.evt_name);
        m_expectedSubstring.push_back(evt.evt.evt_out);
        m_eventsTimeouts.push_back(evt.evt.timeout);
    pthread_mutex_unlock(&m_eventsLock);
    return 0;
}

int TestRunner::handleRunCmdEvt(test_action& cmdEvt)
{
    (void) cmdEvt;
    return 0;
}

// checks for events receival
int TestRunner::handleTestsFinish()
{
    int ret=0;

    m_bFinishing=true;
    void* thrdResult;
    int rez= pthread_join(m_evtsThread,&thrdResult);

    if (rez!=0)
    {
        m_pLog(ALL_FATAL,"TSTR","Failed finishing events thread with error %d!",rez);
        ret=1;
    } else {
        if (thrdResult!=NULL)
        {
            ret =  *((int*)thrdResult);
            m_pLog(ALL_INFO,"TSTR","End of test cycle result: Total failed events %d",ret);

        } else {
            m_pLog(ALL_FATAL,"TSTR","No return from finished event thread!");
            ret = 1;
        }

    }
    return ret;
}

bool TestRunner::testFinishing()
{
    return m_bFinishing;
}

bool TestRunner::evtQueueEmpty()
{
    return m_waitedEvents.size()<=0;
}

void TestRunner::onEvtOccurred(const char* evtName, const char* evtOut)
{
    // push solving to event loop
    if (!(testFinishing() && evtQueueEmpty()))
    {
        pthread_mutex_lock(&m_eventsLock);
            m_evtsToProcess.push_back(evtName);
            m_evtsArgs.push_back(evtOut);
        pthread_mutex_unlock(&m_eventsLock);
    }
}

int TestRunner::handleEventsQueue(int deltaT)
{
    int rez=0;

    pthread_mutex_lock(&m_eventsLock);

    // match received with waited events

    for (unsigned int i=0;i<m_evtsToProcess.size();i++)
    {
        bool bEvtMatched=false;
        bool bEvtFound=false;
        std::string& cur(m_evtsToProcess[i]);
        std::string& arg(m_evtsArgs[i]);
        int evtIdx=-1;
        for (unsigned int j=0;j<m_waitedEvents.size();j++)
        {
            if (cur==m_waitedEvents[j])
            {
                evtIdx=j;
                bEvtFound=true;

                std::string& substr(m_expectedSubstring[j]);
                if (substr.size()==0 || arg.find(substr)!=std::string::npos)
                {
                    bEvtMatched=true;
                    break;
                }
            }
        }

        if (bEvtMatched)
        {
            std::string& substr(m_expectedSubstring[evtIdx]);
            // m_pLog(ALL_INFO,"TSTR","EVT %s with %s SUCCESS. Payload [%s].",cur.c_str(),substr.c_str(),arg.c_str());
            m_pLog(ALL_SPECIAL,"TSTR","EVT [%s][%s] SUCCESS. [%s]",cur.c_str(),substr.c_str(),arg.c_str());
           // remove event from queue
           removeEventAtIdx(evtIdx);
           // make sure to reprocess the new event
           i--;

        } else if (bEvtFound) {
            std::string& substr(m_expectedSubstring[evtIdx]);
            m_pLog(ALL_DEBUG,"TSTR","EVT %s received but pattern not found %s. Received payload [%s].",cur.c_str(),substr.c_str(),arg.c_str());
        } else {
            m_pLog(ALL_DEBUG,"TSTR","Received unexpected event %s. Ignoring.",cur.c_str());
        }
    }
    m_evtsToProcess.clear();
    m_evtsArgs.clear();

    // apply timeouts
    for (unsigned int i=0;i<m_eventsTimeouts.size();i++)
    {
        if (m_eventsTimeouts[i]<0)
        {
            // infite waiting will be ignored
            continue;
        }

        m_eventsTimeouts[i]-= deltaT;
        if (m_eventsTimeouts[i]<0)
        {
            // m_pLog(ALL_INFO,"TSTR","EVT %s for substring %s FAILED. Timeout expired.",m_waitedEvents[i].c_str(),m_expectedSubstring[i].c_str());
            m_pLog(ALL_SPECIAL,"TSTR","EVT [%s][%s] FAILED. Timeout expired.",m_waitedEvents[i].c_str(),m_expectedSubstring[i].c_str());
            removeEventAtIdx(i);
            // make sure to reprocess event
            i--;
            rez++;

        }

    }
    pthread_mutex_unlock(&m_eventsLock);


    return rez;
}
void TestRunner::removeEventAtIdx(int idx)
{
    // mutex should be done in upper method
    m_waitedEvents[idx]= m_waitedEvents[m_waitedEvents.size()-1];
    m_expectedSubstring[idx] =m_expectedSubstring[m_expectedSubstring.size()-1];
    m_eventsTimeouts[idx]= m_eventsTimeouts[m_eventsTimeouts.size()-1];


    m_waitedEvents.pop_back();
    m_expectedSubstring.pop_back();
    m_eventsTimeouts.pop_back();

}
