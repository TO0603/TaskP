#include "Server.h"

Server::Server( int port )
    : m_port( port )
{
    initialize();
}

void Server::initialize()
{
    m_u_web_sockets.ws<ClientSession>( "/*",
                                      {
                                          .open = [this]( uWS::WebSocket<false, true, ClientSession>* ws )
                                          {
                                              this->onOpen( ws );
                                          },
                                          .message = [this]( uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode opCode )
                                          {
                                              this->onMessage( ws, message, opCode );
                                          },
                                          .drain = [](uWS::WebSocket<false, true, ClientSession>* ws)
                                          {
                                              // 未送信バイト数を取得して表示
                                              size_t remaining = ws->getBufferedAmount();
                                              std::cout << "[Server] Remaining bytes to send: " << remaining << std::endl;
                                          },
                                          .close = [this]( uWS::WebSocket<false, true, ClientSession>* ws, int code, std::string_view msg )
                                          {
                                              this->onClose( ws, code, msg );
                                          }
                                      } );

    m_u_web_sockets.listen( m_port, [this]( auto* token )
                           {
                               if( token )
                                   std::cout << "[Server] Listening on port " << m_port << std::endl;
                               else
                                   std::cerr << "[Server] Failed to listen on port " << m_port << std::endl;
                           } ).run();
}

void Server::onOpen( uWS::WebSocket<false, true, ClientSession>* ws )
{
    std::cout << __func__ << std::endl;
}

void Server::onClose( uWS::WebSocket<false, true, ClientSession>* ws, int, std::string_view )
{
    std::cout << __func__ << std::endl;
}

void Server::onMessage( uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode )
{
    std::cout << __func__ << std::endl;
    nlohmann::json received;
    received = nlohmann::json::parse(message);

    if (received.contains("type") && received["type"].get<std::string>() == "request")
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
