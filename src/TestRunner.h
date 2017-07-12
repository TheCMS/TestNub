#ifndef __TEST_RUNNER_H__
#define __TEST_RUNNER_H__

#include "AutoTestLogging.h"
#include "AutoTestTypes.h"
#include "IAutoEventsListener.h"

#include "pthread.h"


class TestRunner : public IAutoEventsListener
{
public:
        TestRunner(std::vector<test_action>& actions);
        ~TestRunner();

        void    setLoggingCb(autoTestLoggingCb cb);
        int     runTests();
        // void    setTestActions(std::vector<test_action>& actions );

        virtual void onEvtOccurred(const char* evtName, const char* evtOut);


        int    handleEventsQueue(int deltaT);

        bool    testFinishing();
        bool    evtQueueEmpty();

private:

        int     handleRunCmd(test_action& cmd);
        int     handleRunExpectEvt(test_action& evt);
        int     handleRunCmdEvt(test_action& cmdEvt);
        // checks for events receival
        int     handleTestsFinish();


        void    removeEventAtIdx(int idx);

        bool    m_bFinishing;

        std::vector<test_action>& m_actions;
        autoTestLoggingCb m_pLog;

        std::vector<std::string> m_waitedEvents;
        std::vector<std::string> m_expectedSubstring;
        std::vector<int> m_eventsTimeouts;

        std::vector<std::string> m_evtsToProcess;
        std::vector<std::string> m_evtsArgs;

        pthread_mutex_t m_eventsLock;
        pthread_t m_evtsThread;



};



#endif
