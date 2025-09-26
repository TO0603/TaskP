#include "Client.h"
#include "ui_Client.h"

Client::Client( kvs::qt::Application& app, QWidget *parent )
    : QMainWindow(parent)
    , ui(new Ui::Client)
    , m_screen( new kvs::qt::Screen( &app ) )
    , m_compositor( new kvs::StochasticRenderingCompositor( m_screen->scene() ) )
    , m_web_socket(nullptr)
{
    initialize();
}

Client::~Client()
{    
    delete m_compositor;
    delete m_screen;
    if( m_web_socket )
    {
        m_web_socket->deleteLater();
    }
    delete ui;
}

#include <kvs/HydrogenVolumeData>
#include <kvs/TransferFunction>
#include <kvs/CellByCellMetropolisSampling>
#include <kvs/ParticleBasedRenderer>
void Client::initialize()
{
    ui->setupUi(this);

    m_compositor->setRepetitionLevel( 4 ); // コンポジターのリピートレベルを設定 初期値:4
    m_screen->setEvent( m_compositor );

    m_screen->setFixedSize( 620, 620 );
    ui->screenArea->addWidget( m_screen );

    connect( ui->connectPushButton    , &QPushButton::clicked, this, &Client::onConnect );      // 接続
    connect( ui->disconnectPushButton , &QPushButton::clicked, this, &Client::onDisconnect );   // 切断
    connect( ui->requestPushButton    , &QPushButton::clicked, this, &Client::onRequest );      // 要求(テスト)
    connect( ui->chatPushButton       , &QPushButton::clicked, this, &Client::onChat );         // チャットメッセージ送信
    this->show();

    {
        auto* volume = new kvs::HydrogenVolumeData( { 32, 32, 32 } );
        const auto repeat = 4; // number of repetitions
        const auto step = 0.5f; // sampling step
        const auto tfunc = kvs::TransferFunction( 256 ); // transfer function
        auto* object = new kvs::CellByCellMetropolisSampling( volume, repeat, step, tfunc );
        delete volume;
        auto* renderer = new kvs::glsl::ParticleBasedRenderer();
        m_screen->registerObject( object, renderer );
    }
}

void Client::onConnect()
{
    const QString address = ui->addressLineEdit->text().trimmed();
    if( address.isEmpty() ) // アドレスが空である場合は何もしない
    {
        return;
    }

    m_web_socket = new QWebSocket();

    connect( m_web_socket, &QWebSocket::connected                                               , this, &Client::websocketConnected );               // 接続成功
    connect( m_web_socket, &QWebSocket::disconnected                                            , this, &Client::websocketDisconnected );            // 接続切断
    connect( m_web_socket, &QWebSocket::textMessageReceived                                     , this, &Client::websocketTextMessageReceived );     // テキストメッセージ受信
    connect( m_web_socket, &QWebSocket::binaryMessageReceived                                   , this, &Client::websocketBinaryMessageReceived );   // バイナリメッセージ受信
    connect( m_web_socket, QOverload<QAbstractSocket::SocketError>::of( &QWebSocket::error )    , this, &Client::websocketError );                   // エラー

    m_web_socket->open( QUrl( address ) );
}

void Client::onDisconnect()
{
    if( m_web_socket )
    {
        if( m_web_socket->isValid() )
        {
            m_web_socket->close(); // ソケットを明示的に閉じる
        }

        m_web_socket->deleteLater(); // メモリ解放は安全なタイミングで
        m_web_socket = nullptr;
    }
}

void Client::onRequest()
{
    if( !m_web_socket || m_web_socket->state() != QAbstractSocket::ConnectedState ) return;

    // JSON形式のメッセージを作成
    QJsonObject jsonMessage;
    jsonMessage["type"] = QString::fromUtf8( "request");

    QJsonDocument doc( jsonMessage );
    QString message = doc.toJson( QJsonDocument::Compact );

    m_web_socket->sendTextMessage( message );
}

void Client::onChat()
{
    if( !m_web_socket || m_web_socket->state() != QAbstractSocket::ConnectedState ) return; // 接続されていなければ送信しない

    QString text = ui->chatLineEdit->text().trimmed();
    if( text.isEmpty() ) return; // 何も入力されていない場合は何もしない

    QJsonObject jsonMessage;
    jsonMessage["type"] = QString::fromUtf8( "chat" );
    jsonMessage[ QString::fromUtf8( "chat_message" ) ]   = text;

    QJsonDocument doc( jsonMessage );
    QString message = doc.toJson( QJsonDocument::Compact );

    m_web_socket->sendTextMessage( message );
    ui->chatLineEdit->clear();
}

void Client::websocketConnected()    // 接続成功
{
    QString log = "Connection successful : " + m_web_socket->requestUrl().toString();

    ui->connectPushButton->setEnabled( false );
    ui->disconnectPushButton->setEnabled( true );
}

void Client::websocketDisconnected() // 接続切断
{
    ui->connectPushButton->setEnabled( true );
    ui->disconnectPushButton->setEnabled( false );
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
