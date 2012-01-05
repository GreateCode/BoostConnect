//
// response_container.hpp
// ~~~~~~~~~~
//
// ���X�|���X���܂Ƃ߂Ă����Ă���
//

#ifndef TWIT_LIB_PROTOCOL_RESREADER_RESPONSE_CONTAINER
#define TWIT_LIB_PROTOCOL_RESREADER_RESPONSE_CONTAINER

#include <map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/lexical_cast.hpp>

namespace oauth{
namespace protocol{
namespace response_reader{

class response_container{
public:
	typedef std::map<std::string,std::string> header_type;
	typedef std::string body_type;
	
	typedef boost::system::error_code error_code;

	response_container(){}
	virtual ~response_container(){}

	bool is_chunked() const {return (header_.find("Transfer-Encoding")==header_.end()) ? false : (header_.at("Transfer-Encoding")=="chunked");}
	
	//
	// ����
	//

	//���Ȃ��Ƃ�\r\n\r\n�܂Ŏ��[����Ă���Ɖ���
	template<class T>
	const int read_header(const T& source)
	{		
		namespace qi = boost::spirit::qi;

		const auto header_rule = 
			"HTTP/" >> +(qi::char_ - " ") >> " " >> qi::int_ >> " " >> +(qi::char_ - "\r\n") >> ("\r\n") //��s��
			>> *(+(qi::char_ - ": ") >> ": " >> +(qi::char_ - "\r\n") >> "\r\n") //��s�ڂ�map��
			>> ("\r\n"); //"\r\n\r\n"�܂�

		std::string::const_iterator it = source.cbegin();
		qi::parse(it,source.cend(),header_rule,http_version_,status_code_,status_message_,header_);
		
		return it - source.cbegin();
	}

	//
	// ����
	//
public:
	template<class Socket>
	error_code& read_starter(Socket& socket,error_code& ec)
	{
		//�Ƃ肠�����S���ǂݍ���		
		while(boost::asio::read(socket,read_buf,boost::asio::transfer_at_least(1),ec));
		if(ec && ec != boost::asio::error::eof) return ec;

		//�S��std::string��
		std::string response = boost::asio::buffer_cast<const char*>(read_buf.data());
		read_buf.consume(read_buf.size());

		//���ăw�b�_�[��ǂݍ��݂s�v������
		response.erase(0,read_header(response));

		//�ŁC�{�f�B����
		body_.append(response);

		return ec;
	}
/*
protected:
	//���Ȃ��Ƃ�\r\n\r\n�܂Ŏ��[����Ă���Ɖ���
	bool read_head(boost::asio::streambuf& buf)
	{
		//�w�b�_�����o��
		std::string header = boost::asio::buffer_cast<const char*>(buf.data());
		header.erase(header.find("\r\n\r\n")+4);
		buf.consume(header.size()); //�w�b�_���؂�̂�(���X�|���X�{�f�B���Z������Ɣ�����Ⴄ��H)

		//�p�[�X
		namespace qi = boost::spirit::qi;

		const auto header_rule = 
			"HTTP/" >> +(qi::char_ - " ") >> " " >> qi::int_ >> " " >> +(qi::char_ - "\r\n") >> ("\r\n") //��s��
			>> *(+(qi::char_ - ": ") >> ": " >> +(qi::char_ - "\r\n") >> "\r\n") //��s�ڂ�map��
			>> *("\r\n") >> *qi::eol; //���s���c���Ă���S���X���[
		
		std::string::const_iterator it = header.cbegin();
		bool r = qi::parse(it,header.cend(),header_rule,http_version_,status_code_,status_message_,header_);
		if (it != header.end()) return false;
		return r;
	}

	void read_body(boost::asio::streambuf& buf)
	{
		body_ += boost::asio::buffer_cast<const char*>(buf.data());
		//buf�̏ܖ������؂ꂩ�ƁE�E�E���������Ȃ���H

		bool cutable = header_.find("Content-Length")!=header_.end();
		if(cutable) body_.erase(boost::lexical_cast<int>(header_.at("Content-Length")));

		return;
	}*/

