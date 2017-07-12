#include "TestCompiler.h"
#include "AutoTestHelperFuncs.h"

TestCompiler::TestCompiler(std::vector<test_unit>& units, std::vector<IAutoTestPlugin*>& wpPlugins, test& main_test):
  m_unitsRef(units),
  m_wpPlugins(wpPlugins),
  m_startTest(main_test),
  m_pLog(NULL)
{
    createKeyWordsMap();
}

TestCompiler::~TestCompiler()
{
    m_actions.clear();
}

// to provide backtraces
void TestCompiler::setTestLoader(TestLoader* pLoader)
{
    m_wpTestLoader=pLoader;
}

void TestCompiler::setLoggingCb(autoTestLoggingCb cb)
{
    m_pLog= cb;
}

int TestCompiler::compile()
{
    m_pLog(ALL_INFO,"TCMP","Validating test units");
    int rez = validateUnits();
    if (rez!=0)
    {
        m_pLog(ALL_ERROR,"TCMP","Validating test units failed ! Compilation stopped");
        return -1;
    }
    m_pLog(ALL_INFO,"TCMP","Parsing tests ");
    rez = parseTest(m_startTest,0);
    if (rez!=0)
    {
        m_pLog(ALL_ERROR,"TCMP","Test composing failed with %d errors! Compilation stopped",rez);
    } else {
    m_pLog(ALL_INFO,"TCMP","Tests compilation succeded ");
    }
    return rez;
}

int TestCompiler::validateUnits()
{
    int rez=0;
    for (unsigned  int i=0;i<m_unitsRef.size();i++)
    {
        int tmp= validateUnit(i);
        if (tmp!=0)
        {
            rez+=tmp;
            m_pLog(ALL_ERROR,"TCMP","Failed validating unit %s.",m_unitsRef[i].fileName.c_str());

            if (m_wpTestLoader!=NULL)
            {
                m_wpTestLoader->printFileReferenceToOutput(i);
            }
        } else {
            m_pLog(ALL_INFO,"TCMP","Unit %s validated succesfully.",m_unitsRef[i].fileName.c_str());
        }
    }
    if (rez>0)
    {
        m_pLog(ALL_ERROR,"TCMP","%d units failed validation.",rez);
        return -1;
    }
    return 0;
}

int TestCompiler::validateUnit(int unitIdx)
{
    // validates all tests inside the unit
    int rez=0;
    test_unit& cur(m_unitsRef[unitIdx]);

    for (unsigned int i=0;i<cur.used_packages.size();i++)
    {
        std::string& curStr(cur.used_packages[i]);
        bool bFound= getUnitForPackage(curStr) >=0;

        if (!bFound)
        {
            rez++;
            m_pLog(ALL_ERROR,"TCMP","Package %s referenced by unit %s not loaded.",curStr.c_str(),cur.fileName.c_str());
        }
    }
    for (unsigned int i=0;i<cur.used_grammars.size();i++)
    {
        std::string& curStr(cur.used_grammars[i]);

        bool bFound = getPluginForGrammar(curStr)>=0;

        if (!bFound)
        {
            rez++;
            m_pLog(ALL_ERROR,"TCMP","Module grammar %s referenced by unit %s not available.",curStr.c_str(),cur.fileName.c_str());
        }
    }

    for (unsigned int i=0;i<cur.tests.size();i++)
    {
        test& tst(cur.tests[i]);

        for (unsigned int j=0;j<tst.actions.size();j++)
        {
            std::string keyWord = getKeyWordFromCommand(tst.actions[j]);

            if (keyWord.length()>0)
            {
                if (!isKeyword(keyWord))
                {
                    m_pLog(ALL_ERROR,"TCMP","File %s, test %s,line %d:\nKeyword [%s] not recognized.",
                           cur.fileName.c_str(),tst.test_name.c_str(),tst.action_fileLine[j],keyWord.c_str());
                    rez++;
                }
            } else {
                m_pLog(ALL_ERROR,"TCMP","File %s, test %s,line %d:\nError extracting keyword from line %s.\n",cur.fileName.c_str(),
                       tst.test_name.c_str(),tst.action_fileLine[j],tst.actions[j].c_str());
                rez++;
            }
        }
    }

    if (rez>0)
    {
        m_pLog(ALL_ERROR,"TCMP","Unit %s has %d errors! ",cur.fileName.c_str(),rez);
        return 1;
    } else {
        return 0;
    }
}

int TestCompiler::parseTest(test& aTest, int unitIdx)
{
    int rez=0;
    test_unit& unit= m_unitsRef[unitIdx];
    for (unsigned int i=0;i<aTest.actions.size();i++)
    {
        std::string keyWord = getKeyWordFromCommand(aTest.actions[i]);
        if (keyWord=="run_test")
        {
            rez += resolveRunTest(aTest,unit,unitIdx,i);
        } else if (keyWord == "cmd")
        {
            rez += resolveSimpleEntry(aTest,unit, unitIdx,i,ACT_CMD);
        } else if (keyWord == "expect_event")
        {
            rez += resolveSimpleEntry(aTest,unit, unitIdx,i,ACT_EVT);
        } else if (keyWord == "run_cmd_and_wait_event")
        {
            rez += resolveSimpleEntry(aTest,unit, unitIdx,i,ACT_CMDEVT);
        } else {
            rez++;
            m_pLog(ALL_ERROR,"File %s, test %s, line %d.\nUnexpected keyword %s validation error.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[i],
                   keyWord.c_str());
        }
    }
    return rez;
}

