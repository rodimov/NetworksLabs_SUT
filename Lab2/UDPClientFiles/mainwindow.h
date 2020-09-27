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
	void fileTransfer(QHash<QString, QVariant> reply);

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
	QString getFile = "get_file";
	QString blockAddr = "block_addr";
	QString blockTotal = "block_total";
	QString blockSize = "block_size";
	QString fileDownloaded = "fileDownloaded";
	QString dataField = "data";
	QString blockReady = "block_ready";
	QString lastBlock = "last_block";
	QString fileInfoWritten = "file_info_written";
	QString filePath;
	QUdpSocket* socket;
	int speedUpdateBlocks = 300;
	QHash<QString, QVariant> fileInfo;
	QHash<QString, QVariant> lastData;
	QTime timerDownload;
	QTimer* timer = nullptr;
	QTimer* replyTimer = nullptr;
	int timerIntetval = 100;
	int speedDownloadSum = 0;
	int downloadedBlocks = 0;
	bool isWaitingList = false;
	bool isWaitingHash = false;
	bool isDownloading = false;
};
#endif // MAINWINDOW_H
