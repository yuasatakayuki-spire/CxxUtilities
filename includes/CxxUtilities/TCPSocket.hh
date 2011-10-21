/*
 * TCPSocket.hh
 *
 *  Created on: Aug 5, 2011
 *      Author: yuasa
 */

#ifndef TCPSOCKET_HH_
#define TCPSOCKET_HH_

#include "CxxUtilities/CommonHeader.hh"
#include "CxxUtilities/Exception.hh"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace CxxUtilities {

class TCPSocketException: public CxxUtilities::Exception {
public:
	enum {
		Disconnected,
		Timeout,
		TCPSocketError,
		PortNumberError,
		BindError,
		ListenError,
		AcceptException,
		OpenException,
		CreateException,
		HostEntryError,
		ConnectException,
		ConnectExceptionWhenChangingSocketModeToNonBlocking,
		ConnectExceptionNonBlockingConnectionImmediateluSucceeded,
		ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult,
		ConnectExceptionWhenChangingSocketModeToBlocking,
		Undefied
	};

public:
	TCPSocketException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}
};

class TCPSocket {
public:
	enum {
		TCPSocketInitialized, TCPSocketCreated, TCPSocketBound, TCPSocketListening, TCPSocketConnected
	};

protected:
	int status;

private:
	std::string name;
	double timeoutDurationInMilliSec;

protected:
	int socketdescriptor;
	int port;

public:
	TCPSocket() {
		status = TCPSocketInitialized;
		timeoutDurationInMilliSec = 0;
		setPort(-1);
	}

	~TCPSocket() {
	}

	int getStatus() {
		return status;
	}

	bool isConnected() {
		if (status == TCPSocketConnected) {
			return true;
		} else {
			return false;
		}
	}

public:
	int send(void* data, unsigned int length) throw (TCPSocketException) {
		int result = ::send(socketdescriptor, data, length, 0);
		if (result < 0) {
			throw TCPSocketException(TCPSocketException::TCPSocketError);
		}
		return result;
	}

	int receive(void* data, unsigned int length) throw (TCPSocketException) {
		ssize_t result = ::recv(socketdescriptor, data, length, 0);
		if (result <= 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				throw TCPSocketException(TCPSocketException::Timeout);
			} else {
				std::string err;
				switch (errno) {
				case EBADF:
					err = "EBADF";
					break;
				case ECONNREFUSED:
					err = "ECONNREFUSED";
					break;
				case EFAULT:
					err = "EFAULT";
					break;
				case EINTR:
					err = "EINTR";
					break;
				case EINVAL:
					err = "EINVAL";
					break;
				case ENOMEM:
					err = "ENOMEM";
					break;
				case ENOTCONN:
					err = "ENOTCONN";
					break;
				case ENOTSOCK:
					err = "ENOTSOCK";
					break;
				}
				usleep(1000000);
				throw TCPSocketException(TCPSocketException::Disconnected);
			}
		}
		return result;
	}

public:
	void setSocketDescriptor(int socketdescriptor) {
		this->socketdescriptor = socketdescriptor;
	}

	void setPort(unsigned short port) {
		this->port = port;
	}

	int getPort() {
		return port;
	}

	void setTimeout(double durationInMilliSec) {
		if (socketdescriptor != NULL) {
			timeoutDurationInMilliSec = durationInMilliSec;
			struct timeval tv;
			tv.tv_sec = (unsigned int) (floor(durationInMilliSec / 1000.));
			tv.tv_usec = (int) ((durationInMilliSec - floor(durationInMilliSec)) * 1000);
			setsockopt(socketdescriptor, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof tv);
		}
	}

	double getTimeoutDuration() {
		return timeoutDurationInMilliSec;
	}

	void setName(std::string name) {
		this->name = name;
	}

	std::string getName() {
		return name;
	}
};

class TCPServerAcceptedSocket: public TCPSocket {
private:
	struct ::sockaddr_in address;

public:
	TCPServerAcceptedSocket() :
		TCPSocket() {
	}

	~TCPServerAcceptedSocket() {
	}

	void open() throw (TCPSocketException) {
		throw TCPSocketException(TCPSocketException::OpenException);
	}

	void close() {
		switch (status) {
		case TCPSocketCreated:
		case TCPSocketBound:
		case TCPSocketListening:
		case TCPSocketConnected:
			::close(socketdescriptor);
			break;
		default:
			break;
		}
		status = TCPSocketInitialized;
	}

	bool isServerSocket() {
		return false;
	}

	void setAddress(struct ::sockaddr_in* address) {
		memcpy(&address, address, sizeof(struct ::sockaddr_in));
	}
};

class TCPServerSocket: public TCPSocket {
private:
	static const int maxofconnections = 5;

public:
	TCPServerSocket(int portNumber) :
		TCPSocket() {
		setPort(portNumber);
	}

	~TCPServerSocket() {
	}

	void open() throw (TCPSocketException) {
		if (getPort() < 0) {
			throw TCPSocketException(TCPSocketException::PortNumberError);
		}
		create();
		bind();
		listen();
	}

	void close() {
		switch (status) {
		case TCPSocketCreated:
		case TCPSocketBound:
		case TCPSocketListening:
		case TCPSocketConnected:
			::close(socketdescriptor);
			break;
		default:
			break;
		}
		status = TCPSocketInitialized;
	}

	bool isServerSocket() {
		return true;
	}

	void create() throw (TCPSocketException) {
		int result = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (result < 0) {
			throw TCPSocketException(TCPSocketException::TCPSocketError);
		} else {
			status = TCPSocketCreated;
			socketdescriptor = result;
		}
	}

