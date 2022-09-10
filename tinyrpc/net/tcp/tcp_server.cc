#include <sys/socket.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include "tinyrpc/net/tcp/tcp_server.h"
#include "tinyrpc/net/tcp/tcp_connection.h"
#include "tinyrpc/net/tcp/io_thread.h"
#include "tinyrpc/net/tcp/tcp_connection_time_wheel.h"
#include "tinyrpc/coroutine/coroutine.h"
#include "tinyrpc/coroutine/coroutine_hook.h"
#include "tinyrpc/coroutine/coroutine_pool.h"
#include "tinyrpc/comm/config.h"
#include "tinyrpc/net/http/http_codec.h"
#include "tinyrpc/net/http/http_dispatcher.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_dispatcher.h"


namespace tinyrpc {

extern tinyrpc::Config::ptr gRpcConfig;

TcpAcceptor::TcpAcceptor(NetAddress::ptr net_addr) : m_local_addr(net_addr) {
	
	m_family = m_local_addr->getFamily();
}

void TcpAcceptor::init() {
	InfoLog << "-------- Listen Port Start";
	m_fd = socket(m_local_addr->getFamily(), SOCK_STREAM, 0);
	if (m_fd < 0) {
		ErrorLog << "start server error. socket error, sys error=" << strerror(errno);
		Exit(0);
	}
	// assert(m_fd != -1);
	DebugLog << "create listenfd succ, listenfd=" << m_fd;

	// int flag = fcntl(m_fd, F_GETFL, 0);
	// int rt = fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
	
	// if (rt != 0) {
		// ErrorLog << "fcntl set nonblock error, errno=" << errno << ", error=" << strerror(errno);
	// }

	int val = 1;
	if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
		ErrorLog << "set REUSEADDR error";
	}

	socklen_t len = m_local_addr->getSockLen();
	int rt = bind(m_fd, m_local_addr->getSockAddr(), len);
	if (rt != 0) {
		ErrorLog << "start server error. bind error, errno=" << errno << ", error=" << strerror(errno);
		Exit(0);
	}
  // assert(rt == 0);

	DebugLog << "set REUSEADDR succ";
	rt = listen(m_fd, 10);
	if (rt != 0) {
		ErrorLog << "start server error. listen error, fd= " << m_fd << ", errno=" << errno << ", error=" << strerror(errno);
		Exit(0);
	}
	InfoLog << "-------- Listen Port Start Over!";
  // assert(rt == 0);

}

TcpAcceptor::~TcpAcceptor() {
  FdEvent::ptr fd_event = FdEventContainer::GetFdContainer()->getFdEvent(m_fd);
  fd_event->unregisterFromReactor();
	if (m_fd != -1) {
		close(m_fd);
	}
}

int TcpAcceptor::toAccept() {

	DebugLog << "toAccept Start!";

	socklen_t len = 0;
	int rt = 0;

	if (m_family == AF_INET) {
		sockaddr_in cli_addr;
		memset(&cli_addr, 0, sizeof(cli_addr));
		len = sizeof(cli_addr);
		// call hook accept
		rt = accept_hook(m_fd, reinterpret_cast<sockaddr *>(&cli_addr), &len);
		if (rt == -1) {
			DebugLog << "error, no new client coming, errno=" << errno << "error=" << strerror(errno);
			return -1;
		}
		
		m_peer_addr = std::make_shared<IPAddress>(cli_addr);
	} else if (m_family == AF_UNIX) {
		sockaddr_un cli_addr;
		len = sizeof(cli_addr);
		memset(&cli_addr, 0, sizeof(cli_addr));
		// call hook accept
		rt = accept_hook(m_fd, reinterpret_cast<sockaddr *>(&cli_addr), &len);
		if (rt == -1) {
			DebugLog << "error, no new client coming, errno=" << errno << "error=" << strerror(errno);
			return -1;
		}
		m_peer_addr = std::make_shared<UnixDomainAddress>(cli_addr);

	} else {
		ErrorLog << "unknown type protocol!";
		close(rt);
		return -1;
	}

	InfoLog << "New client accepted succ! fd:[" << rt <<  ", addr:[" << m_peer_addr->toString() << "]";
	DebugLog << "toAccept Over!\n";
	return rt;	
}

std::string get_prototype(const tinyrpc::ProtocalType& type) {
	if (type == Http_Protocal) {
		return "Http";
	} else {
		return "TinyPb";
	}	
}


