//
// async_connection.hpp
// ~~~~~~~~~~
//
// �񓯊��̂��߂̃N���X�Q
//


#ifndef TWIT_LIB_PROTOCOL_CONNECTTYPE_ASYNC_CONNECTION
#define TWIT_LIB_PROTOCOL_CONNECTTYPE_ASYNC_CONNECTION

#include <memory>
#include <boost/asio.hpp>
#include "connection_base.hpp"
#include "../application_layer/layer_base.hpp"

namespace oauth{
namespace protocol{
namespace connection_type{

class async_connection : public connection_base{
public:
	async_connection(std::shared_ptr<application_layer::layer_base>& socket_layer)
	{
		socket_layer_ = socket_layer;
	}
	virtual ~async_connection(){}
	
	//�ǉ�	
	error_code& operator() (
		const std::string& host,
		boost::asio::streambuf& buf,
		error_code& ec
		)
	{
		buf_ = &buf;
		boost::asio::ip::tcp::resolver resolver(socket_layer_->get_io_service());
		boost::asio::ip::tcp::resolver::query query(host,socket_layer_->service_protocol());
		resolver.async_resolve(query,
			boost::bind(&async_connection::handle_resolve,this,
			boost::asio::placeholders::iterator,
			boost::asio::placeholders::error)); //handle_resolve��

		//�񓯊�����
		return ec;
	}

	//�ǉ�	
	error_code& operator() (
		boost::asio::ip::tcp::resolver::iterator ep_iterator,
		boost::asio::streambuf& buf,
		error_code& ec
		)
	{
		buf_ = &buf;
		socket_layer_->async_connect(ep_iterator,
			boost::bind(&async_connection::handle_connect,this,
				boost::asio::placeholders::error));
		
		//�񓯊�����
		return ec;
	}

private:
	boost::asio::streambuf *buf_;
	void handle_resolve(boost::asio::ip::tcp::resolver::iterator ep_iterator,const boost::system::error_code& ec)
	{
		if(!ec)
		{
			socket_layer_->async_connect(ep_iterator,
				boost::bind(&async_connection::handle_connect,this,
					boost::asio::placeholders::error));
		}
		else std::cout << "Error Resolve!?\n" << ec.message() << std::endl;
	}
	void handle_connect(const boost::system::error_code& ec)
	{
		if(!ec)
		{
			socket_layer_->async_write(*buf_,
				boost::bind(&async_connection::handle_write,this,
					boost::asio::placeholders::error));
		}
		else std::cout << "Error Connect or HandShake!?" << std::endl;
	}
	void handle_write(const boost::system::error_code& ec)
	{
		if(!ec)
		{
			socket_layer_->async_read();
		}
		else std::cout << "Error Write!?" << std::endl;
	}
};

} // namespace connection_type
} // namespace protocol
} // namespace oauth

#endif
