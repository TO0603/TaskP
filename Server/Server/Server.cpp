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
                                          .drain = []( uWS::WebSocket<false, true, ClientSession>* ws )
                                          {
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

void sendBinaryInChunks(uWS::WebSocket<false, true, ClientSession>* ws, std::vector<uint8_t>& data) {
    constexpr size_t chunkSize = 10ULL * 1024ULL * 1024ULL; // 10MB
    size_t offset = 0;

    while(offset < data.size()) {
        size_t sz = std::min(chunkSize, data.size() - offset);
        ws->send(std::string_view(reinterpret_cast<char*>(data.data() + offset), sz), uWS::BINARY);
        offset += sz;
    }
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

    if (received.contains("type") && received["type"].get<std::string>() == "request") {
        // 500MB のバイナリデータを作成
        constexpr size_t dataSize = 500ULL * 1024ULL * 1024ULL;
        std::vector<uint8_t> binaryData(dataSize);
        for (size_t i = 0; i < dataSize; ++i) binaryData[i] = static_cast<uint8_t>(i % 256);

        std::cout << "Generated 500MB binary data, start sending..." << std::endl;
        sendBinaryInChunks(ws, binaryData); // チャンク送信
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
