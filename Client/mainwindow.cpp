#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_web_socket(nullptr)
{
    ui->setupUi(this);
    initialize();
}

MainWindow::~MainWindow()
{
    delete ui;
    if( m_web_socket )
    {
        m_web_socket->deleteLater();
    }
}

void MainWindow::initialize()
{
    connect( ui->connectPushButton    , &QPushButton::clicked, this, &MainWindow::onConnect );      // 接続
    connect( ui->disconnectPushButton , &QPushButton::clicked, this, &MainWindow::onDisconnect );   // 切断
    connect( ui->requestPushButton    , &QPushButton::clicked, this, &MainWindow::onRequest );      // 要求(テスト)
}

void MainWindow::onConnect()
{
    const QString address = ui->addressLineEdit->text().trimmed();
    if( address.isEmpty() ) // アドレスが空である場合は何もしない
    {
        return;
    }

    m_web_socket = new QWebSocket();

    connect( m_web_socket, &QWebSocket::connected                                               , this, &MainWindow::websocketConnected );               // 接続成功
    connect( m_web_socket, &QWebSocket::disconnected                                            , this, &MainWindow::websocketDisconnected );            // 接続切断
    connect( m_web_socket, &QWebSocket::textMessageReceived                                     , this, &MainWindow::websocketTextMessageReceived );     // テキストメッセージ受信
    connect( m_web_socket, &QWebSocket::binaryMessageReceived                                   , this, &MainWindow::websocketBinaryMessageReceived );   // バイナリメッセージ受信
    connect( m_web_socket, QOverload<QAbstractSocket::SocketError>::of( &QWebSocket::error )    , this, &MainWindow::websocketError );                   // エラー

    m_web_socket->open( QUrl( address ) );
}

void MainWindow::onDisconnect()
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

void MainWindow::onRequest()
{
    if( !m_web_socket || m_web_socket->state() != QAbstractSocket::ConnectedState ) return;

    // JSON形式のメッセージを作成
    QJsonObject jsonMessage;
    jsonMessage["type"] = QString::fromUtf8( "request");

    QJsonDocument doc( jsonMessage );
    QString message = doc.toJson( QJsonDocument::Compact );

    m_web_socket->sendTextMessage( message );
}

void MainWindow::websocketConnected()    // 接続成功
{
    QString log = "Connection successful : " + m_web_socket->requestUrl().toString();

    ui->connectPushButton->setEnabled( false );
    ui->disconnectPushButton->setEnabled( true );
}

void MainWindow::websocketDisconnected() // 接続切断
{
    ui->connectPushButton->setEnabled( true );
    ui->disconnectPushButton->setEnabled( false );
}

void MainWindow::websocketTextMessageReceived( const QString& textMessage ) // テキストメッセージ受信
{
    qDebug() << __func__;
}

#include <QFile>
void MainWindow::websocketBinaryMessageReceived(const QByteArray& binaryMessage)
{
    // ヘッダサイズ
    if (binaryMessage.size() < (int)(sizeof(uint32_t) + sizeof(uint64_t))) {
        qWarning() << "Invalid binary message received, too small.";
        return;
    }

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(binaryMessage.constData());
    uint32_t messageId;
    uint32_t chunkIndex;
    uint64_t payloadSize;

    std::memcpy(&messageId, ptr, sizeof(messageId));
    std::memcpy(&chunkIndex, ptr + sizeof(messageId), sizeof(chunkIndex));
    std::memcpy(&payloadSize, ptr + sizeof(messageId) + sizeof(chunkIndex), sizeof(payloadSize));

    qDebug() << "Received message ID:" << messageId
             << "Chunk index:" << chunkIndex
             << "Payload size:" << payloadSize
             << "Actual received size:" << binaryMessage.size() - sizeof(messageId) - sizeof(chunkIndex) - sizeof(payloadSize);

}





void MainWindow::websocketError( QAbstractSocket::SocketError error )
{
    qDebug() << "WebSocket error:" << error;
}
