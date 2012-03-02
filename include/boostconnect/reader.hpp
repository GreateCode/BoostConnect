//
// reader.hpp
// ~~~~~~~~~~
//
// ���X�|���X�R���e�i�[
//

#ifndef TWIT_LIB_PROTOCOL_READER
#define TWIT_LIB_PROTOCOL_READER

#include <map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include "response.hpp"

namespace oauth{
namespace protocol{

class reader : boost::noncopyable{
protected:
	typedef std::shared_ptr<oauth::protocol::response> response_type;
	typedef boost::system::error_code error_code;
	typedef boost::function<void (const error_code&)> ReadHandler;
	boost::asio::streambuf read_buf_;
	response_type response_;

public:
	//ctor & dtor
	reader() : response_(new oauth::protocol::response()) { }
	reader(response_type& response) : response_(response) { }
	virtual ~reader() { }

	//Util
	const response_type& get_response() const { return response_; }
	const response_type& reset_response()
	{
		response_.reset(new oauth::protocol::response());
		return response_;
	}
	bool is_chunked() const {return (response_->header.find("Transfer-Encoding")==response_->header.end()) ? false : (response_->header.at("Transfer-Encoding")=="chunked");}

	//
	// ���ʃ����o�֐�
	//
protected:
	//���X�|���X�w�b�_�ǂݍ���
	template<class T>
	const int read_header(const T& source)
	{		
		namespace qi = boost::spirit::qi;

		const auto header_rule = 
			"HTTP/" >> +(qi::char_ - " ") >> " " >> qi::int_ >> " " >> +(qi::char_ - "\r\n") >> ("\r\n") //��s��
			>> *(+(qi::char_ - ": ") >> ": " >> +(qi::char_ - "\r\n") >> "\r\n") //��s�ڂ�map��
			>> ("\r\n"); //"\r\n\r\n"�܂�

		T::const_iterator it = source.cbegin();
		qi::parse(it,source.cend(),header_rule,
			response_->http_version,
			response_->status_code,
			response_->status_message,
			response_->header);
		
		return std::distance(source.cbegin(),it);
	}
	//�`�����N���Q�ƂȈ�������Ԃ��C�`�����N������̒�����Ԃ�
	template<class T>
	const int chunk_parser(const T& source,std::size_t& chunk)
	{
		namespace qi = boost::spirit::qi;
		const auto header_rule = (qi::hex[boost::phoenix::ref(chunk) = qi::_1] - "\r\n") >> *(qi::char_ - "\r\n") >> ("\r\n");

		T::const_iterator it = source.cbegin();
		qi::parse(it,source.cend(),header_rule,chunk);

		return std::distance(source.cbegin(),it);
	}

	//
	// ����
	//
public:
	template<class Socket>
	void read_starter(Socket& socket,/*error_code& ec,*/ReadHandler handler)
	{
		//�w�b�_�[�̂�
		boost::asio::read_until(socket,read_buf_,"\r\n\r\n"/*,ec*/);
		read_buf_.consume(
			read_header((std::string)boost::asio::buffer_cast<const char*>(read_buf_.data()))
		);
		
		if(is_chunked())
		{
			//sync��chunked
			//�������`�����N�p�̊֐��ɓ����邩��K��
			read_chunk_size(socket,handler);
			return;
		}
		
		//�ǂݍ��݂����s
		//�ǂݍ��߂��炻���body�ɒǉ����āC�G���[�R�[�h���ǂ��ł���handler���Ăяo��
		//�G���[�R�[�h��eof�ȊO��������throw����
		error_code ec;
		if(response_->header.find("Content-Length")==response_->header.end())
		{
			while(boost::asio::read(socket,read_buf_,/*boost::asio::transfer_at_least(1)*/boost::asio::transfer_all(),ec));
			//std::cout << ec.message();
			response_->body +=  boost::asio::buffer_cast<const char*>(read_buf_.data());
			read_buf_.consume(read_buf_.size());
		}
		else
		{
			//�����ɂ����Ȃ�"Content-Length"������܂����
			const size_t content_length = boost::lexical_cast<size_t>(response_->header.at("Content-Length"));
			boost::asio::read(socket,read_buf_,
				boost::asio::transfer_at_least(content_length-boost::asio::buffer_size(read_buf_.data())),
				ec
				);
			response_->body += ((std::string)boost::asio::buffer_cast<const char*>(read_buf_.data())).substr(0,content_length);
			read_buf_.consume(content_length);
		}

		handler(ec);
		if(ec && ec!=boost::asio::error::eof && ec.value()!=0x140000DB)
		{
			boost::asio::detail::throw_error(ec,"read_starter");
		}
		return;
	}

protected:
	//chunk�������Ă��铯���ʐM
	//�`�����N�T�C�Y�̕\���s��ǂݏo��
	template<class Socket>
	void read_chunk_size(Socket& socket,ReadHandler handler)
	{
		error_code ec;
		boost::asio::read_until(socket,read_buf_,"\r\n",ec);
		if(ec && ec!=boost::asio::error::eof)
		{
			handler(ec);
			boost::asio::detail::throw_error(ec,"sync_chunk_read");
			//��O�I
		}

		std::size_t chunk;
		read_buf_.consume(chunk_parser((std::string)boost::asio::buffer_cast<const char*>(read_buf_.data()),chunk));
		//chunk��+"\r\n"�܂ŁCread_buf������������
		
		//chunk��0 => body�̏I��
		if(chunk == 0)
		{
			handler(ec);
			return;
		}

		//���̃`�����N�\����body��read�����݂�D
		read_chunk_body(socket,chunk,handler);
		return;
	}
	
