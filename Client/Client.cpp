#include "Client.h"
#include "ui_Client.h"

Client::Client( kvs::qt::Application& app, QWidget *parent )
    : QMainWindow(parent)
    , ui(new Ui::Client)
    , m_screen( new kvs::qt::Screen( &app ) )
    , m_compositor( new kvs::StochasticRenderingCompositor( m_screen->scene() ) )
    , m_binary_socket(nullptr)
    , m_text_socket(nullptr)
{
    ui->setupUi(this);
    initialize();
}

Client::~Client()
{
    delete ui;
    ui = nullptr;

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
    m_compositor->setRepetitionLevel( 4 ); // コンポジターのリピートレベルを設定 初期値:4
    m_screen->setEvent( m_compositor );
    m_screen->setFixedSize( 620, 620 );
    ui->screenArea->addWidget( m_screen );

    connect( ui->connectPushButton    , &QPushButton::clicked, this, &Client::onConnect );      // 接続
    connect( ui->disconnectPushButton , &QPushButton::clicked, this, &Client::onDisconnect );   // 切断
    connect( ui->requestPushButton    , &QPushButton::clicked, this, &Client::onRequest );      // 要求(テスト)
    connect( ui->chatPushButton       , &QPushButton::clicked, this, &Client::onChat );         // チャットメッセージ送信
    connect( ui->debugPushButton      , &QPushButton::clicked, this, &Client::onDebug );       // デバッグ
    this->show();
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

void Client::registerObject( kvs::PointObject* pointObject )
{
    kvs::glsl::ParticleBasedRenderer* renderer = new kvs::glsl::ParticleBasedRenderer();
    renderer->enableShuffle();

    kvs::Xform m_initial_camera_xfom
        (
            kvs::Mat4(
                1, 0, 0, 0 ,
                0, 1, 0, 0 ,
                0, 0, 1, 12,
                0, 0, 0, 1
                )
            );

    kvs::Vec3 translationOffset = m_screen->scene()->camera()->xform().translation() - m_initial_camera_xfom.translation();
    renderer->setTranslationOffset( translationOffset );
    renderer->setObjectDepth( m_screen->scene()->objectManager()->xform().scaling().z() / m_screen->scene()->camera()->xform().scaling().z() );

    m_server_point_object_ids = m_screen->scene()->registerObject( pointObject, renderer );
    m_screen->update();
}

void Client::replaceObject( kvs::PointObject* pointObject )
{
    m_screen->scene()->replaceObject( m_server_point_object_ids.first, pointObject );
    m_screen->update();
}

void Client::onConnect()
{
    const QString address = ui->addressLineEdit->text().trimmed();
    const QString binaryAddress = address + "/binary";
    const QString textDddress = address + "/text";
    if( address.isEmpty() ) // アドレスが空である場合は何もしない
    {
        return;
    }

    m_binary_socket = new QWebSocket();
    m_binary_socket->setParent(this);
    connect( m_binary_socket, &QWebSocket::connected                                               , this, &Client::binaryWebsocketConnected );         // 接続成功
    connect( m_binary_socket, &QWebSocket::disconnected                                            , this, &Client::binaryWebsocketDisconnected );      // 接続切断
    connect( m_binary_socket, &QWebSocket::textMessageReceived                                     , this, &Client::websocketTextMessageReceived );     // テキストメッセージ受信
    connect( m_binary_socket, &QWebSocket::binaryMessageReceived                                   , this, &Client::websocketBinaryMessageReceived );   // バイナリメッセージ受信
    connect( m_binary_socket, QOverload<QAbstractSocket::SocketError>::of( &QWebSocket::error )    , this, &Client::websocketError );                   // エラー
    m_binary_socket->open( QUrl( binaryAddress ) );

    m_text_socket = new QWebSocket();
    m_text_socket->setParent(this);
    connect( m_text_socket, &QWebSocket::connected                                               , this, &Client::textWebsocketConnected );             // 接続成功
    connect( m_text_socket, &QWebSocket::disconnected                                            , this, &Client::textWebsocketDisconnected );          // 接続切断
    connect( m_text_socket, &QWebSocket::textMessageReceived                                     , this, &Client::websocketTextMessageReceived );       // テキストメッセージ受信
    connect( m_text_socket, &QWebSocket::binaryMessageReceived                                   , this, &Client::websocketBinaryMessageReceived );     // バイナリメッセージ受信
    connect( m_text_socket, QOverload<QAbstractSocket::SocketError>::of( &QWebSocket::error )    , this, &Client::websocketError );                     // エラー
    m_text_socket->open( QUrl( textDddress ) );
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

void Client::onDebug()
{
    QJsonObject jsonMessage;
    jsonMessage["type"] = "curUsers";

    QJsonDocument doc(jsonMessage);
    QString message = doc.toJson(QJsonDocument::Compact);

    m_text_socket->sendTextMessage(message);
}

void Client::binaryWebsocketConnected()
{
    qInfo() << "Binary socket connected";

    // チャンネル情報をサーバに送信
    QJsonObject jsonObject;
    jsonObject["type"] = "join";
    jsonObject["uuid"] = m_user_uuid;
    jsonObject["channel"] = "binary";

    QJsonDocument doc(jsonObject);
    m_binary_socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

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

    // チャンネル情報をサーバに送信
    QJsonObject jsonObject;
    jsonObject["type"] = "join";
    jsonObject["uuid"] = m_user_uuid;
    jsonObject["channel"] = "text";

    QJsonDocument doc(jsonObject);
    m_text_socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

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
    qDebug() << "Received binary data size:" << binaryMessage.size() << "bytes";
    const char* data_ptr = binaryMessage.constData();
    size_t offset = 0;

    // 頂点数を読み出す
    size_t numberOfVertices = 0;
    std::memcpy( &numberOfVertices, data_ptr + offset, sizeof( size_t ) );
    offset += sizeof( size_t );

    // 座標（float3 * N）
    kvs::ValueArray<kvs::Real32> coords( numberOfVertices * 3 );
    std::memcpy( coords.data(), data_ptr + offset, sizeof( kvs::Real32 ) * 3 * numberOfVertices );
    offset += sizeof( kvs::Real32 ) * 3 * numberOfVertices;

    // 色（uchar3 * N）
    kvs::ValueArray<kvs::UInt8> colors( numberOfVertices * 3 );
    std::memcpy( colors.data(), data_ptr + offset, sizeof( kvs::UInt8 ) * 3 * numberOfVertices );
    offset += sizeof( kvs::UInt8 ) * 3 * numberOfVertices;

    // 法線（float3 * N）
    kvs::ValueArray<kvs::Real32> normals( numberOfVertices * 3 );
    std::memcpy( normals.data(), data_ptr + offset, sizeof( kvs::Real32 ) * 3 * numberOfVertices );
    offset += sizeof( kvs::Real32 ) * 3 * numberOfVertices;

    // minObjectCoords（float3）
    kvs::Vec3 minObjectCoords;
    std::memcpy( minObjectCoords.data(), data_ptr + offset, sizeof( kvs::Real32 ) * 3 );
    offset += sizeof( kvs::Real32 ) * 3;

    // maxObjectCoords（float3）
    kvs::Vec3 maxObjectCoords;
    std::memcpy( maxObjectCoords.data(), data_ptr + offset, sizeof( kvs::Real32 ) * 3 );
    offset += sizeof( kvs::Real32 ) * 3;

    // kvs::PointObject の生成
    auto* object = new kvs::PointObject();
    object->setCoords( coords );
    object->setColors( colors );
    object->setNormals( normals );
    object->setMinMaxObjectCoords( minObjectCoords, maxObjectCoords );
    object->setMinMaxExternalCoords( minObjectCoords, maxObjectCoords );

    object->setXform( m_screen->scene()->objectManager()->xform() );
    m_screen->scene()->objectManager()->push_centering_xform();
    m_screen->scene()->objectManager()->updateMinMaxCoords();
    m_screen->scene()->objectManager()->updateExternalCoords();
    m_screen->scene()->objectManager()->pop_centering_xform();

    if( m_server_point_object_ids == QPair<int,int>( -1, -1 ) )
    {
        registerObject( object );
    }
    else
    {
        replaceObject( object );
    }
}

void Client::websocketError( QAbstractSocket::SocketError error )
{
    qDebug() << "WebSocket error:" << error;
}
