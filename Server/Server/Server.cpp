#include "Server.h"

constexpr size_t CHUNK_SIZE = 1 * 1024 * 1024; // 1MBごとに送信

Server::Server(int port)
    : m_port(port)
{
    initialize();
}

void Server::initialize()
{
    uWS::App::WebSocketBehavior<ClientSession> behavior;

    // 接続確立時に呼ばれる。ユーザデータの初期化やログ出力に使用
    behavior.open = [this](uWS::WebSocket<false, true, ClientSession>* ws)
    {
        this->onOpen(ws);
    };

    // メッセージ受信時に呼ばれる。バイナリやテキストを処理する
    behavior.message = [this](uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode op)
    {
        this->onMessage(ws, message, op);
    };

    // メッセージがドロップされた場合に呼ばれる
    // maxBackpressure や closeOnBackpressureLimit により送信失敗時に発生
    behavior.dropped = [this](auto* ws, std::string_view msg, uWS::OpCode op){
        this->onDropped(ws, msg, op);
    };

    // バッファが空き、送信可能になったときに呼ばれる
    // sendQueue に残ったメッセージを送信再開するタイミングで使用
    behavior.drain = [this](uWS::WebSocket<false, true, ClientSession>* ws){
        this->onDrain(ws);
    };

    // クライアントが切断したときに呼ばれる
    // クリーンアップやログ出力に使用
    behavior.close = [this](uWS::WebSocket<false, true, ClientSession>* ws, int code, std::string_view msg)
    {
        this->onClose(ws, code, msg);
    };

    behavior.maxBackpressure = 16 * 1024 * 1024;   // 16MB
    behavior.closeOnBackpressureLimit = false;      // limit に達しても切断しない
    m_u_web_sockets.ws<ClientSession>("/*", std::move(behavior));

    m_u_web_sockets.listen(m_port, [this](auto* token){
                       if(token)
                       {
                           std::cout << "[Server] Listening on port " << m_port << std::endl;
                       }
                       else
                       {
                           std::cerr << "[Server] Failed to listen on port " << m_port << std::endl;
                       }
                   }).run();
}

void Server::onOpen(uWS::WebSocket<false, true, ClientSession>* ws)
{

    ws->getUserData()->nextMessageId = 0;
}

void Server::onClose(uWS::WebSocket<false, true, ClientSession>* ws, int, std::string_view)
{
    std::cout << "[Server] onClose" << std::endl;
}

void Server::onMessage(uWS::WebSocket<false, true, ClientSession>* ws, std::string_view message, uWS::OpCode)
{
    std::cout << "[Server] onMessage" << std::endl;
    nlohmann::json received = nlohmann::json::parse(message);

    if (received.contains("type") && received["type"].get<std::string>() == "request")
    {
        constexpr size_t dataSize = 100ULL * 1024ULL * 1024ULL; // 100MB
        std::vector<uint8_t> binaryData(dataSize);
        for (size_t i = 0; i < dataSize; ++i)
            binaryData[i] = static_cast<uint8_t>(i % 256);

        auto* userData = ws->getUserData();
        uint32_t messageId = userData->nextMessageId++;

        // チャンク化してキューに追加
        size_t offset = 0;
        uint32_t chunkIndex = 0;
        while(offset < binaryData.size())
        {
            size_t chunkSize = std::min(CHUNK_SIZE, binaryData.size() - offset);

            std::vector<uint8_t> buffer(sizeof(messageId) + sizeof(chunkIndex) + sizeof(uint64_t) + chunkSize);
            size_t pos = 0;

            std::memcpy(buffer.data() + pos, &messageId, sizeof(messageId));
            pos += sizeof(messageId);

            std::memcpy(buffer.data() + pos, &chunkIndex, sizeof(chunkIndex));
            pos += sizeof(chunkIndex);

            uint64_t payloadSize = chunkSize;
            std::memcpy(buffer.data() + pos, &payloadSize, sizeof(payloadSize));
            pos += sizeof(payloadSize);

            std::memcpy(buffer.data() + pos, binaryData.data() + offset, chunkSize);

            userData->sendQueue.push_back(std::move(buffer));

            offset += chunkSize;
            ++chunkIndex;
        }

        // 送信開始
        sendNext(ws);
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
    } else {
        std::cout << "Backpressure, wait for drain" << std::endl;
    }
}

void Server::onDropped(uWS::WebSocket<false, true, ClientSession>* ws,
                       std::string_view /*msg*/, uWS::OpCode /*op*/)
{
    std::cout << "Message dropped. Waiting for drain before retry..." << std::endl;
}

void Server::onDrain(uWS::WebSocket<false, true, ClientSession>* ws)
{
    std::cout << "Drain event, sending queued messages..." << std::endl;
    sendNext(ws);
}