	//chunk�w��Ɋ�Â��ď���
	template<class Socket>
	void read_chunk_body(Socket& socket,const std::size_t chunk,ReadHandler handler)
	{
		error_code ec;
		boost::asio::read(socket,read_buf_,
			boost::asio::transfer_at_least(chunk+2-boost::asio::buffer_size(read_buf_.data())),
			ec
			);
		
		//�ǂݍ��񂾂Ƃ���� chunk��+"\r\n" ���Ȃ��ꍇ�̓G���Ƃ��Ĕr��
		if(boost::asio::buffer_size(read_buf_.data()) < chunk + 2)
		{
			handler(ec);
			boost::asio::detail::throw_error(ec,"sync_chunk_less");
			//��O�I
		}

		response_->body.append(boost::asio::buffer_cast<const char*>(read_buf_.data()),chunk/*+2*/);
		read_buf_.consume(chunk+2); //����

		read_chunk_size(socket,handler);
		return;
	}
	
	//
	// �񓯊�
	//
public:
	//�񓯊��ǂݏo���J�n
	template<class Socket>
	void async_read_starter(Socket& socket,ReadHandler handler)
	{
		//�����킩��₷�����������D�n�������D�܂��C�w�b�_��ǂݍ��ݐ؂��Ă����΁D
		boost::asio::async_read_until(socket,
			read_buf_,
			"\r\n\r\n",
			boost::bind(&reader::async_read_header<Socket>,this,
				boost::ref(socket),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				handler));

		return;
	}

protected:
	//�w�b�_�[�����C�������e�n���֓n���D
	template<class Socket>
	void async_read_header(Socket& socket,const error_code& ec,const std::size_t,ReadHandler handler)
	{
		//���X�|���X���A���Ă��Ȃ��H
		if(read_buf_.size()==0)
		{
			handler(ec);
			boost::asio::detail::throw_error(ec,"async_not_response");
			//��O�I
		}

		read_buf_.consume(
			read_header((std::string)boost::asio::buffer_cast<const char*>(read_buf_.data()))
		);

		if(is_chunked())
		{
			//chunked��async�ʐM
			boost::asio::async_read_until(socket,
				read_buf_,
				"\r\n",
				boost::bind(&reader::async_read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
		else if(response_->header.find("Content-Length")==response_->header.end())
		{
			//Content-Length��������Ȃ�async�ʐM
			//�I�������͈Î��I��transfer_all()�@= �ǂ߂邾���ǂݍ���
			boost::asio::async_read(socket,
				read_buf_,
				boost::bind(&reader::async_read_all<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
		else
		{
			//Content-Length�����Ƃ�Read���s���Casync�ʐM
			//�����ɂ����Ȃ�"Content-Length"������܂����
			boost::asio::async_read(socket,
				read_buf_,
				boost::asio::transfer_at_least(boost::lexical_cast<size_t>(response_->header.at("Content-Length"))-boost::asio::buffer_size(read_buf_.data())),
				boost::bind(&reader::async_read_all<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
				
		return;
	}
	
	//�����ɗ���O�ɍŌ�܂œǂݐ؂��Ă�͂�
	template<class Socket>
	void async_read_all(Socket& socket,const error_code& ec,const std::size_t size,ReadHandler handler)
	{
		if(read_buf_.size()==0) 
		{
			handler(ec);
			boost::asio::detail::throw_error(ec,"async_not_response");
		}
		else
		{
			response_->body.append(boost::asio::buffer_cast<const char*>(read_buf_.data()));
			read_buf_.consume(read_buf_.size());

			handler(ec);
		}
				
		return;
	}
		
	//�`�����N�s��ǂݍ��ݏI���Ă�͂��Ȃ̂ŁC�`�����N�ʂ�ǂݏo���D
	//(�Ȃ񂩖������Ǝ�������Ă�悤��)
	template<class Socket>
	void async_read_chunk_size(Socket& socket,const error_code& ec,const std::size_t,ReadHandler handler)
	{
		if(read_buf_.size()==0)
		{
			//�w�b�_�[�ǂݍ��񂾂��甼�[�͗L��͂�
			//�Ƃ������Ƃ́C�����ɗ���ƃ}�Y��
			handler(ec);
			boost::asio::detail::throw_error(ec,"async_read_chunk");
		}
		else if(read_buf_.size()<=2)
		{
			//chunk��+"\r\n"�Ȃ�����������Read���Ă݂悤��
			boost::asio::async_read_until(socket,
				read_buf_,
				"\r\n",
				boost::bind(&reader::async_read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
		else
		{
			//chunk�����Ă�̂ŁCchunk��ǂݍ�����
			//chunk��+"\r\n"�܂ŁCread_buf����������
			std::size_t chunk;
			read_buf_.consume(chunk_parser((std::string)boost::asio::buffer_cast<const char*>(read_buf_.data()),chunk));

			if(chunk == 0) //�I���������
			{
				boost::asio::async_read(socket,
					read_buf_,
					boost::bind(&reader::async_read_end<Socket>,this,
						boost::ref(socket),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred,
						handler));
			}

			//chunk�ʓǂݏo��
			boost::asio::async_read(socket,
				read_buf_,
				boost::asio::transfer_at_least(chunk+2-boost::asio::buffer_size(read_buf_.data())),
				boost::bind(&reader::async_read_chunk_body<Socket>,this,
					boost::ref(socket),
					chunk,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}

		return;
	}

	//�`�����N�̕\���ʂ�ǂ݂����I���Ă�͂������ǁD
	template<class Socket>
	void async_read_chunk_body(Socket& socket,const std::size_t chunk,const error_code& ec,const std::size_t,ReadHandler handler)
	{
		if(read_buf_.size()==0)
		{
			//�Ȃ��񂾂��ǁH
			handler(ec);
			boost::asio::detail::throw_error(ec,"async_read_body");
			//��O�I
		}
		else
		{
			//���Ė{��
			if(boost::asio::buffer_size(read_buf_.data()) < chunk + 2) return;

			//���ɒǉ�
			response_->body.append(boost::asio::buffer_cast<const char*>(read_buf_.data()),chunk);
			read_buf_.consume(chunk+2); //����

			//chunk�擾�ɂ��ǂ��
			boost::asio::async_read_until(socket,
				read_buf_,
				"\r\n",
				boost::bind(&reader::async_read_chunk_size<Socket>,this,
					boost::ref(socket),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					handler));
		}
	}

	//�Ō�C�����܂ŁD
	template<class Socket>
	void async_read_end(Socket& socket,const error_code &ec,const std::size_t,ReadHandler handler)
	{
		if(read_buf_.size() != 0)
		{
			//�I����ĂȂ��c���ƁH
			handler(ec);
			boost::asio::detail::throw_error(ec,"async_not_end");
		}

		handler(ec);
		return; //��Ȃ�I��肾
	}
};

} // namespace protocol
} // namespace oauth

#endif
