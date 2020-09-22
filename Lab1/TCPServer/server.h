#ifndef SERVER_H
#define SERVER_H

#include <QObject>

class QTcpServer;
class QTcpSocket;

class Server : public QObject
{
	Q_OBJECT

public:
	Server(int port, QObject* parent = nullptr);
	virtual ~Server();

private slots:
	void connected();
	void disconnected();
	void readyRead();

private:
	QTcpServer* tcpServer = nullptr;
	QList<QTcpSocket*> clients;
	int port = 3510;
};

#endif //SERVER_H
