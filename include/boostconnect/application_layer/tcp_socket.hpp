//
// tcp_socket.hpp
// ~~~~~~~~~~
//
// TCP�ʐM�œ��ʂɍs���邱�Ƃ��s��
//

#ifndef BOOSTCONNECT_APPLAYER_TCP_SOCKET
#define BOOSTCONNECT_APPLAYER_TCP_SOCKET

#include "socket_base.hpp"

namespace bstcon{
namespace application_layer{
  
class tcp_socket : public socket_common<socket_base::tcp_socket_type>{
  typedef socket_common<socket_base::tcp_socket_type> my_base;
public:  
  tcp_socket(io_service& io_service) : my_base(io_service){}
  virtual ~tcp_socket(){}
  
  const std::string service_protocol() const { return "http"; }

  //TCP�ʐM�̃R�l�N�V�����m��
  error_code& connect(endpoint_type& begin,error_code& ec)
  {
    ec = socket_.connect(begin,ec);
    return ec;
  }
  void async_connect(endpoint_type& begin,ConnectHandler handler)
  {
    socket_.async_connect(begin,handler);
    return;
  }

  //TCP�ʐM�ł�Handshake���s��Ȃ� -> Handler�𒼐ڌĂяo��
  void handshake(handshake_type){return;}
  void async_handshake(handshake_type,HandshakeHandler handler){handler(error_code());return;}

  //TCP���C���[�̏���
  void close()
  {
    socket_.close();
    return;
  }
  void shutdown(shutdown_type what)
  {
    socket_.shutdown(what);
    return;
  }
};

} // namespace application_layer
} // namespace bstcon

#endif
