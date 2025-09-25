#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QWebSocket>

#include <QJsonObject>
#include <QJsonDocument>

QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();

private:
    void initialize();
    void updateButtons();
    bool areSocketsConnected() const;

    Ui::Client *ui;
    QWebSocket* m_binary_socket = nullptr;
    QWebSocket* m_text_socket = nullptr;

private slots:
    void onConnect();
    void onDisconnect();
    void onRequest();
    void onChat();

private slots: // WebSocket
    void binaryWebsocketConnected();                                        // 接続
    void binaryWebsocketDisconnected();                                     // 切断
    void textWebsocketConnected();                                        // 接続
    void textWebsocketDisconnected();                                     // 切断
    void websocketTextMessageReceived( const QString& receivedMessage );    // 受信(テキスト)
    void websocketBinaryMessageReceived( const QByteArray& binary );        // 受信(バイナリ)
    void websocketError( QAbstractSocket::SocketError error );              // エラー
};
#endif // CLIENT_H
