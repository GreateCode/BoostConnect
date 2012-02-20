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
	typedef boost::function<void (const error_code&)> ReadHandler;

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
	error_code& read_starter(Socket& socket,error_code& ec,ReadHandler handler)
	{
		//�w�b�_�[�̂�
		boost::asio::read_until(socket,read_buf,"\r\n\r\n",ec);
		read_buf.consume(
			read_header((std::string)boost::asio::buffer_cast<const char*>(read_buf.data()))
		);
		
		if(is_chunked())
		{
			//sync��chunked
			read_chunk_size(socket,ec,handler);
			return ec;
		}
		
		if(header_.find("Content-Length")==header_.end())
		{
			while(boost::asio::read(socket,read_buf,boost::asio::transfer_at_least(1),ec));
			body_ +=  boost::asio::buffer_cast<const char*>(read_buf.data());
			read_buf.consume(read_buf.size());
		}
		else
		{
			//�����ɂ����Ȃ�"Content-Length"������܂����
			const int content_length = boost::lexical_cast<size_t>(header_.at("Content-Length"));
			boost::asio::read(socket,read_buf,
				boost::asio::transfer_at_least(content_length-boost::asio::buffer_size(read_buf.data())),
				ec
				);
			body_ += ((std::string)boost::asio::buffer_cast<const char*>(read_buf.data())).substr(0,content_length);
			read_buf.consume(content_length);
		}



		//�Ƃ肠�����S���ǂݍ���		
		//while(boost::asio::read(socket,read_buf,boost::asio::transfer_at_least(1),ec));
		//if(ec && ec != boost::asio::error::eof) return ec;

		//�S��std::string��
		//std::string response = boost::asio::buffer_cast<const char*>(read_buf.data());
		//read_buf.consume(read_buf.size());

		//���ăw�b�_�[��ǂݍ��݂s�v������
		//response.erase(0,read_header(response));

		//�ŁC�{�f�B����
		//body_.append(response);
		handler(ec);
		return ec;
	}
	
	template<class Socket>
	error_code& read_chunk_size(Socket& socket,error_code& ec,ReadHandler handler)
	{
		boost::asio::read_until(socket,read_buf,"\r\n",ec);

		std::size_t chunk;
		read_buf.consume(chunk_parser((std::string)boost::asio::buffer_cast<const char*>(read_buf.data()),chunk));
		//chunk��+"\r\n"�܂ŁCread_buf������������
		
		if(chunk == 0) return read_end(socket,ec,handler);
		return read_chunk_body(socket,chunk,ec,handler);
	}
	
	template<class Socket>
	error_code& read_chunk_body(Socket& socket,const std::size_t chunk,error_code& ec,ReadHandler handler)
	{
		boost::asio::read(socket,read_buf,
			boost::asio::transfer_at_least(chunk+2-boost::asio::buffer_size(read_buf.data())),
			ec
			);
		
		if(boost::asio::buffer_size(read_buf.data()) < chunk + 2) return ec;
		body_.append(boost::asio::buffer_cast<const char*>(read_buf.data()),chunk+2);
		read_buf.consume(chunk+2); //����

		return read_chunk_size(socket,ec,handler);
	}
	template<class Socket>
	error_code& read_end(Socket& socket,error_code &ec,ReadHandler handler)
	{
		boost::asio::read(socket,read_buf,boost::asio::transfer_at_least(1),ec);
		if(ec == boost::asio::error::eof || read_buf.size() == 0)
		{
			handler(ec);
			return ec; //��Ȃ�I��肾�D
		}
		return ec; //�����̓G���[
	}


	//
	// �񓯊�
	//
public:
	template<class Socket>
	void async_read_starter(Socket& socket,ReadHandler handler)
	{
		//�����킩��₷�����������D�n�������D
		boost::asio::async_read_until(socket,
			read_buf,
			"\r\n\r\n",
			boost::bind(&response_container::async_read_header<Socket>,this,
				boost::ref(socket),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				handler));

		return;
	}

	int status_code_;
	std::string http_version_;
	std::string status_message_;
	header_type header_;
	body_type body_;

protected:

	template<class Socket>
	void async_read_header(Socket& socket,const error_code& ec,const std::size_t,ReadHandler handler)
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
				boost::bind(&response_container::async_read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));

			return;
		}
		if(header_.find("Content-Length")==header_.end())
		{
			boost::asio::async_read(socket,
				read_buf,
				boost::bind(&response_container::async_read_all<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));

			return;
		}
		else
		{
			//�����ɂ����Ȃ�"Content-Length"������܂����
			boost::asio::async_read(socket,
				read_buf,
				boost::asio::transfer_at_least(boost::lexical_cast<size_t>(header_.at("Content-Length"))-boost::asio::buffer_size(read_buf.data())),
				boost::bind(&response_container::async_read_all<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
				
		return;
	}
		
	template<class Socket>
	void async_read_all(Socket& socket,const error_code& ec,const std::size_t size,ReadHandler handler)
	{
		if(read_buf.size()==0) 
		{
			handler(ec);
		}
		else
		{
			body_.append(boost::asio::buffer_cast<const char*>(read_buf.data()));
			read_buf.consume(read_buf.size());

			boost::asio::async_read(socket,
				read_buf,
				boost::bind(&response_container::async_read_all<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
				
		return;
	}
		
	/*template<class Socket>
	void async_read_body(Socket& socket,const error_code& ec,const std::size_t,ReadHandler handler)
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
	*/
	//�Ȃ񂩖������Ǝ�������Ă�悤��
	template<class Socket>
	void async_read_chunk_size(Socket& socket,const error_code& ec,const std::size_t,ReadHandler handler)
	{
		if(read_buf.size()==0) return; //�w�b�_�[�ǂݍ��񂾂��甼�[�͗L��͂�
		if(read_buf.size()<=2) //chunk��+"\r\n"�Ȃ��D
		{
			//�`�����N��
			boost::asio::async_read_until(socket,
				read_buf,
				"\r\n",
				boost::bind(&response_container::async_read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
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
				boost::bind(&response_container::async_read_chunk_body<Socket>,this,
					boost::ref(socket),
					chunk,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}

		return;
	}
	template<class Socket>
	void async_read_chunk_body(Socket& socket,const std::size_t chunk,const error_code& ec,const std::size_t,ReadHandler handler)
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
					boost::asio::placeholders::bytes_transferred,
					handler));
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
				boost::bind(&response_container::async_read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
	}
	template<class Socket>
	void async_read_end(Socket& socket,const error_code &ec,const std::size_t,ReadHandler handler)
	{
		if(read_buf.size() == 0)
		{
			handler(ec);
			return; //��Ȃ�I��肾�D
		}

		//�w�b�_�[����Ăяo���n�߂�
		boost::asio::async_read_until(socket,
			read_buf,
			"\r\n\r\n",
			boost::bind(&response_container::async_read_header<Socket>,this,
				boost::ref(socket),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				handler));
	}

private:
	//�`�����N���Q�ƂȈ�������Ԃ��C�`�����N������̒�����Ԃ�
	template<class T>
	const int chunk_parser(const T& source,std::size_t& chunk)
	{
		namespace qi = boost::spirit::qi;
		const auto header_rule = (qi::hex[boost::phoenix::ref(chunk) = qi::_1] - "\r\n") >> *(qi::char_ - "\r\n") >> ("\r\n");

		T::const_iterator it = source.cbegin();
		qi::parse(it,source.cend(),header_rule,chunk);

		return it-source.cbegin();
	}
	boost::asio::streambuf read_buf;
};

} // namespace application_layer
} // namespace protocol
} // namespace oauth

#endif
