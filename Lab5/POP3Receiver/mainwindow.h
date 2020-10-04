#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>

class QSslSocket;
class QFile;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum SmtpError
{
	ConnectionTimeoutError,
	ResponseTimeoutError,
	SendDataTimeoutError,
	AuthenticationFailedError,
	SendMailError,
	NoopError,
	ServerError,    // 4xx smtp error
	ClientError     // 5xx smtp error
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	void setPort(int port) { this->port = port; }
	int getPort() { return port; }
	void setIP(QString ip) { this->ip = ip; }
	QString getIP() { return ip; }
	void setUsername(QString username) { this->username = username; }
	QString getUsername() { return username; }
	void setPassword(QString password) { this->password = password; }
	QString getPassword() { return password; }

protected:
	void closeEvent(QCloseEvent* event);

private:
	bool waitForResponse();
	void showError(SmtpError error);
	void showMessageBox(const QString& text, QMessageBox::Icon icon, bool isExec = false);
	bool sendMessage(const QString& text);
	void login();
	void openWebPage(QString& url);
	void setSocketConnectState(bool isConnected);
	QString createMIME();
	QString toBase64(const QString& text);

public slots:
	void connectToServer();

private slots:
	void disconnected();
	void readyRead();
	void sendMail();
	void attach();
	void sendNoop();

private:
	Ui::MainWindow* ui;
	int port = 0;
	QString ip;
	QString username;
	QString password;
	QSslSocket* socket;
	QString responseText;
	QString okText = "+OK";
	QList<QFile*> files;
	QTimer* timer;
	bool isResponseOk;
	int sendMessageTimeout = 30000;
	int responseTimeout = 5000;
	int maxFileSize = 10485760;
	int sendNoopInterval = 10000;
};
#endif // MAINWINDOW_H
