#ifndef SHMTIMEPERIODICEXECUTIONCONTEXT_H
#define SHMTIMEPERIODICEXECUTIONCONTEXT_H

#include <rtm/RTC.h>
#include <rtm/Manager.h>

#include <rtm/PeriodicExecutionContext.h>


class ShmTimePeriodicExecutionContext : public virtual RTC::PeriodicExecutionContext
{
public:
    ShmTimePeriodicExecutionContext();
    virtual ~ShmTimePeriodicExecutionContext(void);
    virtual int svc(void);
    virtual RTC::ReturnCode_t deactivate_component(RTC::LightweightRTObject_ptr comp) throw (CORBA::SystemException);
};

extern "C"
{
  void ShmTimePeriodicExecutionContextInit(RTC::Manager* manager);
};

#endif
