#ifndef TINYRPC_NET_EVENT_LOOP_H
#define TINYRPC_NET_EVENT_LOOP_H

#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <atomic>
#include <map>
#include <functional>
#include <string>
#include "../coroutine/coroutine.h"
#include "fd_event.h"
#include "mutex.h"


using  std::string;

namespace tinyrpc {

class FdEvent;
class Timer;

// typedef std::shared_ptr<Timer> TimerPtr;

class Reactor {

 public:

  typedef std::shared_ptr<Reactor> ptr;

  explicit Reactor(const string name="");

  ~Reactor();

  void addEvent(int fd, epoll_event event, bool is_wakeup = true);

  void delEvent(int fd, bool is_wakeup = true);

  void addTask(std::function<void()> task, bool is_wakeup = true);

  void addTask(std::vector<std::function<void()>> task, bool is_wakeup = true);
  
  void addCoroutine(tinyrpc::Coroutine::ptr cor, bool is_wakeup = true);

  void wakeup();
  
  void loop();

  void process_pending_task();

  bool process_wakeup_event();

  bool process_outer_event(epoll_event& one_event);

  bool is_unknown_event(const epoll_event& one_event);

  void process_pending_fds();



  void stop();

  Timer* getTimer();

  pid_t getTid();
 
 public:
  static Reactor* GetReactor();
  

 private:

  void addWakeupFd();

  bool isLoopThread() const;

  void addEventInLoopThread(int fd, epoll_event event);

  void delEventInLoopThread(int fd);
  
 private:
  std::string m_name;
  int m_epfd {-1};
  int m_wake_fd {-1};         // wakeup fd
  int m_timer_fd {-1};        // timer fd
  bool m_stop_flag {false};
  bool m_is_looping {false};
  bool m_is_init_timer {false};
  pid_t m_tid {0};        // thread id

  Mutex m_mutex;                    // mutex
  
  std::vector<int> m_fds;              // alrady care events
  std::atomic<int> m_fd_size; 

  // fds that wait for operate
  // 1 -- to add to loop
  // 2 -- to del from loop
  std::map<int, epoll_event> m_pending_add_fds;
  std::vector<int> m_pending_del_fds;
  std::vector<std::function<void()>> m_pending_tasks;

  Timer* m_timer {nullptr};

};


}


#endif