std::string TestCompiler::getKeyWordFromCommand(std::string& command)
{
    std::vector<std::string> lst;
    splitString(command,"(",lst);

    if (lst.size()>0)
    {
        return lst[0];
    }
    return "";
}

bool TestCompiler::isKeyword(std::string& aWord)
{
    if (m_keywords.count(aWord)==0) return false;
    return true;
}
void TestCompiler::createKeyWordsMap()
{
    m_keywords["run_test"]=0;
    m_keywords["cmd"]=0;
    m_keywords["expect_event"]=0;
    m_keywords["run_cmd_and_wait_event"]=0;
}

int TestCompiler::getPluginForGrammar(std::string& grammar)
{

    for (unsigned int j=0;j<m_wpPlugins.size();j++)
    {
        std::vector<auto_grammar>& gramms(m_wpPlugins[j]->getModuleGrammars());

        for (unsigned int k=0;k<gramms.size();k++)
        {
            if (grammar==gramms[k].grammar_name)
            {
                return j;
            }
        }
    }
    return -1;
}

int TestCompiler::getUnitForPackage(std::string& packName)
{
    for (unsigned int j=0;j<m_unitsRef.size();j++)
    {
        if (packName == m_unitsRef[j].package_name)
        {
            return j;
        }
    }
    return -1;
}
int TestCompiler::getTestWithName(test_unit& unit,std::string& testName )
{
   for (unsigned int i=0;i<unit.tests.size();i++)
   {
        if (unit.tests[i].test_name == testName) return i;
   }
   return -1;
}

int TestCompiler::resolveRunTest(test& aTest, test_unit& refUnit, int unitIdx, int actIdx)
{
    std::vector<std::string> args;
    std::string unpacked = aTest.actions[actIdx];
    removeSequence(unpacked,"run_test(");
    removeSequence(unpacked,")");
    splitString(unpacked,",",args);
    // validate called command

    std::string& cmd(args[0]);

    int rez=0;

    if (cmd.find("::")!=std::string::npos)
    {
        // references plugin

        m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nTests cannot be invoked from plugins.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx]);
        rez++;


    } else if (cmd.find(".")!=std::string::npos)
    {
        // references package
        std::vector<std::string> splitted;
        splitString(cmd,".",splitted);

        if (splitted.size()==2)
        {

            bool bFound=false;
            for (unsigned int j=0;j<refUnit.used_packages.size();j++)
            {
                if (refUnit.used_packages[j]==splitted[0])
                {
                    bFound=true;
                    break;
                }
            }
            if (bFound)
            {
                int uIdx = getUnitForPackage(splitted[0]);
                if(uIdx <0)
                {
                    m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nPackage %s referenced but not loaded.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
                           splitted[0].c_str());

                } else {
                    test_unit& u(m_unitsRef[uIdx]);
                    int testIdx = getTestWithName(u,splitted[1]);

                    if (testIdx<0)
                    {
                        m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nPackage %s doesn't contain test %s.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
                               splitted[0].c_str(),splitted[1].c_str());
                        rez++;
                    } else {

                        // actuall action is here
                        rez+= parseTest(u.tests[testIdx],uIdx);
                    }

                }
            } else {
                 m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nPackage %s not declared for usage.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx]
                        ,splitted[0].c_str());
                 rez++;
            }
        } else {
            m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nUnable to parse test reference %s.Expected 2, got %d.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx]
                   ,cmd.c_str(),splitted.size());
            rez++;
        }
    } else {
        // direct access
        int testIdx = getTestWithName(refUnit,cmd);
        if (testIdx>=0)
        {
            // actual action is here
            rez+= parseTest(refUnit.tests[testIdx],unitIdx);
        } else {
            m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nTest named %s not found declared inside pacakge %s.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
                   cmd.c_str(),refUnit.package_name.c_str());
        }
    }
    return rez;
}

