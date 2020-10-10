#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QHash>

class QTcpServer;
class QTcpSocket;

struct FileInfo {
	QString fileName;
	qint64 size;
	qint64 progress;
};

class Server : public QObject {
	Q_OBJECT

public:
	Server(int port, QObject* parent = nullptr);
	virtual ~Server();

private slots:
	void connected();
	void disconnected();
	void readyRead();
	void appendToFile(FileInfo& fileInfo, const QByteArray& data);
	void sendFilesList(QTcpSocket* tcpSocket);
	void sendFileInfo(const QString& fileName, QTcpSocket* tcpSocket);
	void uploadFile(QTcpSocket* tcpSocket);

private:
	QTcpServer* tcpServer = nullptr;
	QList<QTcpSocket*> clients;
	int port = 3510;
	QHash<QTcpSocket*, FileInfo> files;
	QHash<QTcpSocket*, FileInfo> filesDownload;
};

#endif //SERVER_H
