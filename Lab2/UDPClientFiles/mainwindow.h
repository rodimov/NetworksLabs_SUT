#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <QVariant>

class QUdpSocket;
class QFile;

struct FileInfo {
	QString fileName;
	qint64 size;
	qint64 progress;
};

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
	void uploadFile(const QString& fileName);

protected:
	virtual void dragEnterEvent(QDragEnterEvent* event) override;
	virtual void dropEvent(QDropEvent* event) override;

private:
	void fileTransfer(const QHash<QString, QVariant>& reply);

private slots:
	void disconnected();
	void readyRead();
	void upload();
	void download();
	void downloadFiles(const QStringList& filesList);
	void downloading(const QHash<QString, QVariant>& reply);
	void resendData();
	void showError();
	void requestFirstPart(const QHash<QString, QVariant>& reply);

private:
	Ui::MainWindow* ui;
	int port = 0;
	QString ip;
	QString clientFolder = "client_files_upd";
	QString status = "status";
	QString getFile = "get_file";
	QString getFileUpload = "get_file_upload";
	QString blockAddr = "block_addr";
	QString blockTotal = "block_total";
	QString blockSize = "block_size";
	QString fileDownloaded = "fileDownloaded";
	QString dataField = "data";
	QString blockReady = "block_ready";
	QString filesList = "files_list";
	QString getFilesList = "get_files_list";
	QString getFileDownload = "get_file_download";
	QString fileInfoWritten = "file_info_written";
	QString filePath;
	QUdpSocket* socket;
	int speedUpdateBlocks = 300;
	QTime timerDownload;
	QTimer* timer = nullptr;
	QTimer* replyTimer = nullptr;
	int timerIntetval = 100;
	int speedDownloadSum = 0;
	int downloadedBlocks = 0;
};
#endif // MAINWINDOW_H
