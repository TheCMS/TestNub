#ifndef __I_AUTO_GRAMMAR_H__
#define __I_AUTO_GRAMMAR_H__

#include <string>
#include <vector>


typedef const char* exposed_cmd;
typedef const char* exposed_evt;

struct auto_grammar
{
    std::string module_name;
    std::string grammar_name;
    std::vector<exposed_cmd> commands;
    std::vector<exposed_evt> events;
};

#endif
