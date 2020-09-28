#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTcpSocket;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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

private:
	void connectToServer();

private slots:
	void disconnected();
	void readyRead();

private:
	Ui::MainWindow* ui;
	int port = 0;
	QString ip;
	QString username;
	QString password;
	QTcpSocket* socket;
	
};
#endif // MAINWINDOW_H