TcpServer::TcpServer(NetAddress::ptr addr, ProtocalType type /*= TinyPb_Protocal*/) : m_addr(addr) {
	DebugLog << "-------- Construct TcpServer ----------- ";
	DebugLog << "prototype: "<< get_prototype(type) << ", io_thread_num: " << gRpcConfig->m_iothread_num << ", address: " << m_addr->toString();

  	m_io_pool = std::make_shared<IOThreadPool>(gRpcConfig->m_iothread_num);
	if (type == Http_Protocal) {
		m_dispatcher = std::make_shared<HttpDispacther>();
		m_codec = std::make_shared<HttpCodeC>();
		m_protocal_type = Http_Protocal;
	} else {
		m_dispatcher = std::make_shared<TinyPbRpcDispacther>();
		m_codec = std::make_shared<TinyPbCodeC>();
		m_protocal_type = TinyPb_Protocal;
	}
	m_main_reactor = tinyrpc::Reactor::GetReactor();
	DebugLog << "-------- Construct TcpServer Over! -----------\n";
	// InfoLog << "TcpServer setup on [" << m_addr->toString() << "]";
}

void TcpServer::start() {

	DebugLog << "-------- TcpServer Start -----------";

	m_acceptor.reset(new TcpAcceptor(m_addr));
	// m_accept_cor = std::make_shared<tinyrpc::Coroutine>(128 * 1024, std::bind(&TcpServer::MainAcceptCorFunc, this)); 

	m_accept_cor = GetCoroutinePool()->getCoroutineInstanse();
	m_accept_cor->setCallBack(std::bind(&TcpServer::MainAcceptCorFunc, this));

	tinyrpc::Coroutine::Resume(m_accept_cor.get());

	DebugLog << "tinyrpc::Coroutine::Resume(m_accept_cor.get()) over!";

	m_main_reactor->loop();

	DebugLog << "-------- TcpServer Start Over! -----------";

}

TcpServer::~TcpServer() {
	GetCoroutinePool()->returnCoroutine(m_accept_cor);
  DebugLog << "~TcpServer";
}

NetAddress::ptr TcpServer::getPeerAddr() {
	return m_acceptor->getPeerAddr();
}

void TcpServer::MainAcceptCorFunc() {
  DebugLog << "MainAcceptCorFunc enable Hook here";

  m_acceptor->init();
  while (!m_is_stop_accept) {
	DebugLog << "----- accept loop start!";

    int fd = m_acceptor->toAccept();
    if (fd == -1) {
      ErrorLog << "accept ret -1 error, return, to yield";
      Coroutine::Yield();
      continue;
    }
    IOThread *io_thread = m_io_pool->getIOThread();
    auto cb = [this, io_thread, fd]() {
      io_thread->addClient(this, fd);
    };
    io_thread->getReactor()->addTask(cb);
    m_tcp_counts++;
    DebugLog << "----- accept loop end , current tcp connection count is [" << m_tcp_counts << "]\n";
  }
}

AbstractDispatcher::ptr TcpServer::getDispatcher() {	
	return m_dispatcher;	
}

AbstractCodeC::ptr TcpServer::getCodec() {
	return m_codec;
}

void TcpServer::addCoroutine(Coroutine::ptr cor) {
	m_main_reactor->addCoroutine(cor);
}

void TcpServer::registerService(std::shared_ptr<google::protobuf::Service> service) {
	if (m_protocal_type == TinyPb_Protocal) {
		if (service) {
			dynamic_cast<TinyPbRpcDispacther*>(m_dispatcher.get())->registerService(service);
		} else {
			ErrorLog << "service is nullptr";
		}
	} else {
		ErrorLog << "register service error. Just TinyPB protocal server need to resgister Service";
	} 
}

void TcpServer::registerHttpServlet(const std::string& url_path, HttpServlet::ptr servlet) {
	if (m_protocal_type == Http_Protocal) {
		if (servlet) {
			dynamic_cast<HttpDispacther*>(m_dispatcher.get())->registerServlet(url_path, servlet);
		} else {
			ErrorLog << "service is nullptr";
		}
	} else {
		ErrorLog << "register service error. Just Http protocal server need to resgister HttpServlet";
	} 
}

IOThreadPool::ptr TcpServer::getIOThreadPool() {
	return m_io_pool;
}

}
