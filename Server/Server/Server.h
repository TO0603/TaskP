#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
#include <uwebsockets/App.h>
#else
#include <App.h>
#endif
#include "../../Shared/json.hpp"
#include <deque>

struct ClientSession
{
    std::deque<std::vector<uint8_t>> sendQueue;  // 再送用の送信キュー
    uint32_t nextMessageId = 0;                  // メッセージ番号カウンタ
};
class Server
{
public:
    Server( int port );

private:
    uWS::App m_u_web_sockets;
    int m_port;

    void initialize();

    void onOpen( uWS::WebSocket<false, true, ClientSession>* ws );
    void onClose( uWS::WebSocket<false, true, ClientSession>* ws, int /*code*/, std::string_view /*msg*/ );
    void onMessage( uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode );

    void sendNext(uWS::WebSocket<false, true, ClientSession>* ws);
    void onDropped(uWS::WebSocket<false, true, ClientSession>* ws, std::string_view /*msg*/, uWS::OpCode /*op*/);
    void onDrain(uWS::WebSocket<false, true, ClientSession> *ws);
};

#endif // SERVER_H
