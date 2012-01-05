#include <twit-library/protocol.hpp>
#include <map>
#include <boost/thread.hpp>
#include <boost/property_tree/ptree.hpp>

int main()
{
	//�����̏����ł�
	boost::asio::io_service io_service;
	boost::asio::ssl::context ctx(io_service,boost::asio::ssl::context_base::sslv3_client); //SSL�p
	boost::system::error_code ec;
	const std::string host("www.hatena.ne.jp");
	boost::asio::streambuf buf;
	std::ostream os(&buf);
	os << "GET / HTTP/1.1\r\nHost: " << host << "\r\nConnection: close\r\n\r\n"; //�����̃��N�G�X�g
	
	////
	//// �����EHTTP�ʐM
	////
	//{
	//	oauth::protocol::client client(io_service/*,oauth::protocol::connection_type::sync*/);
	//	client(host,buf,ec);
	//	
	//	//�I��
	//}

	////
	//// �����ESSL�ʐM
	////
	//{
	//	oauth::protocol::client client(io_service,ctx/*,oauth::protocol::connection_type::sync*/);
	//	client(host,buf,ec);

	//	//�I��
	//}
	
	////
	//// �񓯊��EHTTP�ʐM
	////
	//{
	//	oauth::protocol::client client(io_service,oauth::protocol::connection_type::async);
	//	client(host,buf,ec);

	//	//����

	//	io_service.run(); //�ʐM�𑖂点��

	//	//�I��
	//	return 0;
	//}

	//
	// �񓯊��ESSL�ʐM
	//
	{
		oauth::protocol::client client(io_service,ctx,oauth::protocol::connection_type::async);
		client(host,buf,ec);

		//����

		io_service.run(); //�ʐM�𑖂点��

		//�I��
	}
	
	return 0;
}
