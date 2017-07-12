#ifndef __I_AUTO_EVENTS_LISTENER_H__
#define __I_AUTO_EVENTS_LISTENER_H__

class IAutoEventsListener
{
public:


        virtual ~IAutoEventsListener(){}

        virtual void  onEvtOccurred(const char* evtName, const char* evtOut)=0;
};

#endif
