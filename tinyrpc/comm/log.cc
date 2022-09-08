#include <time.h>
#include <sys/time.h>
#include <sstream>
#include <sys/syscall.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string>
#include <vector>

#ifdef DECLARE_MYSQL_PLUGIN 
#include <mysql/mysql.h>
#endif


#include "tinyrpc/comm/log.h"
#include "tinyrpc/comm/config.h"
#include "tinyrpc/comm/run_time.h"
#include "tinyrpc/coroutine/coroutine.h"
#include "tinyrpc/net/reactor.h"
#include "tinyrpc/net/timer.h"


std::string get_time_string() {
  timeval cur_time;
  gettimeofday(&cur_time, nullptr);

  struct tm time; 
  localtime_r(&(cur_time.tv_sec), &time);

  const char* format = "%Y-%m-%d %H:%M:%S";
  char buf[128];
  strftime(buf, sizeof(buf), format, &time);

  std::string rst = std::string(buf) + "." + std::to_string(cur_time.tv_usec);

  return rst;
}


namespace tinyrpc {

extern tinyrpc::Logger::ptr gRpcLogger;
extern tinyrpc::Config::ptr gRpcConfig;

void CoredumpHandler(int signal_no) {
  ErrorLog << "progress received invalid signal, will exit";
  printf("progress received invalid signal, will exit\n");
  gRpcLogger->flush();
  pthread_join(gRpcLogger->getAsyncLogger()->m_thread, NULL);
  pthread_join(gRpcLogger->getAsyncAppLogger()->m_thread, NULL);

  signal(signal_no, SIG_DFL);
  raise(signal_no);
}

class Coroutine;

static thread_local pid_t t_thread_id = 0;
static pid_t g_pid = 0;

// LogLevel g_log_level = DEBUG;

pid_t gettid() {
  if (t_thread_id == 0) {
    t_thread_id = syscall(SYS_gettid);
  }
  return t_thread_id;
}

void setLogLevel(LogLevel level) {
  // g_log_level = level;
}

inline std::vector<std::string>
StringSplit(const std::string& src, char split, size_t split_count = 0) {
    std::vector<std::string> tar;
    auto idx_s = src.begin(), idx_e = src.begin(), str_end = src.end();
    for (; idx_e < str_end; ++idx_e) {
        if (*idx_e == split && idx_e != idx_s && split_count-- != 0) {
            tar.emplace_back(idx_s, idx_e);
            idx_s = idx_e + 1;
        }
    }
    if (idx_e != idx_s) {
        tar.emplace_back(idx_s, idx_e);
    }
    return tar;
}

std::string get_short_file_name(const char* ori_file_name, int count=3) {
  std::string s_file_name = ori_file_name;
  std::vector<std::string> vectors = StringSplit(s_file_name, '/');

  if (vectors.size() > 3) {
    return vectors[vectors.size()-3] + "/" + vectors[vectors.size()-2] + "/" + vectors[vectors.size()-1];
  }
  return s_file_name;
}



LogEvent::LogEvent(LogLevel level, const char* file_name, int line, const char* func_name, LogType type)
  : m_level(level),
    m_line(line),
    m_func_name(func_name),
    m_type(type) {

    m_file_name= get_short_file_name(file_name).c_str();
    // m_file_name = file_name;

    // printf("ori_file_name: %s, short_file_name: %s \n", file_name, m_file_name.c_str());
}

LogEvent::~LogEvent() {

}

std::string levelToString(LogLevel level) {
  std::string re = "DEBUG";
  switch(level) {
    case DEBUG:
      re = "DEBUG";
      return re;
    
    case INFO:
      re = "INFO";
      return re;

    case WARN:
      re = "WARN";
      return re;

    case ERROR:
      re = "ERROR";
      return re;

    default:
      return re;
  }
}


LogLevel stringToLevel(const std::string& str) {
    if (str == "DEBUG")
      return LogLevel::DEBUG;
    
    if (str == "INFO")
      return LogLevel::INFO;

    if (str == "WARN")
      return LogLevel::WARN;

    if (str == "ERROR")
      return LogLevel::ERROR;

    return LogLevel::DEBUG;
}

std::string LogTypeToString(LogType logtype) {
  switch (logtype) {
    case APP_LOG:
      return "app";
    case RPC_LOG:
      return "rpc";
    default:
      return "";
  }
}

std::stringstream& LogEvent::getStringStream() {

  // time_t now_time = m_timestamp;
  
  gettimeofday(&m_timeval, nullptr);

  struct tm time; 
  localtime_r(&(m_timeval.tv_sec), &time);

  const char* format = "%Y-%m-%d %H:%M:%S";
  char buf[128];
  strftime(buf, sizeof(buf), format, &time);

  m_ss << "[" << buf << "." << m_timeval.tv_usec << "] "; 

  std::string s_level = levelToString(m_level);
  m_ss << "[" << s_level << "] ";

  if (g_pid == 0) {
    g_pid = getpid();
  }
  m_pid = g_pid;  

  if (t_thread_id == 0) {
    t_thread_id = gettid();
  }
  m_tid = t_thread_id;

  m_cor_id = Coroutine::GetCurrentCoroutine()->getCorId();
  
  // m_ss << "[" << m_pid << "] " 
	// 	<< "[" << m_tid << "] "
	// 	<< "[" << m_cor_id << "] "
  //   << "[" << m_file_name << ":" << m_line << "] ";

  m_ss << "[" << m_tid << "] "
		<< "[" << m_cor_id << "] "
    << "[" << m_file_name << "." 
    << m_func_name << "." << m_line << "]:";

    // << "[" << m_func_name << "]\t";
  RunTime* runtime = getCurrentRunTime();
  if (runtime) {
    std::string msgno = runtime->m_msg_no;
    if (!msgno.empty()) {
      m_ss << "[" << msgno << "]\t";
    }

    std::string interface_name = runtime->m_interface_name;
    if (!interface_name.empty()) {
      m_ss << "[" << interface_name << "]\t";
    }

  }
  return m_ss;
}

void LogEvent::log() {
  if (m_level >= gRpcConfig->m_log_level) {
    m_ss << "\n";
    // printf("%s", m_ss.str().c_str());
    if (m_type == RPC_LOG) {
      gRpcLogger->pushRpcLog(m_ss.str());
    } else if (m_type == APP_LOG) {
      gRpcLogger->pushAppLog(m_ss.str());
    }
  }
}


LogTmp::LogTmp(LogEvent::ptr event) : m_event(event) {

}

std::stringstream& LogTmp::getStringStream() {
  return m_event->getStringStream();
}

LogTmp::~LogTmp() {
  m_event->log(); 
}

Logger::Logger() {
  // cannot do anything which will call LOG ,otherwise is will coredump

}

Logger::~Logger() {
  flush();
  pthread_join(m_async_rpc_logger->m_thread, NULL);
  pthread_join(m_async_app_logger->m_thread, NULL);
}

void Logger::init(const char* file_name, const char* file_path, int max_size, int sync_inteval) {
  if (!m_is_init) {
    printf("%s Logger::init \n", get_time_string().c_str());
    TimerEvent::ptr event = std::make_shared<TimerEvent>(sync_inteval, true, std::bind(&Logger::loopFunc, this));
    Reactor::GetReactor()->getTimer()->addTimerEvent(event);
    DebugLog << "Add Logger TimerEvent Over!\n";

    m_async_rpc_logger = std::make_shared<AsyncLogger>(file_name, file_path, max_size, RPC_LOG);
    m_async_app_logger = std::make_shared<AsyncLogger>(file_name, file_path, max_size, APP_LOG);

    signal(SIGSEGV, CoredumpHandler);
    signal(SIGABRT, CoredumpHandler);
    signal(SIGTERM, CoredumpHandler);
    signal(SIGKILL, CoredumpHandler);
    signal(SIGINT, CoredumpHandler);
    signal(SIGSTKFLT, CoredumpHandler);

    // ignore SIGPIPE 
    signal(SIGPIPE, SIG_IGN);
    m_is_init = true;
  }
}
	
void Logger::loopFunc() {
  std::vector<std::string> tmp;
  std::vector<std::string> app_tmp;
  Mutex::Lock lock(m_mutex);
  tmp.swap(m_buffer);
  app_tmp.swap(m_app_buffer);
  lock.unlock();

  m_async_rpc_logger->push(tmp);
  m_async_app_logger->push(app_tmp);
}

void Logger::pushRpcLog(const std::string& msg) {
  Mutex::Lock lock(m_mutex);
  m_buffer.push_back(msg);
  lock.unlock();
}

void Logger::pushAppLog(const std::string& msg) {
  Mutex::Lock lock(m_mutex);
  m_app_buffer.push_back(msg);
  lock.unlock();
}

void Logger::flush() {
  loopFunc();
  m_async_rpc_logger->stop();
  m_async_rpc_logger->flush();

  m_async_app_logger->stop();
  m_async_app_logger->flush();
}

AsyncLogger::AsyncLogger(const char* file_name, const char* file_path, int max_size, LogType logtype)
  : m_file_name(file_name), m_file_path(file_path), m_max_size(max_size), m_log_type(logtype) {

  pthread_create(&m_thread, nullptr, &AsyncLogger::excute, this);
}

AsyncLogger::~AsyncLogger() {

}

void* AsyncLogger::excute(void* arg) {
  AsyncLogger* ptr = reinterpret_cast<AsyncLogger*>(arg);
  pthread_cond_init(&ptr->m_condition, NULL);

  while (1) {
    Mutex::Lock lock(ptr->m_mutex);

    while (ptr->m_tasks.empty()) {
      pthread_cond_wait(&(ptr->m_condition), ptr->m_mutex.getMutex());
    }
    std::vector<std::string> tmp;
    tmp.swap(ptr->m_tasks.front());
    ptr->m_tasks.pop();
    bool is_stop = ptr->m_stop;
    lock.unlock();

    timeval now;
    gettimeofday(&now, nullptr);

    struct tm now_time;
    localtime_r(&(now.tv_sec), &now_time);

    const char *format = "%Y%m%d";
    char date[32];
    strftime(date, sizeof(date), format, &now_time);
    if (ptr->m_date != std::string(date)) {
      // cross day
      // reset m_no m_date
      ptr->m_no = 0;
      ptr->m_date = std::string(date);
      ptr->m_need_reopen = true;
    }

    if (!ptr->m_file_handle) {
      ptr->m_need_reopen = true;
    }    

    std::stringstream ss;
    ss << ptr->m_file_path << ptr->m_file_name << "_" << ptr->m_date << "_" << LogTypeToString(ptr->m_log_type) << "_" << ptr->m_no << ".log";
    std::string full_file_name = ss.str();

    if (ptr->m_need_reopen) {
      if (ptr->m_file_handle) {
        fclose(ptr->m_file_handle);
      }

      ptr->m_file_handle = fopen(full_file_name.c_str(), "a");
      ptr->m_need_reopen = false;
    }

    if (ftell(ptr->m_file_handle) > ptr->m_max_size) {
      fclose(ptr->m_file_handle);

      // single log file over max size
      ptr->m_no++;
      std::stringstream ss2;
      ss2 << ptr->m_file_path << ptr->m_file_name << "_" << ptr->m_date << "_" << LogTypeToString(ptr->m_log_type) << "_" << ptr->m_no << ".log";
      full_file_name = ss2.str();

      // printf("open file %s", full_file_name.c_str());
      ptr->m_file_handle = fopen(full_file_name.c_str(), "a");
      ptr->m_need_reopen = false;
    }

    if (!ptr->m_file_handle) {
      printf("open log file %s error!", full_file_name.c_str());
    }

    for(auto i : tmp) {
      fwrite(i.c_str(), 1, i.length(), ptr->m_file_handle);
      // printf("succ write rt %d bytes ,[%s] to file[%s]", rt, i.c_str(), full_file_name.c_str());
    }
    fflush(ptr->m_file_handle);
    if (is_stop) {
      break;
    }

  }
  if (!ptr->m_file_handle) {
    fclose(ptr->m_file_handle);
  }

  return nullptr;

}



void AsyncLogger::push(std::vector<std::string>& buffer) {
  if (!buffer.empty()) {
    Mutex::Lock lock(m_mutex);
    m_tasks.push(buffer);
    lock.unlock();
    pthread_cond_signal(&m_condition);
  }
}

void AsyncLogger::flush() {
  if (m_file_handle) {
    fflush(m_file_handle);
  }
}


void AsyncLogger::stop() {
  if (!m_stop) {
    m_stop = true;
  }
}

void Exit(int code) {
  #ifdef DECLARE_MYSQL_PLUGIN
  mysql_library_end();
  #endif

  printf("It's sorry to said we start TinyRPC server error, look up log file to get more deatils!\n");
  gRpcLogger->flush();
  pthread_join(gRpcLogger->getAsyncLogger()->m_thread, NULL);

  _exit(code);
}


}
