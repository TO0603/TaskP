#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebSocket>

#include <QJsonObject>
#include <QJsonDocument>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initialize();

    Ui::MainWindow *ui;
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
#endif // MAINWINDOW_H
