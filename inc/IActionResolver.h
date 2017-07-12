#ifndef __I_ACTION_RESOLVER_H__
#define __I_ACTION_RESOLVER_H__


// to be implemneted by each plugin
class IActionResolver
{
public:

        virtual ~IActionResolver(){}

        virtual int runAction(const char* cmdName, const char* arg)=0;
};


#endif
