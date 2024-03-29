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

  int shm_key;
  if(std::getenv("CLOCK_SHM_KEY")){
    shm_key=atoi(std::getenv("CLOCK_SHM_KEY"));
  }else{
    shm_key=969;
  }
  std::cerr << "[ShmTimePeriodicExecutionContext] shmget " << shm_key << std::endl;

  int shm_id = shmget(shm_key, sizeof(struct timeval), 0666|IPC_CREAT);
  if(shm_id == -1) {
    std::cerr << "\e[0;31m" << "[ClockShmItem] shmget failed"  << "\e[0m" << std::endl;
    exit(1);
  }
  c_shm = (struct timeval *)shmat(shm_id, (void *)0, 0);
  if(c_shm == (void*)-1) {
    std::cerr << "\e[0;31m" << "[ClockShmItem] shmat failed"  << "\e[0m" << std::endl;
    exit(1);
  }

  coil::TimeValue prev_t(c_shm->tv_sec,c_shm->tv_usec);
  std::vector<std::string> rtc_names;
  do
    {
      // PeriodicExecutionContextではm_worker.running_のチェックを行い、trueになるまでwaitする機能が入っていた. hrpECでは、入っていなかった. この機能があると、複数RTCを一つのECで回している場合に、ctrl-Cで終了するときになぜかwaitから戻ってこないことがあったため、入れないことにした. m_worker.running_と、m_runningは、stop()のときに同時にfalseになるので、ほとんど影響はない?

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

      coil::TimeValue cur_t(c_shm->tv_sec,c_shm->tv_usec);
      if((double)(cur_t - prev_t) < 0) prev_t = cur_t; //巻き戻り

      if ((double)(cur_t - prev_t) > m_period){
        if( m_comps.size() != rtc_names.size() ) {
          rtc_names.resize(m_comps.size());
          for(int i=0;i<m_comps.size();i++){
            RTC::ComponentProfile_var profile = RTC::RTObject::_narrow(m_comps[i]._ref)->get_component_profile(); // get_component_profile() はポインタを返すので、呼び出し側で release するか、_var型変数で受ける必要がある. get_component_profile() は時間がかかるので、毎周期呼んではいけない
            rtc_names[i] = profile->instance_name;
          }
        }

        std::cerr<<"[ShmTimeEC] Timeover: processing time = "<<(double)(cur_t - prev_t)<<"[s]"<<std::endl;
        for(int i=0;i<m_comps.size();i++){
          std::cerr << rtc_names[i] <<"("<<processTimes[i]<<"), ";
        }
        std::cerr << std::endl;
      }

      // ROS::Timeは/clockが届いたときにしか変化しない. /clockの周期と制御周期が近い場合、単純なcoil::sleep(m_period - (t1 - t0))では誤差が大きい
      while (!m_nowait && m_period > (cur_t - prev_t))
        {
          coil::sleep((coil::TimeValue)(double(m_period) / 100));
          cur_t = coil::TimeValue(c_shm->tv_sec,c_shm->tv_usec);
          if((double)(cur_t - prev_t) < 0) prev_t = cur_t; //巻き戻り
        }

      while (prev_t < cur_t) prev_t = prev_t + m_period;
      ++count;
    } while (m_running); // m_svcはデストラクタでfalseになる. m_runningはstop()でfalseになる. PeriodicExecutionContextではm_svcを見て、hrpECはm_runningを見ている. hrpECに合わせてm_runningにした

  return 0;
}

/*
  https://github.com/choreonoid/choreonoid/blob/release-1.7/src/OpenRTMPlugin/SimulationPeriodicExecutionContext.cpp を参考にした
  PeriodicExecutionContextは、Ctrl-Cしてから終了するまでに時間がかかる. これは、全てのRTCがINACTIVEになるまで待ってから終了する処理が走っているからで、Ctrl-Cしてゾンビ状態になっているRTCからのレスポンスを待ち続けるような状態になっているらしい. 待つ処理を削除した.
 */
RTC::ReturnCode_t ShmTimePeriodicExecutionContext::deactivate_component(RTC::LightweightRTObject_ptr comp)
throw (CORBA::SystemException)
{
    RTC_TRACE(("deactivate_component()"));
    
    CompItr it = std::find_if(m_comps.begin(), m_comps.end(), find_comp(comp));
    if(it == m_comps.end()) { return RTC::BAD_PARAMETER; }
    if(!(it->_sm.m_sm.isIn(RTC::ACTIVE_STATE))){
        return RTC::PRECONDITION_NOT_MET;
    }
    it->_sm.m_sm.goTo(RTC::INACTIVE_STATE);
    
    return RTC::RTC_OK;
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
 
