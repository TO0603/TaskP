#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>

#include <kvs/qt/Application>
#include <kvs/qt/Screen>
#include <kvs/StochasticRenderingCompositor>

QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client( kvs::qt::Application& app, QWidget *parent = nullptr );
    ~Client();

private:
    void initialize();

    Ui::Client *ui;
    kvs::qt::Screen* m_screen                           = nullptr;
    kvs::StochasticRenderingCompositor* m_compositor    = nullptr;
    QWebSocket* m_web_socket = nullptr;

private slots:
    void onConnect();
    void onDisconnect();
    void onRequest();
    void onChat();

private slots: // WebSocket
    void websocketConnected();                                              // 接続
    void websocketDisconnected();                                           // 切断
    void websocketTextMessageReceived( const QString& receivedMessage );    // 受信(テキスト)
    void websocketBinaryMessageReceived( const QByteArray& binary );        // 受信(バイナリ)
    void websocketError( QAbstractSocket::SocketError error );              // エラー
};
#endif // CLIENT_H
