#include "server.h"
#include "func.h"

#include <QDataStream>
#include <QUdpSocket>

Server::Server(int port, QObject* parent) : QObject(parent) {
	this->port = port;
	udpServer = new QUdpSocket(this);

	connect(udpServer, &QUdpSocket::readyRead, this, &Server::readyRead);

	if (!udpServer->bind(QHostAddress::Any, port)) {
		udpServer->close();
		throw udpServer->errorString();
	}

	consoleOut() << "Server started at port: " << port << endl;
}

Server::~Server() {
	
}

void Server::readyRead() {
	QByteArray buffer;
	buffer.resize(udpServer->pendingDatagramSize());

	QHostAddress sender;
	quint16 senderPort;

	udpServer->readDatagram(buffer.data(), buffer.size(),
		&sender, &senderPort);

	QString message(buffer);

	consoleOut() << "Client " + sender.toString() + ":" + QString::number(senderPort) + " message: " << message << endl;

	udpServer->writeDatagram(QByteArray().append(message), sender, senderPort);
}
