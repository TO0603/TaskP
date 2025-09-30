#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
#include <uwebsockets/App.h>
#else
#include <App.h>
#endif
#include "../Shared/json.hpp"

#include <kvs/HydrogenVolumeData>
#include <kvs/TransferFunction>
#include <kvs/RGBColor>
#include <kvs/CellByCellMetropolisSampling>

struct ClientSession
{
    uWS::WebSocket<false,true,ClientSession>* binary_ws = nullptr;
    uWS::WebSocket<false,true,ClientSession>* text_ws   = nullptr;
    int userNumber      = -1;
    std::string uuid    = "";
    bool isOperator     = false;
};

class Server
{
public:
    Server( int port );

private:
    uWS::App m_u_web_sockets;
    int m_port;
    std::unordered_map<std::string, ClientSession> m_users;
    // std::map<int, uWS::WebSocket<false, true, ClientSession>*> m_users;
    int m_next_user_number = 0;

    void initialize();

    void onOpen( uWS::WebSocket<false, true, ClientSession>* ws, std::string socketType );
    void onClose( uWS::WebSocket<false, true, ClientSession>* ws, int /*code*/, std::string_view /*msg*/ );
    void onMessage( uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode );
};

#endif // SERVER_H
