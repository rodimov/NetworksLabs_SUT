#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QUdpSocket;

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

private slots:
	void readyRead();
	void send();
	void clear();

private:
	Ui::MainWindow* ui;
	int port = 0;
	QString ip;
	QUdpSocket* clientSocket;
};
#endif // MAINWINDOW_H
