﻿//
// server.hpp
// ~~~~~~~~~~
//
// Main Sever Connection provide class
//

#ifndef BOOSTCONNECT_SERVER_HPP
#define BOOSTCONNECT_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "manager.hpp"
#include "session/session_base.hpp"
#include "session/http_session.hpp"

namespace bstcon{

class server : boost::noncopyable{
private:
    typedef session::http_session SessionType;
    manager<session::session_base> manage_;

public:
    typedef boost::asio::io_service io_service;
    typedef boost::system::error_code error_code;
    typedef session::http_session::RequestHandler RequestHandler;
    typedef session::http_session::CloseHandler CloseHandler;

    server(io_service &io_service,unsigned short port);

#ifdef USE_SSL_BOOSTCONNECT
    typedef boost::asio::ssl::context context;
    server(io_service &io_service,context &ctx,unsigned short port);
#endif

    void start(RequestHandler handler);

private:
    void handle_accept(boost::shared_ptr<session::session_base> new_session,RequestHandler handler,const boost::system::error_code& ec);
    void handle_closed(boost::shared_ptr<session::session_base>& session);

    io_service* io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    const unsigned short port_;
    bool is_ctx_;
#ifdef USE_SSL_BOOSTCONNECT
    context *ctx_;
#endif
};

} // namespace bstcon

#ifdef BOOSTCONNECT_LIB_BUILD
#include "impl/server.ipp"
#endif

#endif
