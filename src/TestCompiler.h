#ifndef __TEST_COMPILER_H__
#define __TEST_COMPILER_H__

#include "AutoTestLogging.h"
#include "AutoTestTypes.h"
#include "IAutoTestPlugin.h"
#include "TestLoader.h"

#include <map>


// translates all actions to the proper lamda calls
// stats from the main test and expands continously in the code
// end result will be a list of actions
// validates proper use of keywords
// validates packages / modules visibility


class TestCompiler
{
public:

                TestCompiler(std::vector<test_unit>& units, std::vector<IAutoTestPlugin*>& wpPlugins, test& main_test);
                ~TestCompiler();

                // to provide backtraces
        void    setTestLoader(TestLoader* pLoader);

        void    setLoggingCb(autoTestLoggingCb cb);
        int     compile();

        std::vector<test_action>& getActions();

private:

        // validates that the referenced packages / modules exist
        // validates that the keywords used inside the module are valid
        int     validateUnits();
        int     validateUnit(int unitIdx);
        int     parseTest(test& aTest,int unitIdx);

        // internals
        std::string     getKeyWordFromCommand(std::string& command);
        bool isKeyword(std::string& aWord);
        void createKeyWordsMap();
        int getPluginForGrammar(std::string& grammar);
        int getUnitForPackage(std::string& packName);

        int getTestWithName(test_unit& unit,std::string& testName );



        std::vector<test_unit>& m_unitsRef;
        std::vector<IAutoTestPlugin*> m_wpPlugins;
        test& m_startTest;
        autoTestLoggingCb m_pLog;

        TestLoader* m_wpTestLoader;


        std::vector<test_action> m_actions;

        std::map<std::string, int> m_keywords;


        int resolveRunTest(test& aTest, test_unit& refUnit, int unitIdx, int actIdx);
        int resolveCmd(test& aTest, test_unit& refUnit, int unitIdx, int actIdx);
        int resolveExpectEvt(test& aTest, test_unit& refUnit, int unitIdx, int actIdx);
        int resolveCmdEvt(test& aTest, test_unit& refUnit, int unitIdx, int actIdx);

        const int ACT_CMD=0;
        const int ACT_EVT=1;
        const int ACT_CMDEVT=2;

        int resolveSimpleEntry(test& aTest, test_unit& refUnit, int unitIdx, int actIdx, int actType);


};

#endif
