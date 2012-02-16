//
// protocol.hpp
// ~~~~~~~~~~
//
// TCP��SSL���f�̂��߂́ABoost.Asio���g�p�����N���X�Q
//

#ifndef TWIT_LIB_PROTOCOL_APPLAYER_LAYER_BASE
#define TWIT_LIB_PROTOCOL_APPLAYER_LAYER_BASE

#include <map>
#include <memory>
#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>

#ifndef NO_SSL
#include <boost/asio/ssl.hpp>
#pragma comment(lib, "libeay32MDd.lib")
#pragma comment(lib, "ssleay32MDd.lib")
#endif

#include "../response_reader/response_container.hpp"

namespace oauth{
namespace protocol{
namespace application_layer{

//socket���ݍ��ށAapplicationlayer����Ɍ������邽�߂�
class layer_base : boost::noncopyable{
public:
	layer_base()
	{
		reset_response();
	}
	virtual ~layer_base(){}
	
	typedef boost::asio::io_service io_service;
	typedef boost::asio::ip::tcp::socket tcp_socket;
#ifndef NO_SSL
	typedef boost::asio::ssl::stream<tcp_socket> ssl_socket;
	typedef boost::asio::ssl::context context;
#endif
	typedef boost::system::error_code error_code;
	typedef boost::asio::ip::tcp::resolver resolver;
	typedef boost::asio::ip::tcp::endpoint endpoint;
	
	typedef boost::function<void (const error_code&)> ConnectHandler;
	typedef boost::function<void (const error_code&)> WriteHandler;
	//typedef boost::function<void (const error_code&)> ReadHandler;
	
	//�ǉ�
	virtual io_service& get_io_service() = 0;
	virtual const std::string service_protocol() const = 0;

	virtual error_code& connect(boost::asio::ip::tcp::resolver::iterator&,error_code&) = 0;
	virtual error_code& write(boost::asio::streambuf&,error_code&) = 0;
	virtual error_code& read(error_code&) = 0;
	
	virtual void async_connect(boost::asio::ip::tcp::resolver::iterator&,ConnectHandler) = 0;
	virtual void async_write(boost::asio::streambuf&,WriteHandler) = 0;
	virtual void async_read() = 0;
/*	virtual void async_read_header(boost::asio::streambuf&,ReadHandler) = 0;
	virtual void async_read_body(boost::asio::streambuf&,ReadHandler) = 0;*/
	
	std::shared_ptr<oauth::protocol::response_reader::response_container> get_response(){return response_;}
	void reset_response(){response_.reset(new oauth::protocol::response_reader::response_container());}

protected:
	std::shared_ptr<oauth::protocol::response_reader::response_container> response_;
};

/*class layer_base : boost::noncopyable{ //�Ŋ��N���X�H
public:
	layer_base(){} //�ꉞ�ق���ǉ����ď���
	virtual ~layer_base(){}
	
	typedef boost::asio::io_service io_service;
	typedef boost::asio::ip::tcp::socket tcp_socket;
#ifndef NO_SSL
	typedef boost::asio::ssl::stream<tcp_socket> ssl_socket;
	typedef boost::asio::ssl::context context;
#endif
	typedef boost::system::error_code error_code;
	typedef boost::asio::ip::tcp::endpoint endpoint;

	typedef boost::function<void (error_code)> ConnectHandler;
	//Handler�͒ǉ����Ă���

	//virtual error_code& async_connect() = 0;
	//virtual void async_write() = 0;
	//virtual void async_write_some() = 0;
	//virtual void async_read() = 0;
	//virtual void async_read_some() = 0;

	virtual error_code& sync_resolver(error_code&,endpoint&) = 0;
	virtual error_code& sync_connect(error_code&,endpoint&) = 0;
	virtual error_code& sync_write(boost::asio::streambuf&,error_code&) = 0;
	virtual error_code& sync_read(boost::asio::streambuf&,error_code&) = 0;

	//���܂������ł��Ȃ���ł��D
	//template<class Delim>
	//virtual error_code& sync_read_some(boost::asio::streambuf&,const Delim&,error_code&) = 0;

	virtual error_code& close(error_code& ec) = 0;

private:


//protected:
//	boost::scoped_ptr<work> worker;
//	void worker_set(io_service& io_service){worker.reset(new work(io_service));}
//	void worker_end(){worker.reset();}

};*/

} // namespace application_layer
} // namespace protocol
} // namespace oauth

#endif