int TestCompiler::resolveSimpleEntry(test& aTest, test_unit& refUnit, int unitIdx, int actIdx,int actType)
{

    (void)unitIdx;
    int rez=0;
    std::string actionStr= aTest.actions[actIdx];

    std::string &unpacked(actionStr);

    if (actType==ACT_CMD)
    {
        removeSequence(unpacked,"cmd(");
        removeSequence(unpacked,")");
    } else if (actType==ACT_EVT)
    {
        removeSequence(unpacked,"expect_event(");
        removeSequence(unpacked,")");
    }


    std::vector<std::string> splitted;


    splitOnce(unpacked,",",splitted);

    if (splitted.size()==2)
    {
        std::string& comm(splitted[0]);
        std::string& arg(splitted[1]);

        std::vector<std::string> commSplit;

        splitString(comm,"::",commSplit);

        if (commSplit.size()==2)
        {
            std::string& cmd(commSplit[1]);
            std::string& gramm(commSplit[0]);

            // validate plugin
            int plugIdx = getPluginForGrammar(gramm);
            if (plugIdx>=0)
            {
                // validated cmd
                IAutoTestPlugin* pPlugin = m_wpPlugins[plugIdx];
                std::vector<auto_grammar>& v(pPlugin->getModuleGrammars());
                int grammIdx=-1;
                for (unsigned int i=0;i<v.size();i++)
                {
                    if (v[i].grammar_name == gramm)
                    {
                        grammIdx=i;
                        break;
                    }
                }

                if (grammIdx>=0)
                {
                    auto_grammar& g(v[grammIdx]);

                    // validate cmd
                    int idx=-1;
                    if (actType==ACT_CMD)
                    {
                        for (unsigned int i=0;i<g.commands.size();i++)
                        {
                            if (g.commands[i]==cmd)
                            {
                                idx=i;
                                break;
                            }
                        }
                    } else if (actType==ACT_EVT)
                    {
                        for (unsigned int i=0;i<g.events.size();i++)
                        {
                            if (g.events[i]==cmd)
                            {
                                idx=i;
                                break;
                            }
                        }
                    }

                    int timeout;
                    std::string expectOut;

                    bool bValid=true;

                    if (actType==ACT_EVT)
                    {
                        std::vector<std::string> splittedArg;
                        splitOnce(arg,",",splittedArg);
                        if (splittedArg.size()==2)
                        {

                            sscanf(splittedArg[0].c_str(),"%d",&timeout);
                            expectOut= splittedArg[1];
                        } else {
                            m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nInvalid number of arguments for event expect %s. Expected format GRAMMAR::EVENT,timeout,argument.",
                                   aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],arg.c_str());
                            rez++;
                            bValid=false;
                        }
                    }

                    if (idx>=0)
                    {
                        if (bValid)
                        {
                            test_action act;

                            // functional
                            act.interpretor=pPlugin->getActionResolver();

                            if (actType==ACT_CMD)
                            {
                                act.actionType=AT_CMD;
                                act.command.arg=arg;
                                act.command.cmd_name=cmd;
                            } else if (actType==ACT_EVT)
                            {
                                act.actionType=AT_EVT;
                                act.evt.evt_name=cmd;
                                act.evt.timeout=timeout;
                                act.evt.evt_out=expectOut;

                            }

                            // meta info
                            act.actionLine=aTest.action_fileLine[actIdx];
                            act.actionStr=aTest.actions[actIdx];
                            act.fileName=refUnit.fileName;
                            act.testName=aTest.test_name;

                            m_actions.push_back(act);;

                            m_pLog(ALL_INFO,"TCMP","Added command to exectution queue %s.",cmd.c_str());
                        }

                    } else {

                        if (actType==ACT_CMD)
                        {
                            m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nCommand %s not exposed by grammar %s.",
                                   aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx], cmd.c_str(),gramm.c_str());

                        } else if (actType==ACT_EVT){

                            m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nEvent %s not exposed by grammar %s."
                                   ,aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],cmd.c_str(),gramm.c_str());
                        }
                        rez++;
                    }

                } else {
                    m_pLog(ALL_FATAL,"TCMP","File %s, test %s, line %d.\nUnexpected unmatch when treating grammar %s",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
                           gramm.c_str());
                    rez++;
                }


            } else {
                m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nGrammar %s not found!.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
                       gramm.c_str());
                rez++;
            }

        } else {
            if (actType==ACT_CMD)
            {
                m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nNo grammar referenced for command %s. Expected format is cmd(GRAMMAR::COMMAND,ARGUMENT).",
                       aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
                   actionStr.c_str());
            } else if (actType==ACT_EVT){
                m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nNo grammar referenced for event %s. Expected format is evt(GRAMMAR::EVT,TIMEOUT,ARGUMENT).",
                       aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
                   actionStr.c_str());
            }
            rez++;
        }


    } else {
        m_pLog(ALL_ERROR,"TCMP","File %s, test %s, line %d.\nError parsing command %s: command malformed.",aTest.src_file.c_str(),aTest.test_name.c_str(),aTest.action_fileLine[actIdx],
               actionStr.c_str());
        rez++;
    }

    return rez;
}


int TestCompiler::resolveCmd(test& aTest, test_unit& refUnit, int unitIdx, int actIdx)
{

    return resolveSimpleEntry(aTest,refUnit,unitIdx,actIdx,ACT_CMD);
}

int TestCompiler::resolveExpectEvt(test& aTest, test_unit& refUnit, int unitIdx, int actIdx)
{
    return resolveSimpleEntry(aTest,refUnit,unitIdx,actIdx,ACT_EVT);
}

int TestCompiler::resolveCmdEvt(test& aTest, test_unit& refUnit, int unitIdx, int actIdx)
{
    (void) aTest;
    (void)refUnit;
    (void)unitIdx;
    (void)actIdx;
    m_pLog(ALL_ERROR,"TCMP","Cmd and wait evet isn't implemented yet");
    return 1;
}

std::vector<test_action>& TestCompiler::getActions()
{
    return m_actions;
}
