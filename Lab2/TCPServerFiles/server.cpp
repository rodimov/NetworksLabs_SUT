#include "server.h"
#include "func.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QFile>
#include <QDir>

const qint64 BUFFER_SIZE = 307704;

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
	QTcpSocket* clientSocket = static_cast<QTcpSocket*>(sender());
	clients.removeOne(clientSocket);

	if (files.contains(clientSocket)) {
		QString fileName = files[clientSocket].fileName;
		QFile file("server_files/" + fileName);

		if (file.exists()) {
			file.remove();
		}

		files.remove(clientSocket);

		consoleOut() << "File: " << fileName << " removed!" << endl;
	}

	if (filesDownload.contains(clientSocket)) {
		filesDownload.remove(clientSocket);
		consoleOut() << "Client stop download: " << clientSocket->peerAddress().toString() << endl;
	}

	consoleOut() << "Client disconnected: " << clientSocket->peerAddress().toString() << endl;
}

void Server::readyRead() {
	QTcpSocket* clientSocket = static_cast<QTcpSocket*>(sender());
	QByteArray data = clientSocket->readAll();

	if (files.contains(clientSocket)) {
		FileInfo fileInfo = files[clientSocket];

		appendToFile(fileInfo, data);

		files[clientSocket] = fileInfo;

		if (fileInfo.size == fileInfo.progress) {
			consoleOut() << "File " << fileInfo.fileName << " dowloaded!" << endl;
			files.remove(clientSocket);
		}

		return;
	}

	QDataStream stream(data);

	QHash<QString, QVariant> request;
	stream >> request;

	if (request.contains("download")) {
		sendFilesList(clientSocket);
		return;
	} else if (request.contains("fileName")) {
		sendFileInfo(request["fileName"].toString(), clientSocket);
		return;
	} else if (request.contains("startDownload")) {
		uploadFile(clientSocket);
		return;
	}

	if (!request.contains("Name") || !request.contains("Size")) {
		return;
	}

	FileInfo file;
	file.fileName = request["Name"].toString();
	file.size = request["Size"].toLongLong();
	file.progress = 0;

	files.insert(clientSocket, file);

	QString result = "ok";
	QDataStream socketStream(clientSocket);

	socketStream << result;

	consoleOut() << "File " << file.fileName << " start downloading" << endl;
	consoleOut() << "Size: " << file.size / 1024 / 1024 << "mb" << endl;
}

void Server::appendToFile(FileInfo& fileInfo, QByteArray& data) {
	if (!QDir("server_files").exists()) {
		QDir().mkdir("server_files");
	}

	QFile file("server_files/" + fileInfo.fileName);

	if (!file.open(QIODevice::Append)) {
		return;
	}

	file.write(data);
	file.close();

	fileInfo.progress += data.size();

	consoleOut() << "File " << file.fileName() <<
		" download " << static_cast<int>(fileInfo.progress * 100.0 / fileInfo.size) << "%" << endl;
}

void Server::sendFilesList(QTcpSocket* tcpSocket) {
	QStringList filesList;
	QDir dir("server_files");
	QFileInfoList filesInfoList = dir.entryInfoList();

	for (const auto& fileInfo : filesInfoList) {
		if (!fileInfo.isDir()) {
			filesList.append(fileInfo.fileName());
		}
	}

	QDataStream socketStream(tcpSocket);
	socketStream << filesList;

	consoleOut() << "Send files list" << endl;
}

void Server::sendFileInfo(QString& fileName, QTcpSocket* tcpSocket) {
	QFile file("server_files/" + fileName);
	QFileInfo fileInfo(file);

	if (!file.exists()) {
		return;
	}

	QHash<QString, QVariant> fileHashMap;

	fileHashMap.insert("Name", fileInfo.fileName());
	fileHashMap.insert("Size", file.size());
	fileHashMap.insert("Progress", 0);

	QDataStream socketStream(tcpSocket);
	socketStream << fileHashMap;

	FileInfo fileParam;
	fileParam.fileName = fileHashMap["Name"].toString();
	fileParam.size = fileHashMap["Size"].toLongLong();
	fileParam.progress = fileHashMap["Progress"].toLongLong();

	filesDownload.insert(tcpSocket, fileParam);

	consoleOut() << "Send file info" << endl;
}

void Server::uploadFile(QTcpSocket* tcpSocket) {
	if (!filesDownload.contains(tcpSocket)) {
		return;
	}

	FileInfo fileInfo = filesDownload[tcpSocket];

	QFile file("server_files/" + fileInfo.fileName);

	if (!file.open(QIODevice::ReadOnly)) {
		filesDownload.remove(tcpSocket);
		return;
	}

	qint64 numberOfBlocks = fileInfo.size / BUFFER_SIZE;
	numberOfBlocks = fileInfo.size % BUFFER_SIZE != 0 ? numberOfBlocks + 1 : numberOfBlocks;
	double speedSum = 0;

	consoleOut() << "Uploading file: " + fileInfo.fileName << endl;

	for (qint64 i = 0; i < numberOfBlocks; i++) {

		QByteArray data;
		data = file.read(BUFFER_SIZE);

		tcpSocket->write(data);
		tcpSocket->waitForBytesWritten();

		consoleOut() << "Uploading file: " + fileInfo.fileName << " "
			<< QString::number((i + 1) * 100 / numberOfBlocks) << "%" << endl;
	}

	filesDownload.remove(tcpSocket);
}
