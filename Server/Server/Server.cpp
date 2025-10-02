#include "Server.h"

Server::Server(int port)
    : m_port(port)
{
    initialize();
}

void Server::initialize()
{
    uWS::App::WebSocketBehavior<ClientSession> behavior;

    behavior.open = [this](uWS::WebSocket<false, true, ClientSession>* ws){
        this->onOpen(ws);
    };

    behavior.message = [this](uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode op){
        this->onMessage(ws, message, op);
    };

    behavior.dropped = [this](auto* ws, std::string_view msg, uWS::OpCode op){
        this->onDropped(ws, msg, op);
    };

    behavior.drain = [this](uWS::WebSocket<false, true, ClientSession>* ws){
        this->onDrain(ws);
    };

    behavior.close = [this](uWS::WebSocket<false, true, ClientSession>* ws, int code, std::string_view msg){
        this->onClose(ws, code, msg);
    };

    m_u_web_sockets.ws<ClientSession>("/*", std::move(behavior));

    m_u_web_sockets.listen(m_port, [this](auto* token){
                       if(token)
                           std::cout << "[Server] Listening on port " << m_port << std::endl;
                       else
                           std::cerr << "[Server] Failed to listen on port " << m_port << std::endl;
                   }).run();
}

void Server::onOpen(uWS::WebSocket<false, true, ClientSession>* ws)
{
    std::cout << __func__ << std::endl;
    ws->getUserData()->nextMessageId = 0;
}

void Server::onClose(uWS::WebSocket<false, true, ClientSession>* ws, int, std::string_view)
{
    std::cout << __func__ << std::endl;
}

void Server::onMessage(uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode)
{
    std::cout << "on message" << std::endl;
    nlohmann::json received = nlohmann::json::parse(message);

    if (received.contains("type") && received["type"].get<std::string>() == "request")
    {
        {
            constexpr size_t dataSize = 100ULL * 1024ULL * 1024ULL; // 100MB
            std::vector<uint8_t> binaryData(dataSize);

            for (size_t i = 0; i < dataSize; ++i)
                binaryData[i] = static_cast<uint8_t>(i % 256);

            auto* userData = ws->getUserData();
            uint32_t messageId = userData->nextMessageId++;
            uint64_t payloadSize = binaryData.size();

            // ヘッダ + ペイロード
            std::vector<uint8_t> buffer(sizeof(messageId) + sizeof(payloadSize) + binaryData.size());
            std::memcpy(buffer.data(), &messageId, sizeof(messageId));
            std::memcpy(buffer.data() + sizeof(messageId), &payloadSize, sizeof(payloadSize));
            std::memcpy(buffer.data() + sizeof(messageId) + sizeof(payloadSize), binaryData.data(), binaryData.size());

            // キューに格納
            userData->sendQueue.push_back(std::move(buffer));

            // 送信開始
            sendNext(ws);
        }
    }
}

// 再送も含めて送信
void Server::sendNext(uWS::WebSocket<false, true, ClientSession>* ws)
{
    auto* userData = ws->getUserData();
    if (!userData || userData->sendQueue.empty()) return;

    const auto& data = userData->sendQueue.front();
    bool sent = ws->send(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()), uWS::BINARY);

    if (sent) {
        userData->sendQueue.pop_front();
        if (!userData->sendQueue.empty()) sendNext(ws);
    }
}

void Server::onDropped(uWS::WebSocket<false, true, ClientSession>* ws, std::string_view /*msg*/, uWS::OpCode /*op*/)
{
    std::cout << "Message dropped, will retry..." << std::endl;
    sendNext(ws);
}

void Server::onDrain(uWS::WebSocket<false, true, ClientSession>* ws)
{
    std::cout << "Drain event, sending queued messages..." << std::endl;
    sendNext(ws);
}
