//
// async_connection.hpp
// ~~~~~~~~~~
//
// �񓯊��̂��߂̃N���X�Q
//

#ifndef TWIT_LIB_PROTOCOL_CONNECTTYPE_SYNC_CONNECTION
#define TWIT_LIB_PROTOCOL_CONNECTTYPE_SYNC_CONNECTION

#include <memory>
#include <boost/asio.hpp>
#include "connection_base.hpp"
#include "../application_layer/layer_base.hpp"

namespace oauth{
namespace protocol{
namespace connection_type{

class sync_connection : public connection_base{
public:
	sync_connection(std::shared_ptr<application_layer::layer_base>& socket_layer)
	{
		socket_layer_ = socket_layer;
		busy = false;
	}
	virtual ~sync_connection(){}
	
	//�ǉ�	
	void operator() (
		const std::string& host,
		boost::asio::streambuf& buf,
		//error_code& ec,
		ReadHandler handler
		)
	{
		if(busy)
		{
			throw std::runtime_error("SOCKET_BUSY");
			//��O�I�I
		}
		busy = true;
		handler_ = handler;

		boost::asio::ip::tcp::resolver resolver(socket_layer_->get_io_service());
		boost::asio::ip::tcp::resolver::query query(host,socket_layer_->service_protocol());
		boost::asio::ip::tcp::resolver::iterator ep_iterator = resolver.resolve(query);
		
		socket_layer_->connect(ep_iterator); //resolve�������Ă���Ȃ炻�̖��O������ɐڑ�(�n���h�V�F�C�N)
		socket_layer_->write(buf); //��������
		socket_layer_->read(boost::bind(&sync_connection::handle_read,this,boost::asio::placeholders::error)); //�ǂݍ���
		
		busy = false;

		return;
	}
private:
	void handle_read(const error_code& ec)
	{
		//std::cout << "SYNC";
		handler_(ec);
		//std::cout << "END";
	}
	
	ReadHandler handler_;
	bool busy;
	/*
	//�ǉ�	
	error_code& operator() (
		boost::asio::ip::tcp::resolver::iterator ep_iterator,
		boost::asio::streambuf& buf,
		error_code& ec
		)
	{
		
		//�񓯊�����
		return ec;
	}*/
};

} // namespace connection_type
} // namespace protocol
} // namespace oauth

#endif