	//
	// �񓯊�
	//
public:
	template<class Socket>
	void async_read_starter(Socket& socket)
	{
		//�����킩��₷�����������D�n�������D
		boost::asio::async_read_until(socket,
			read_buf,
			"\r\n\r\n",
			boost::bind(&response_container::async_read_header<Socket>,this,
				boost::ref(socket),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		return;
	}

protected:
	template<class Socket>
	void async_read_header(Socket& socket,const error_code& ec,const std::size_t)
	{
		if(read_buf.size()==0) return; //�����ƃ��X�|���X�����^�C�v
		read_buf.consume(
			read_header((std::string)boost::asio::buffer_cast<const char*>(read_buf.data()))
		);

		if(is_chunked())
		{
			//async��chunked
			boost::asio::async_read_until(socket,
				read_buf,
				"\r\n",
				boost::bind(&response_container::read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

			return;
		}

		//�����ɂ����Ȃ�"Content-Length"������܂����
		boost::asio::async_read(socket,
			read_buf,
			boost::asio::transfer_at_least(boost::lexical_cast<size_t>(header_.at("Content-Length"))-boost::asio::buffer_size(read_buf.data())),
			boost::bind(&response_container::async_read_body<Socket>,this,
				boost::ref(socket),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
				
		return;
	}
	
	template<class Socket>
	void async_read_body(Socket& socket,const error_code& ec,const std::size_t)
	{
		if(read_buf.size()==0) return; //��H
		else
		{
			body_.append(boost::asio::buffer_cast<const char*>(read_buf.data()));
			read_buf.consume(read_buf.size());
		}

		async_read_starter(socket);
		
		return;
	}

	//�Ȃ񂩖������Ǝ�������Ă�悤��
	template<class Socket>
	void read_chunk_size(Socket& socket,const error_code& ec,const std::size_t)
	{
		if(read_buf.size()==0) return; //�w�b�_�[�ǂݍ��񂾂��甼�[�͗L��͂�
		if(read_buf.size()<=2) //chunk��+"\r\n"�Ȃ��D
		{
			//�`�����N��
			boost::asio::async_read_until(socket,
				read_buf,
				"\r\n",
				boost::bind(&response_container::read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else //chunk�������Ă�
		{
			std::size_t chunk;
			read_buf.consume(chunk_parser((std::string)boost::asio::buffer_cast<const char*>(read_buf.data()),chunk));
			//chunk��+"\r\n"�܂ŁCread_buf������������

			//chunk�ʓǂݏo��
			boost::asio::async_read(socket,
				read_buf,
				boost::asio::transfer_at_least(chunk+2-boost::asio::buffer_size(read_buf.data())),
				boost::bind(&response_container::read_chunk_body<Socket>,this,
					boost::ref(socket),
					chunk,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}

		return;
	}
	template<class Socket>
	void read_chunk_body(Socket& socket,const std::size_t chunk,const error_code& ec,const std::size_t)
	{
		if(read_buf.size()==0) return; //�Ȃ��񂾂��ǁH
		if(chunk == 0) //�I���������
		{
			boost::asio::async_read_until(socket,
				read_buf,
				"\r\n\r\n",
				boost::bind(&response_container::async_read_end<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else //���Ė{��
		{
			if(boost::asio::buffer_size(read_buf.data()) < chunk + 2) return;

			//���ɒǉ�
			body_.append(boost::asio::buffer_cast<const char*>(read_buf.data()),chunk+2);
			read_buf.consume(chunk+2); //����

			//chunk�擾�ɂ��ǂ��
			boost::asio::async_read_until(socket,
				read_buf,
				"\r\n\r\n",
				boost::bind(&response_container::read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
	template<class Socket>
	void async_read_end(Socket& socket,const error_code &ec,const std::size_t)
	{
		if(read_buf.size() == 0) return; //��Ȃ�I��肾�D

		//�w�b�_�[����Ăяo���n�߂�
		boost::asio::async_read_until(socket,
			read_buf,
			"\r\n\r\n",
			boost::bind(&response_container::async_read_header<Socket>,this,
				boost::ref(socket),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

private:
	//�`�����N���Q�ƂȈ�������Ԃ��C�`�����N������̒�����Ԃ�
	template<class T>
	const int chunk_parser(const T& source,std::size_t& chunk)
	{
		namespace qi = boost::spirit::qi;
		const auto header_rule = (qi::hex[boost::phoenix::ref(chunk) = qi::_1] - "\r\n") >> ("\r\n");

		T::const_iterator it = source.cbegin();
		qi::parse(it,source.cend(),header_rule,chunk);

		return it-source.cbegin();
	}

	int status_code_;
	std::string http_version_;
	std::string status_message_;
	header_type header_;
	body_type body_;
	boost::asio::streambuf read_buf;
};

} // namespace application_layer
} // namespace protocol
} // namespace oauth

#endif
