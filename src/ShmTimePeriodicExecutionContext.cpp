#include "ShmTimePeriodicExecutionContext.h"
#include <rtm/ECFactory.h>
#include <sys/shm.h>

ShmTimePeriodicExecutionContext::ShmTimePeriodicExecutionContext()
    : PeriodicExecutionContext()
{
}

ShmTimePeriodicExecutionContext::~ShmTimePeriodicExecutionContext()
{
}

int ShmTimePeriodicExecutionContext::svc(void)
{
  RTC_TRACE(("svc()"));
  int count(0);

  struct timeval * c_shm;

  int shm_id = shmget(969, sizeof(struct timeval), 0666|IPC_CREAT);
  if(shm_id == -1) {
    std::cerr << "\e[0;31m" << "[ClockShmItem] shmget failed"  << "\e[0m" << std::endl;
    exit(1);
  }
  c_shm = (struct timeval *)shmat(shm_id, (void *)0, 0);
  if(c_shm == (void*)-1) {
    std::cerr << "\e[0;31m" << "[ClockShmItem] shmat failed"  << "\e[0m" << std::endl;
    exit(1);
  }


  do
    {
      m_worker.mutex_.lock();
      while (!m_worker.running_)
        {
          m_worker.cond_.wait();
        }
      coil::TimeValue t0(c_shm->tv_sec,c_shm->tv_usec);

      std::vector<double> processTimes(m_comps.size());
      if (m_worker.running_)
        {
          for(int i=0;i<m_comps.size();i++){
            coil::TimeValue before(c_shm->tv_sec,c_shm->tv_usec);
            invoke_worker()(m_comps[i]);
            coil::TimeValue after(c_shm->tv_sec,c_shm->tv_usec);
            processTimes[i] = double(after - before);
          }
        }
      m_worker.mutex_.unlock();
      coil::TimeValue t1(c_shm->tv_sec,c_shm->tv_usec);

      if ((double)(t1 - t0) > m_period){
        std::cerr<<"[ShmTimeEC] Timeover: processing time = "<<(double)(t1 - t0)<<"[s]"<<std::endl;
        for(int i=0;i<m_comps.size();i++){
          RTC::ComponentProfile_var profile = RTC::RTObject::_narrow(m_comps[i]._ref)->get_component_profile(); // get_component_profile() はポインタを返すので、呼び出し側で release するか、_var型変数で受ける必要がある
          std::cerr << profile->instance_name<<"("<<processTimes[i]<<"), ";
        }
        std::cerr << std::endl;
      }

      // ROS::Timeは/clockが届いたときにしか変化しない. /clockの周期と制御周期が近い場合、単純なcoil::sleep(m_period - (t1 - t0))では誤差が大きい
      // ros::Rate::sleepは内部で1ms間隔でsleepするため、粗すぎる
      while (!m_nowait && m_period > (t1 - t0))
        {
          coil::sleep((coil::TimeValue)(double(m_period) / 100));
          t1 = coil::TimeValue(c_shm->tv_sec,c_shm->tv_usec);
        }

      ++count;
    } while (m_svc);

  return 0;
}

extern "C"
{
    void ShmTimePeriodicExecutionContextInit(RTC::Manager* manager)
    {
        manager->registerECFactory("ShmTimePeriodicExecutionContext",
                                   RTC::ECCreate<ShmTimePeriodicExecutionContext>,
                                   RTC::ECDelete<ShmTimePeriodicExecutionContext>);
        std::cerr << "ShmTimePeriodicExecutionContext is registered" << std::endl;
    }
};
 