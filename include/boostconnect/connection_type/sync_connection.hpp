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
//#include "../application_layer/layer_base.hpp"

namespace oauth{
namespace protocol{
namespace connection_type{

class sync_connection : public connection_base{
public:
	sync_connection(const std::shared_ptr<application_layer::socket_base>& socket)
	{
		socket_ = socket;
		busy = false;
	}
	virtual ~sync_connection(){}
	
	//�ǉ�
	response_type operator() (
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

		boost::asio::ip::tcp::resolver resolver(socket_->get_io_service());
		boost::asio::ip::tcp::resolver::query query(host,socket_->service_protocol());
		boost::asio::ip::tcp::resolver::iterator ep_iterator = resolver.resolve(query);

		boost::asio::connect(socket_->lowest_layer(),ep_iterator);

	//	socket_->connect((boost::asio::ip::tcp::endpoint)*ep_iterator); //resolve�������Ă���Ȃ炻�̖��O������ɐڑ�(�n���h�V�F�C�N)
		socket_->handshake(application_layer::socket_base::ssl_socket_type::client);
		
		boost::asio::write(*socket_.get(),buf);

		//socket_layer_->connect(ep_iterator);
		//socket_layer_->write(buf); //��������

		// TODO
		reader_.reset(new reader());
		reader_->read_starter(*socket_.get(),boost::bind(&sync_connection::handle_read,this,boost::asio::placeholders::error));

		//response_.reset(new oauth::protocol::response());
		//socket_layer_->read(boost::bind(&sync_connection::handle_read,this,boost::asio::placeholders::error)); //�ǂݍ���
		
		busy = false;

		return reader_->get_response();
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