	void bind() throw (TCPSocketException) {
		using namespace std;
		struct ::sockaddr_in serveraddress;
		memset(&serveraddress, 0, sizeof(struct ::sockaddr_in));
		serveraddress.sin_family = AF_INET;
		serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddress.sin_port = htons(getPort());
		int yes = 1;
		setsockopt(socketdescriptor, SOL_SOCKET, SO_REUSEADDR, (const char *) &yes, sizeof(yes));
		int result = ::bind(socketdescriptor, (struct ::sockaddr*) &serveraddress, sizeof(struct ::sockaddr_in));
		if (result < 0) {
			throw TCPSocketException(TCPSocketException::BindError);
		} else {
			status = TCPSocketBound;
		}
	}

	void listen() throw (TCPSocketException) {
		int result = ::listen(socketdescriptor, maxofconnections);
		if (result < 0) {
			throw TCPSocketException(TCPSocketException::ListenError);
		} else {
			status = TCPSocketListening;
		}
	}

	TCPServerAcceptedSocket* accept() throw (TCPSocketException) {
		using namespace std;
		struct ::sockaddr_in clientaddress;
		int length = sizeof(clientaddress);
		int result = ::accept(socketdescriptor, (struct ::sockaddr*) &clientaddress, (::socklen_t*) &length);
		if (result < 0) {
			throw TCPSocketException(TCPSocketException::AcceptException);
		} else {
			TCPServerAcceptedSocket* acceptedsocket = new TCPServerAcceptedSocket();
			acceptedsocket->setAddress(&clientaddress);
			acceptedsocket->setPort(getPort());
			acceptedsocket->setSocketDescriptor(result);
			return acceptedsocket;
		}
	}
};

class TCPClientSocket: public TCPSocket {
private:
	std::string url;
	struct ::sockaddr_in serveraddress;

public:
	TCPClientSocket() :
		TCPSocket() {
		setURL("");
		socketdescriptor = NULL;
	}

	TCPClientSocket(std::string url, int port) :
		TCPSocket() {
		setURL(url);
		setPort(port);
		socketdescriptor = NULL;
	}

	~TCPClientSocket() {
	}

	void open(double timeoutDurationInMilliSec = 1000) throw (TCPSocketException) {
		if (url.size() == 0) {
			throw TCPSocketException(TCPSocketException::OpenException);
		}
		create();
		connect(timeoutDurationInMilliSec);
	}

	void open(std::string url, int port, double timeoutDurationInMilliSec = 1000) throw (TCPSocketException) {
		setURL(url);
		setPort(port);
		open(timeoutDurationInMilliSec);
	}

	void close() {
		switch (status) {
		case TCPSocketCreated:
		case TCPSocketBound:
		case TCPSocketListening:
		case TCPSocketConnected:
			::close(socketdescriptor);
			break;
		default:
			break;
		}
		status = TCPSocketInitialized;
	}

	bool isServerSocket() {
		return false;
	}

	void create() throw (TCPSocketException) {
		int result = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (result < 0) {
			throw TCPSocketException(TCPSocketException::CreateException);
		} else {
			status = TCPSocketCreated;
			socketdescriptor = result;
		}
	}

	void connect(double timeoutDurationInMilliSec) throw (TCPSocketException) {
		int result;
		if (status == TCPSocketConnected) {
			return;
		}
		struct ::sockaddr_in serveraddress;
		memset(&serveraddress, 0, sizeof(struct ::sockaddr_in));
		serveraddress.sin_family = AF_INET;
		serveraddress.sin_port = htons(getPort());
		//set url or ip address
		struct ::hostent* hostentry;
		hostentry = ::gethostbyname(url.c_str());
		if (hostentry == NULL) {
			throw TCPSocketException(TCPSocketException::HostEntryError);
		} else {
			serveraddress.sin_addr.s_addr = *((unsigned long*) hostentry->h_addr_list[0]);
		}

		result=0;

		int flag=fcntl(socketdescriptor,F_GETFL,0);
		if(flag<0){
			throw TCPSocketException(TCPSocketException::ConnectExceptionWhenChangingSocketModeToNonBlocking);
		}
		if(fcntl(socketdescriptor, F_SETFL, flag|O_NONBLOCK)<0){
			throw TCPSocketException(TCPSocketException::ConnectExceptionWhenChangingSocketModeToNonBlocking);
		}


		result = ::connect(socketdescriptor, (struct ::sockaddr*) &serveraddress, sizeof(struct ::sockaddr_in));
		if (result < 0) {
			//if(result==EINPROGRESS){
				struct timeval tv;
				tv.tv_sec = (unsigned int) (floor(timeoutDurationInMilliSec / 1000.));
				tv.tv_usec = (int) ((timeoutDurationInMilliSec - floor(timeoutDurationInMilliSec)) * 1000);
				fd_set rmask,wmask;FD_ZERO(&rmask);FD_SET(socketdescriptor,&rmask);wmask=rmask;
				result=select(socketdescriptor+1,&rmask, &wmask, NULL, &tv);
				if(result==0){
					//timeout happened
					throw TCPSocketException(TCPSocketException::Timeout);
				}else{
					//connected
					status = TCPSocketConnected;
					//reset flag
					if(fcntl(socketdescriptor, F_SETFL, flag)<0){
						throw TCPSocketException(TCPSocketException::ConnectExceptionWhenChangingSocketModeToBlocking);
					}
					return;
				}
//			}else{
//				std::cout << result << std::endl;
//				throw TCPSocketException(TCPSocketException::ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult);
//			}
		} else {
			throw TCPSocketException(TCPSocketException::ConnectExceptionNonBlockingConnectionImmediateluSucceeded);
		}
	}

	void setURL(std::string url) {
		this->url = url;
	}
};

}
#endif /* TCPSOCKET_HH_ */