#ifndef SERVER_H
#define SERVER_H

#include <QObject>

class QUdpSocket;

class Server : public QObject
{
	Q_OBJECT

public:
	Server(int port, QObject* parent = nullptr);
	virtual ~Server();

private slots:
	void readyRead();

private:
	QUdpSocket* udpServer = nullptr;
	int port = 0;
};

#endif //SERVER_H
