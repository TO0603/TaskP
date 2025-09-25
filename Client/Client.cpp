#include "Client.h"
#include "ui_Client.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
    , m_binary_socket(nullptr)
    , m_text_socket(nullptr)
{
    ui->setupUi(this);
    initialize();
}

Client::~Client()
{
    delete ui;
    if( m_binary_socket )
    {
        m_binary_socket->deleteLater();
    }

    if( m_text_socket )
    {
        m_text_socket->deleteLater();
    }
}

void Client::initialize()
{
    connect( ui->connectPushButton    , &QPushButton::clicked, this, &Client::onConnect );      // 接続
    connect( ui->disconnectPushButton , &QPushButton::clicked, this, &Client::onDisconnect );   // 切断
    connect( ui->requestPushButton    , &QPushButton::clicked, this, &Client::onRequest );      // 要求(テスト)
    connect( ui->chatPushButton       , &QPushButton::clicked, this, &Client::onChat );         // チャットメッセージ送信
}

void Client::updateButtons()
{
    if (!ui) return;

    bool anyConnected = (m_binary_socket && m_binary_socket->isValid()) || (m_text_socket && m_text_socket->isValid());

    ui->connectPushButton->setEnabled(!anyConnected);
    ui->disconnectPushButton->setEnabled(anyConnected);
}

bool Client::areSocketsConnected() const
{
    return m_binary_socket && m_text_socket &&
           m_binary_socket->state() == QAbstractSocket::ConnectedState &&
           m_text_socket->state() == QAbstractSocket::ConnectedState;
}

void Client::onConnect()
{
    const QString address = ui->addressLineEdit->text().trimmed();
    if( address.isEmpty() ) // アドレスが空である場合は何もしない
    {
        return;
    }

    m_binary_socket = new QWebSocket();
    m_binary_socket->setParent(this);
    connect( m_binary_socket, &QWebSocket::connected                                               , this, &Client::binaryWebsocketConnected );               // 接続成功
    connect( m_binary_socket, &QWebSocket::disconnected                                            , this, &Client::binaryWebsocketDisconnected );            // 接続切断
    connect( m_binary_socket, &QWebSocket::textMessageReceived                                     , this, &Client::websocketTextMessageReceived );     // テキストメッセージ受信
    connect( m_binary_socket, &QWebSocket::binaryMessageReceived                                   , this, &Client::websocketBinaryMessageReceived );   // バイナリメッセージ受信
    connect( m_binary_socket, QOverload<QAbstractSocket::SocketError>::of( &QWebSocket::error )    , this, &Client::websocketError );                   // エラー
    m_binary_socket->open( QUrl( address ) );

    m_text_socket = new QWebSocket();
    m_text_socket->setParent(this);
    connect( m_text_socket, &QWebSocket::connected                                               , this, &Client::textWebsocketConnected );               // 接続成功
    connect( m_text_socket, &QWebSocket::disconnected                                            , this, &Client::textWebsocketDisconnected );            // 接続切断
    connect( m_text_socket, &QWebSocket::textMessageReceived                                     , this, &Client::websocketTextMessageReceived );     // テキストメッセージ受信
    connect( m_text_socket, &QWebSocket::binaryMessageReceived                                   , this, &Client::websocketBinaryMessageReceived );   // バイナリメッセージ受信
    connect( m_text_socket, QOverload<QAbstractSocket::SocketError>::of( &QWebSocket::error )    , this, &Client::websocketError );                   // エラー
    m_text_socket->open( QUrl( address ) );
}

void Client::onDisconnect()
{
    if( m_binary_socket )
    {
        if( m_binary_socket->isValid() )
        {
            m_binary_socket->close(); // ソケットを明示的に閉じる
        }

        m_binary_socket->deleteLater(); // メモリ解放は安全なタイミングで
        m_binary_socket = nullptr;
    }

    if( m_text_socket )
    {
        if( m_text_socket->isValid() )
        {
            m_text_socket->close(); // ソケットを明示的に閉じる
        }

        m_text_socket->deleteLater(); // メモリ解放は安全なタイミングで
        m_text_socket = nullptr;
    }
}

void Client::onRequest()
{
    if( !areSocketsConnected() ) return;

    // JSON形式のメッセージを作成
    QJsonObject jsonMessage;
    jsonMessage["type"] = QString::fromUtf8( "request");

    QJsonDocument doc( jsonMessage );
    QString message = doc.toJson( QJsonDocument::Compact );

    m_binary_socket->sendTextMessage( message );
}

void Client::onChat()
{
    if( !areSocketsConnected() ) return;

    QString text = ui->chatLineEdit->text().trimmed();
    if( text.isEmpty() ) return; // 何も入力されていない場合は何もしない

    QJsonObject jsonMessage;
    jsonMessage["type"] = QString::fromUtf8( "chat" );
    jsonMessage[ QString::fromUtf8( "chat_message" ) ]   = text;

    QJsonDocument doc( jsonMessage );
    QString message = doc.toJson( QJsonDocument::Compact );

    m_text_socket->sendTextMessage( message );
    ui->chatLineEdit->clear();
}

void Client::binaryWebsocketConnected()
{
    qInfo() << "Binary socket connected";
    updateButtons();
}

void Client::binaryWebsocketDisconnected()
{
    qInfo() << "Binary socket disconnected";
    updateButtons();
}

void Client::textWebsocketConnected()
{
    qInfo() << "Text socket connected";
    updateButtons();
}

void Client::textWebsocketDisconnected()
{
    qInfo() << "Text socket disconnected";
    updateButtons();
}

void Client::websocketTextMessageReceived(const QString& textMessage)
{
    qDebug() << __func__;

    QJsonDocument doc = QJsonDocument::fromJson(textMessage.toUtf8());
    if (!doc.isObject()) return; // JSON 形式でない場合は無視

    QJsonObject jsonObject = doc.object();
    const QString type = jsonObject.value("type").toString();

    if (type == "chat")
    {
        QString chatMessage = jsonObject.value("chat_message").toString();
        ui->chatTextBrowser->append(chatMessage);
    }
}

void Client::websocketBinaryMessageReceived(const QByteArray& binaryMessage)
{
    // メソッド名を出力
    qDebug() << __func__;

    // 受け取ったバイナリのサイズを表示
    qDebug() << "Received binary data size:" << binaryMessage.size() << "bytes";
}

void Client::websocketError( QAbstractSocket::SocketError error )
{
    qDebug() << "WebSocket error:" << error;
}
