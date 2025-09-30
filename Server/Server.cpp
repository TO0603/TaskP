#include "Server.h"

Server::Server( int port )
    : m_port( port )
{
    initialize();
}

void Server::initialize()
{
    // バイナリ用 WebSocket
    m_u_web_sockets.ws<ClientSession>("/binary", {
                                                     .open = [this](auto* ws){
                                                      // std::cout << "binary" << std::endl;
                                                         this->onOpen(ws, "binary");
                                                     },
                                                     .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode){
                                                         nlohmann::json received = nlohmann::json::parse(message);
                                                         // std::cout << "binary" << std::endl;
                                                         this->onMessage( ws, message, opCode );
                                                     },
                                                     .drain = [](auto* ws){
                                                         size_t remaining = ws->getBufferedAmount();
                                                         std::cout << "[Server] Remaining bytes to send (binary): " << remaining << std::endl;
                                                     },
                                                     .close = [this](auto* ws, int code, std::string_view msg){
                                                         this->onClose(ws, code, msg);
                                                     }
                                                 });

    // テキスト用 WebSocket
    m_u_web_sockets.ws<ClientSession>("/text", {
                                                   .open = [this](auto* ws){
                                                    // std::cout << "text" << std::endl;
                                                       this->onOpen(ws, "text");
                                                   },
                                                   .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode){
                                                       nlohmann::json received = nlohmann::json::parse(message);
                                                       // std::cout << "text" << std::endl;
                                                       this->onMessage( ws, message, opCode );
                                                   },
                                                   .drain = [](auto* ws){
                                                       size_t remaining = ws->getBufferedAmount();
                                                       std::cout << "[Server] Remaining bytes to send (text): " << remaining << std::endl;
                                                   },
                                                   .close = [this](auto* ws, int code, std::string_view msg){
                                                       this->onClose(ws, code, msg);
                                                   }
                                               });

    m_u_web_sockets.listen( m_port, [this]( auto* token )
                           {
                               if( token )
                                   std::cout << "[Server] Listening on port " << m_port << std::endl;
                               else
                                   std::cerr << "[Server] Failed to listen on port " << m_port << std::endl;
                           } ).run();
}

void Server::onOpen( uWS::WebSocket<false, true, ClientSession>* ws, std::string socketType )
{
    std::cout << "[Server] Connection opened (" << socketType << ")" << std::endl;
}

void Server::onClose( uWS::WebSocket<false, true, ClientSession>* ws, int, std::string_view )
{
    for(auto it = m_users.begin(); it != m_users.end(); )
    {
        auto& session = it->second;
        if(session.binary_ws == ws) session.binary_ws = nullptr;
        if(session.text_ws   == ws) session.text_ws   = nullptr;

        if(!session.binary_ws && !session.text_ws)
        {
            std::cout << "[Server] User " << it->first << " disconnected" << std::endl;
            it = m_users.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Server::onMessage( uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode )
{
    nlohmann::json received;
    received = nlohmann::json::parse(message);

    if(received.contains("type") && received["type"] == "join")
    {
        std::string user_uuid = received["uuid"];   // クライアントから送られてきたUUID
        std::string channel = received["channel"];  // "binary" or "text"

        auto it = m_users.find(user_uuid);
        if( it == m_users.end() )
        {
            // 新規ユーザの場合
            ClientSession session;
            session.uuid = user_uuid;
            session.userNumber = m_next_user_number++;
            if( channel == "binary" ) session.binary_ws = ws;
            else if( channel == "text" ) session.text_ws = ws;
            m_users[user_uuid] = session;
            std::cout << "[Server] User " << session.uuid << " (userNumber=" << session.userNumber << ") connected (" << channel << ")" << std::endl;
        }
        else
        {
            // 既存ユーザの場合、接続してきたチャンネルだけセット
            if( channel == "binary" ) it->second.binary_ws = ws;
            else if( channel == "text" ) it->second.text_ws = ws;

            std::cout << "[Server] User " << it->second.uuid << " (userNumber=" << it->second.userNumber << ") connected (" << channel << ")" << std::endl;
        }
    }
    else if(received.contains("type") && received["type"] == "curUsers")
    {
        std::cout << "[Server] Current connected users:" << std::endl;
        for (auto& [uuid, session] : m_users)
        {
            // session は ClientSession のオブジェクトなので '.' を使う
            std::cout << "  userNumber: " << session.userNumber << ", uuid: " << session.uuid << std::endl;
        }
    }
    else if (received.contains("type") && received["type"].get<std::string>() == "request")
    {
        auto* volume = new kvs::HydrogenVolumeData( { 32, 32, 32 } );
        const auto repeat = 4; // number of repetitions
        const auto step = 0.5f; // sampling step
        const auto tfunc = kvs::TransferFunction( 256 ); // transfer function
        auto* object = new kvs::CellByCellMetropolisSampling( volume, repeat, step, tfunc );
        delete volume;

        const size_t numberOfVertices = object->numberOfVertices();
        const kvs::ValueArray<kvs::Real32>& coords = object->coords();
        const kvs::ValueArray<kvs::UInt8>& colors = object->colors();
        const kvs::ValueArray<kvs::Real32>& normals = object->normals();
        const kvs::Vec3& minObjectCoords = object->minObjectCoord();
        const kvs::Vec3& maxObjectCoords = object->maxObjectCoord();

        size_t total_size =
            sizeof( size_t ) +
            sizeof( kvs::Real32 ) * 3 * numberOfVertices +
            sizeof( kvs::UInt8 )  * 3 * numberOfVertices +
            sizeof( kvs::Real32 ) * 3 * numberOfVertices +
            sizeof( kvs::Real32 ) * 3 +
            sizeof( kvs::Real32 ) * 3;

        std::vector<char> buffer( total_size );
        size_t offset = 0;
        std::memcpy( buffer.data() + offset, &numberOfVertices, sizeof( size_t ) );
        offset += sizeof( size_t );
        std::memcpy( buffer.data() + offset, coords.data(), sizeof( kvs::Real32 ) * 3 * numberOfVertices );
        offset += sizeof( kvs::Real32 ) * 3 * numberOfVertices;
        std::memcpy( buffer.data() + offset, colors.data(), sizeof( kvs::UInt8 ) * 3 * numberOfVertices );
        offset += sizeof( kvs::UInt8 ) * 3 * numberOfVertices;
        std::memcpy( buffer.data() + offset, normals.data(), sizeof( kvs::Real32 ) * 3 * numberOfVertices );
        offset += sizeof( kvs::Real32 ) * 3 * numberOfVertices;
        std::memcpy( buffer.data() + offset, minObjectCoords.data(), sizeof( kvs::Real32 ) * 3 );
        offset += sizeof( kvs::Real32 ) * 3;
        std::memcpy( buffer.data() + offset, maxObjectCoords.data(), sizeof( kvs::Real32 ) * 3 );
        offset += sizeof( kvs::Real32 ) * 3;

        delete object;

        ws->send( std::string_view( buffer.data(), buffer.size() ), uWS::OpCode::BINARY );
    }
    else if (received.contains("type") && received["type"].get<std::string>() == "chat")
    {
        if (received.contains("chat_message") && received["chat_message"].is_string()) {
            std::string chat = received["chat_message"].get<std::string>();
            nlohmann::json chat_message =
                {
                    { "type", "chat" },
                    { "chat_message", chat }
                };
            ws->send(chat_message.dump(), uWS::OpCode::TEXT); // send は文字列に変換
        } else {
            std::cout << "[Warning] chat_message missing or invalid type" << std::endl;
        }
    }
}
