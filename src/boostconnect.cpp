#define USE_SSL_BOOSTCONNECT

#include <boostconnect/application_layer/socket_base.hpp>
#include <boostconnect/application_layer/impl/socket_base.ipp>
#include <boostconnect/application_layer/tcp_socket.hpp>
#include <boostconnect/application_layer/impl/tcp_socket.ipp>
#include <boostconnect/application_layer/ssl_socket.hpp>
#include <boostconnect/application_layer/impl/ssl_socket.ipp>

#include <boostconnect/connection_type/connection_base.hpp>
#include <boostconnect/connection_type/impl/connection_base.ipp>
#include <boostconnect/connection_type/async_connection.hpp>
#include <boostconnect/connection_type/impl/async_connection.ipp>
#include <boostconnect/connection_type/sync_connection.hpp>
#include <boostconnect/connection_type/impl/sync_connection.ipp>

#include <boostconnect/session/session_base.hpp>
#include <boostconnect/session/impl/session_base.ipp>
#include <boostconnect/session/http_session.hpp>
#include <boostconnect/session/impl/http_session.ipp>

#include <boostconnect/system/error_code.hpp>
#include <boostconnect/system/impl/error_code.ipp>

#include <boostconnect/request.hpp>
#include <boostconnect/response.hpp>

#include <boostconnect/manager.hpp>
#include <boostconnect/impl/manager.ipp>
#include <boostconnect/client.hpp>
#include <boostconnect/impl/client.ipp>
#include <boostconnect/server.hpp>
#include <boostconnect/impl/server.ipp>
