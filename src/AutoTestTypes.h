#ifndef __AUTO_TEST_TYPES_H__
#define __AUTO_TEST_TYPES_H__

#include <string>
#include <vector>


enum eActionTypes
{
    AT_CMD,
    AT_EVT,
    AT_CMD_AND_WAIT
};

struct test_command
{
    std::string cmd_name;
    std::string arg;
};

struct test_event
{
    std::string evt_name;
    std::string evt_out;
    int timeout;
};

struct test_action
{
    eActionTypes actionType;

    test_command command;
    test_event evt;

    // should be only of type IActionResolver
    void* interpretor;
    // meta info
    std::string fileName;
    std::string testName;
    std::string actionStr;
    int actionLine;

};

struct test
{
    std::string test_name;
    std::string package;
    std::vector<std::string> actions;
    int testResult;

    // meta inf
    std::string src_file;
    int testLine;
    std::vector<int> action_fileLine;

    void resetInfo()
    {
        src_file="";
        action_fileLine.clear();
        actions.clear();
        test_name="";
        package="";
        testLine=-1;
        testResult=-1;
    }

};

struct test_unit
{
    std::vector<test> tests;
    std::vector<std::string> used_grammars;
    std::vector<std::string> used_packages;
    std::string package_name;
    std::string fileName;

    int refIdx;

};

// tests are composed of actions + visibility
// tests are compiled according to test_unit
// compilation is done test by test with unit visibility

#endif
