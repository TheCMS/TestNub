#include "PluginsManager.h"
#include "TestManager.h"
#include "AutoTestLogging.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdarg.h>
#include <iostream>


#include "AutoTestHelperFuncs.h"


void printUsageMessage()
{
    printf("ModuleTester [run mode] [plugins file path] [options]\n");
    printf("\n\n");
    printf("run modes\n");
    printf("\t  interactive\t\tallows the user to use the cli version of the loaded plugins.\n"); // v
    printf("\t  automated\t\tallows the program to run automated tests without alowing user interaction.\n");
    printf("\n\noptions\n");
    printf("\t  runmodule:[module]\t\tstarts cli directly for the specified module\n"); // v
    printf("\t  testsOutfile:[filepath]\tredirects test results output to specified file\n");
    printf("\t  testsfile:[filepath]\t\tloads,validates and compiles and runs autotests from specified tests file from filepath\n");
    printf("\t  validate\t\t\tonly validates and compiles tests from specified test file\n");
    printf("\t  modluesLogFile:[fielpath]\tredirects logs from modules to the given filepath. Using \"NULL\" will silence the output.\n");
    printf("\t  quietmode\t\t\tafter testing starts ,only test results will be displayed.\n");
    printf("\t  loglevel:[loglvl]\t\tonly outputs log levels under or equal to the given value. 0 - Fatal 5 -verbose\n");

}

bool interractiveMode=false;
bool validateOnly=false;
bool quietMode=false;

std::string pluginsPath="";
std::string testFilePath="";
std::string testsOutfile="";
std::string runModule="";
std::string modulesLog="";


int g_logLevel=5;


void validateArguments()
{
    if (modulesLog.find("NULL")==std::string::npos && modulesLog.size()>1)
    {
        FILE* f = fopen(modulesLog.c_str(),"wb");
        fclose(f);
    }
    if (testsOutfile.size()>0)
    {
        FILE* f = fopen(testsOutfile.c_str(),"wb");
        fclose(f);
    }

    printf("Auto test options:\n");
    printf("Plugins file:%s\n",pluginsPath.c_str());
    if (interractiveMode)
    {
        printf("Interractive mode\n");
        if (runModule.size()>0)
        {
            printf("Running module %s\n",runModule.c_str());
        }
    } else {
        printf("Automated mode\n");

        printf("Tests file %s\n",testFilePath.c_str());
        if (modulesLog.size()>0)
        {
            printf("Modules Log File %s\n",modulesLog.c_str());
        }
        if (testsOutfile.size()>0)
        {
            printf("Test Results Output File %s\n",testsOutfile.c_str());
        }
        if (validateOnly)
        {
            printf("Validation only\n");
        }
    }

    if (quietMode)
    {
        printf("Quiet Mode is active!\n");
    }

    printf("********************************************\n");

}

void stdOutCb(eAutoLogLevels level, const char* ctx, const char* fmt, ...)
{
    if (quietMode) return;
    if ((int)level > g_logLevel)
        return;

    char buffer[2048];
    va_list lst;
    va_start (lst,fmt);

    vsnprintf(buffer,2048,fmt,lst);

    va_end(lst);


    std::cout<<LogLevelToStr(level)<<" ["<<ctx<<"] "<<buffer<<std::endl;

}
void modulesLogCb(eAutoLogLevels level, const char* ctx, const char* fmt, ...)
{
    if ((int)level > g_logLevel)
        return;

    if (quietMode) return;
    char buffer[2048];
    va_list lst;
    va_start (lst,fmt);

    vsnprintf(buffer,2048,fmt,lst);

    va_end(lst);
    if (modulesLog.size()>0)
    {
        FILE* f = fopen(modulesLog.c_str(),"a+");
        if (f)
        {
            if (level==ALL_SPECIAL)
            {
                fprintf(f,"%s\n",buffer);
            } else {
                fprintf(f,"%s [%s] %s\n",LogLevelToStr(level),ctx,buffer);
            }
            fclose(f);
        } else {
            // quiet
        }

    } else {

        if (level==ALL_SPECIAL)
        {
            std::cout<<buffer<<std::endl;
        } else {
            std::cout<<LogLevelToStr(level)<<" ["<<ctx<<"] "<<buffer<<std::endl;
        }
    }
}
// not affected by normal logs
void testResultLogCb(eAutoLogLevels level, const char* ctx, const char* fmt, ...)
{
    if (level > 0)
        return;

    char buffer[2048];
    va_list lst;
    va_start (lst,fmt);


    vsnprintf(buffer,2048,fmt,lst);

    va_end(lst);
    if (testsOutfile.size()>0)
    {
        FILE* f = fopen(modulesLog.c_str(),"a+");
        if (f)
        {
            if (level==ALL_SPECIAL)
            {
                fprintf(f,"%s\n",buffer);
            } else {
                fprintf(f,"%s [%s] %s\n",LogLevelToStr(level),ctx,buffer);
            }
            fclose(f);
        } else {
            std::cout<<LogLevelToStr(level)<<" ["<<ctx<<"] "<<buffer<<std::endl;
        }

    } else {

        if (level==ALL_SPECIAL)
        {
            std::cout<<buffer<<std::endl;
        } else {
            std::cout<<LogLevelToStr(level)<<" ["<<ctx<<"] "<<buffer<<std::endl;
        }
    }
}

