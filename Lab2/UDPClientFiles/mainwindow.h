#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>

class QUdpSocket;
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

private:
	void fileTransfer();

private slots:
	void disconnected();
	void readyRead();
	void upload();
	void download();
	void downloadFiles(QStringList& filesList);
	void downloading(QByteArray& data);
	void resendData();
	void showError();

private:
	Ui::MainWindow* ui;
	int port = 0;
	QString ip;
	QString clientFolder = "client_files_upd";
	QString status = "status";
	QString blockReady = "block_ready";
	QString lastBlock = "last_block";
	QString fileInfoWritten = "file_info_written";
	QUdpSocket* socket;
	QFile* file = nullptr;
	int speedUpdateBlocks = 300;
	QHash<QString, QVariant> fileInfo;
	QHash<QString, QVariant> lastData;
	QTime timerDownload;
	QTimer* timer = nullptr;
	QTimer* replyTimer = nullptr;
	int timerIntetval = 1000;
	int speedDownloadSum = 0;
	int downloadedBlocks = 0;
	int uploadedBlocks = 0;
	double speedSum = 0;
	qint64 numberOfBlocks = 0;
	bool isFileInit = false;
	bool isWaitingList = false;
	bool isWaitingHash = false;
	bool isDownloading = false;
};
#endif // MAINWINDOW_H
