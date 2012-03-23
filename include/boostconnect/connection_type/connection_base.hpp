//
// connection_base.hpp
// ~~~~~~~~~~
//
// ������񓯊����f�̂��߂́ABoost.Asio���g�p�����N���X�Q
//

#ifndef TWIT_LIB_PROTOCOL_CONNECTTYPE_CONNECTION_BASE
#define TWIT_LIB_PROTOCOL_CONNECTTYPE_CONNECTION_BASE

#include <memory>
#include <boost/asio.hpp>
#include "../application_layer/layer_base.hpp"

namespace oauth{
namespace protocol{
namespace connection_type{
	enum connection_type{async,sync};

class connection_base : boost::noncopyable{
public:
	typedef boost::system::error_code error_code;
	typedef boost::function<void (const error_code&)> ReadHandler;

	connection_base(){}
	virtual ~connection_base(){}

	//�ʐM�J�n(�I�[�o�[���C�h�K�{)
	virtual void operator() (const std::string&,boost::asio::streambuf&,/*error_code&,*/ReadHandler handler = [](const error_code&)->void{}) = 0;

protected:
	std::shared_ptr<application_layer::layer_base> socket_layer_;
};

} // namespace connection_type
} // namespace protocol
} // namespace oauth

#endif