void parseInputArguments(int startIdx, int argc, char** argv)
{
    for (int i=startIdx;i<argc;i++)
    {
        std::string str(argv[i]);
        if (str.find("runmodule")!=std::string::npos)
        {
            std::vector<std::string> strList;
            splitString(str,":",strList);
            if (strList.size()<2)
            {
                printf("no argument provided for runmodule option!\n");
            } else {
                runModule = strList[1];
                if (!interractiveMode)
                {
                    printf("Warning: RunModule only works in interractive mode !\n");
                }
            }
        } else if (str.find("testsOutfile")!=std::string::npos)
        {
            std::vector<std::string> strList;
            splitString(str,":",strList);
            if (strList.size()<2)
            {
                printf("no argument provided for outfile option!\n");
            } else {
                testsOutfile = std::string(strList[1]);
            }
            if (interractiveMode)
            {
                printf("Warning: testsOutfile option only works in automated mode !\n");
            }
        }  else if (str.find("testsfile")!=std::string::npos)
        {
            std::vector<std::string> strList;
            splitString(str,":",strList);
            if (strList.size()<2)
            {
                printf("no argument provided for testsfile option!\n");
            } else {
                testFilePath = strList[1];
            }
            if (interractiveMode)
            {
                printf("Warning: testsfile option only works in automated mode !\n");
            }
        }  else if (str.find("validate")!=std::string::npos)
        {
            validateOnly=true;
            if (interractiveMode)
            {
                printf("Warning: validate option only works in automated mode !\n");
            }
        } else if (str.find("modluesLogFile")!=std::string::npos)
        {
            std::vector<std::string> strList;
            splitString(str,":",strList);
            if (strList.size()<2)
            {
                printf("no argument provided for testsfile option!\n");
            } else {
                modulesLog = strList[1];
            }
            if (interractiveMode)
            {
                printf("Warning: modulesLogFile option only works in automated mode !\n");
            }
        } else if (str.find("quietmode")!=std::string::npos)
        {
            quietMode=true;
        } else if (str.find("loglevel")!=std::string::npos)
        {
            std::vector<std::string> strList;
            splitString(str,":",strList);
            if (strList.size()<2)
            {
                printf("no argument provided for log level option!\n");
            } else {
                g_logLevel = atoi(strList[1].c_str());
                if (g_logLevel<0)
                {
                    g_logLevel=0;
                    printf("minimum log level is 0(FATAL). If silence is required, use quite moed.\n");
                }
                if (g_logLevel>5)
                {
                    g_logLevel=5;
                    printf("maximum log level is 5(VERBOSE).Using maximum value.\n");
                }
            }
        }
        else {
            printf("ignoring untreated argument :%s\n",argv[i]);
        }
    }
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        printUsageMessage();
        return 1;
    }

    if (strcmp("interactive",argv[1])==0)
    {
        printf("Running in interractive mode\n");
        interractiveMode=true;
    } else if (strcmp("automated",argv[1])==0)
    {

        printf("Running in automated mode\n");
        interractiveMode=false;
    } else {
        printf("Unknown run mode : %s . Aborting\n",argv[1]);
        return 1;
    }

    // validate plugins
    pluginsPath = argv[2];

    FILE* f = fopen(pluginsPath.c_str(),"rb");
    if (!f)
    {
        printf("Warning: Failed to open provided plugins file %s.\nRunning program without any plugins.\n",pluginsPath.c_str());
    } else {
        fclose(f);
    }

    parseInputArguments(3,argc,argv);

    validateArguments();

    PluginsManager::GetInstance()->setStdCb(stdOutCb);
    PluginsManager::GetInstance()->setModulesLoggerCb(modulesLogCb);
    PluginsManager::GetInstance()->loadPluginsFromFile(pluginsPath.c_str());

    if (interractiveMode)
    {
        printf("Starting interractive mode.\n");
        if (runModule.size() > 1)
        {
            IAutoTestPlugin* pPlugin = PluginsManager::GetInstance()->getPlugin(runModule.c_str());
            if (pPlugin != NULL)
            {
                pPlugin->runCLI();
            } else {
                printf("Cannot find plugin with name %s. Aborting.\n",runModule.c_str());
                return 1;
            }

        } else {
            PluginsManager::GetInstance()->beginInterractiveMode();
        }
    } else {

        TestManager::GetInstance()->setStdCb(stdOutCb);
        TestManager::GetInstance()->setTestLoggerCb(testResultLogCb);

        TestManager::GetInstance()->setGrammarModules(PluginsManager::GetInstance()->getLoadedPlugins());
        int rez = TestManager::GetInstance()->loadTestsFromFile(testFilePath.c_str());
        if (rez!=0)
        {
            printf("ERROR loading tests. Aborting!\n");
            return 1;
        }
        rez = TestManager::GetInstance()->compileTests();

        if (rez!=0)
        {
            printf("ERROR compiling tests. Aborting!\n");
            return 1;
        }
        if (!validateOnly)
        {
            printf("Beggining Auto tests.\n");
            TestManager::GetInstance()->runTests();
        }
    }

    TestManager::Destroy();
    PluginsManager::Destroy();


    return 0;
   
}
