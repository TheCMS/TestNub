#ifndef __TEST_MANAGER_H__
#define __TEST_MANAGER_H__


#include "TestLoader.h"
#include "TestCompiler.h"
#include "AutoTestLogging.h"
#include "TestRunner.h"
#include "IAutoTestPlugin.h"
#include <vector>

class TestManager
{

private:
        TestManager();
        ~TestManager();
public:

static TestManager* GetInstance();
static void Destroy();
       void setTestLoggerCb(autoTestLoggingCb cb);
       void setStdCb(autoTestLoggingCb cb);

       int loadTestsFromFile(const char* filepath);
       void setGrammarModules(std::vector<IAutoTestPlugin*>& plugins);
       int compileTests();
       int runTests();

private:

       TestLoader*   m_pLoader;
       TestCompiler*    m_pCompiler;
       TestRunner*   m_pRunner;

       autoTestLoggingCb    m_pTestLoggerCb;
       autoTestLoggingCb    m_pStdCb;

       std::vector<IAutoTestPlugin*> m_wpPlugins;

       bool m_bLoaded;
       bool m_bCompiled;

static TestManager* s_pInstance;

};


#endif
