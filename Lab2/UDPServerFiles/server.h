#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QHash>
#include <QHostAddress>

class QUdpSocket;
class QTimer;

struct FileInfo {
	QString fileName;
	qint64 size;
	qint64 progress;
};

struct ClientAddress {
	QHostAddress ip;
	quint16 port;
	int attemptsLeft = 10;
};

class Server : public QObject {
	Q_OBJECT

public:
	Server(int port, QObject* parent = nullptr);
	virtual ~Server();

private slots:
	void disconnected(ClientAddress* clientAddress);
	void readyRead();
	void appendToFile(FileInfo& fileInfo, QByteArray& data);
	void sendFilesList(ClientAddress* clientAddress);
	void sendFileInfo(QString& fileName, ClientAddress* clientAddress);
	void uploadFile(ClientAddress* clientAddress);
	void resendData();

private:
	template<typename T>
	ClientAddress* isContainsClient(ClientAddress clientAddress, T container);
	
	void sendReply(ClientAddress* clientAddress, QString message, QVariant parametr);

	QUdpSocket* udpServer = nullptr;
	int port = 3510;
	int timerIntetval = 10000;
	int timerAttempts = 10;
	QString serverDir = "server_files_udp";
	QString status = "status";
	QString blockReady = "block_ready";
	QString fileInfoWritten = "file_info_written";
	QHash<ClientAddress*, FileInfo> files;
	QHash<ClientAddress*, FileInfo> filesDownload;
	QHash<ClientAddress*, QByteArray> filesBlock;
	QHash<ClientAddress*, QVariant> filesStatus;
	QHash<QTimer*, ClientAddress*> timersForClients;
	QHash<ClientAddress*, QTimer*> clientsForTimers;
	QHash<ClientAddress*, QByteArray> fileInfoData;
};

#endif //SERVER_H
