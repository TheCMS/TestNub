
#include "TestManager.h"

TestManager* TestManager::s_pInstance=NULL;

TestManager::TestManager():
    m_pLoader(NULL),
    m_pCompiler(NULL),
    m_bLoaded(false),
    m_bCompiled(false)
{

    m_pLoader = new TestLoader();
}

TestManager::~TestManager()
{
    delete m_pLoader;
    delete m_pCompiler;
    delete m_pRunner;
    m_pLoader=NULL;
    m_pCompiler=NULL;
    m_pRunner=NULL;
}

TestManager* TestManager::GetInstance()
{
    if (s_pInstance==NULL)
    {
        s_pInstance= new TestManager();
    }
    return s_pInstance;
}

void TestManager::Destroy()
{
    delete s_pInstance;
    s_pInstance=NULL;
}

void TestManager::setTestLoggerCb(autoTestLoggingCb cb)
{
    m_pTestLoggerCb=cb;

}

void TestManager::setStdCb(autoTestLoggingCb cb)
{
    m_pStdCb=cb;
    m_pLoader->setLoggingCb(cb);
}

int TestManager::loadTestsFromFile(const char* filepath)
{
    int rez = m_pLoader->loadTestsFromFile(filepath);
    if (rez!=0)
    {
        m_pStdCb(ALL_ERROR,"TMGR","Error loading test file [%s]!",filepath);
        m_bLoaded=false;
    } else {
        m_pStdCb(ALL_INFO,"TMGR","Success loading test file [%s]!",filepath);
        m_bLoaded=true;
    }
    return rez;
}

void TestManager::setGrammarModules(std::vector<IAutoTestPlugin*>& plugins)
{
    m_wpPlugins=plugins;
}

int TestManager::compileTests()
{
    if (!m_bLoaded)
    {
        m_pStdCb(ALL_ERROR,"TMGR","Cannot compile tests: tests failed to load!");
        return -1;
    }
    if (!m_pLoader->loadedTestsAreValid())
    {
        m_pStdCb(ALL_ERROR,"TMGR","Loaded tests are invalid !Aborting compile!");
        return -1;
    }

    m_pCompiler=new TestCompiler(m_pLoader->getLoadedUnits(),m_wpPlugins,m_pLoader->getStartingTest());

    m_pCompiler->setLoggingCb(m_pStdCb);
    m_pCompiler->setTestLoader(m_pLoader);

    int rez = m_pCompiler->compile();
    if (rez==0)
    {
        m_bCompiled=true;
    }
    return rez;
}

int TestManager::runTests()
{
    if (!m_bCompiled)
    {
        m_pStdCb(ALL_ERROR,"TMGR","Cannot run tests. Compilation failed!");
        return -1;
    }
    // init runner
    m_pRunner= new TestRunner(m_pCompiler->getActions());
    for (unsigned int i=0;i<m_wpPlugins.size();i++)
    {
        m_wpPlugins[i]->setEventsListener(m_pRunner);
    }
    m_pRunner->setLoggingCb(m_pTestLoggerCb);
    return m_pRunner->runTests();

}



