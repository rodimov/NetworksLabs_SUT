#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>

class QTcpSocket;
class QFile;

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
	void connectToServer();

private:
	void fileTransfer();

private slots:
	void disconnected();
	void readyRead();
	void upload();
	void download();
	void downloadFiles(QStringList& filesList);
	void downloading(QByteArray& data);

private:
	Ui::MainWindow* ui;
	int port = 0;
	QString ip;
	QTcpSocket* clientSocket;
	QFile* file = nullptr;
	int speedUpdateBlocks = 300;
	QHash<QString, QVariant> fileInfo;
	QTime timerDownload;
	int speedDownloadSum = 0;
	int downloadedBlocks = 0;
	bool isWaitingList = false;
	bool isWaitingHash = false;
	bool isDownloading = false;
};
#endif // MAINWINDOW_H
