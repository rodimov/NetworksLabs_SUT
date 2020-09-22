#include "server.h"
#include "func.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>

Server::Server(int port, QObject* parent) : QObject(parent) {
	this->port = port;
	tcpServer = new QTcpServer(this);

	connect(tcpServer, &QTcpServer::newConnection, this, &Server::connected);

	if (!tcpServer->listen(QHostAddress::Any, port)) {
		tcpServer->close();
		throw tcpServer->errorString();
	}

	consoleOut() << "Server started at port: " << port << endl;
}

Server::~Server() {
	for (auto& client : clients) {
		client->disconnect();
	}

	tcpServer->close();
}

void Server::connected() {
	QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
	clients.push_back(clientSocket);

	connect(clientSocket, &QTcpSocket::disconnected, this, &Server::disconnected);
	connect(clientSocket, &QTcpSocket::readyRead, this, &Server::readyRead);

	consoleOut() << "Client connected: " << clientSocket->peerAddress().toString() << endl;
}

void Server::disconnected() {
	QTcpSocket* clientSocket = static_cast<QTcpSocket*>(QObject::sender());
	clients.removeOne(clientSocket);

	consoleOut() << "Client disconnected: " << clientSocket->peerAddress().toString() << endl;
}

void Server::readyRead() {
	QTcpSocket* ñlientSocket = static_cast<QTcpSocket*>(QObject::sender());
	QDataStream stream(ñlientSocket);
	QString message;

	stream >> message;

	consoleOut() << "Client message: " << message << endl;

	stream << message;
}
