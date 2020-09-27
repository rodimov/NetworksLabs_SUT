#include "server.h"
#include "func.h"

#include <QUdpSocket>
#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QNetworkDatagram>

const qint64 BUFFER_SIZE = 4096;

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

void Server::disconnected(ClientAddress* clientAddress) {
	if (clientsForTimers.contains(clientAddress)) {
		QTimer* timer = clientsForTimers[clientAddress];
		timer->stop();
		timersForClients.remove(timer);
		clientsForTimers.remove(clientAddress);

		delete timer;
	}

	if (files.contains(clientAddress)) {
		QString fileName = files[clientAddress].fileName;
		QFile file(serverDir + "/" + fileName);

		if (file.exists()) {
			file.remove();
		}

		consoleOut() << "File: " << fileName << " removed!" << endl;
	}

	if (filesDownload.contains(clientAddress)) {
		consoleOut() << "Client stop download: " << clientAddress->ip.toString()
			+ ":" + QString::number(clientAddress->port) << endl;
	}

	files.remove(clientAddress);
	filesDownload.remove(clientAddress);
	filesBlock.remove(clientAddress);
	filesStatus.remove(clientAddress);

	consoleOut() << "Client disconnected: " << clientAddress->ip.toString()
		+ ":" + QString::number(clientAddress->port) << endl;

	delete clientAddress;
}

void Server::readyRead() {
	QByteArray data;
	data.resize(udpServer->pendingDatagramSize());

	QHostAddress sender;
	quint16 senderPort;

	udpServer->readDatagram(data.data(), data.size(),
		&sender, &senderPort);

	while (udpServer->hasPendingDatagrams()) {
		QNetworkDatagram datagram = udpServer->receiveDatagram();
		data += datagram.data();
	}

	udpServer->flush();

	ClientAddress clientAddress;
	clientAddress.ip = sender;
	clientAddress.port = senderPort;

	/*if (fileInfoData.contains(isContainsClient(clientAddress, files))
		&& (fileInfoData[isContainsClient(clientAddress, files)] == data)) {
		ClientAddress* client = isContainsClient(clientAddress, files);
		
		if (!client) {
			return;
		}
		
		QTimer* timer = clientsForTimers[client];
		timer->stop();
		sendReply(client, status, fileInfoWritten);
		timer->start();

		return;
	}*/

	if (isContainsClient(clientAddress, files)) {
		ClientAddress* client = isContainsClient(clientAddress, files);
		FileInfo fileInfo = files[client];
		QTimer* timer = clientsForTimers[client];
		timer->stop();

		appendToFile(fileInfo, data);

		files[client] = fileInfo;
		client->attemptsLeft = timerAttempts;
		sendReply(client, status, blockReady);
		filesStatus[client] = blockReady;

		if (fileInfo.size == fileInfo.progress) {
			consoleOut() << "File " << fileInfo.fileName << " dowloaded!" << endl;
			files.remove(client);
			disconnected(client);
		} else {
			timer->start();
		}

		while (udpServer->hasPendingDatagrams()) {
			QNetworkDatagram datagram = udpServer->receiveDatagram();
		}

		return;
	}

	QDataStream stream(data);
	QHash<QString, QVariant> request;
	stream >> request;

	/*if (request.contains("download")) {
		sendFilesList(clientSocket);
		return;
	} else if (request.contains("fileName")) {
		sendFileInfo(request["fileName"].toString(), clientSocket);
		return;
	} else if (request.contains("startDownload")) {
		uploadFile(clientSocket);
		return;
	}*/

	if (!request.contains("Name") || !request.contains("Size")) {
		return;
	}

	ClientAddress* client = new ClientAddress;
	client->ip = sender;
	client->port = senderPort;
	client->attemptsLeft = timerAttempts;

	FileInfo file;
	file.fileName = request["Name"].toString();
	file.size = request["Size"].toLongLong();
	file.progress = 0;

	fileInfoData.insert(client, data);

	files.insert(client, file);
	sendReply(client, status, fileInfoWritten);
	filesStatus.insert(client, fileInfoWritten);

	QTimer* timer = new QTimer(this);
	timer->setInterval(timerIntetval);
	timersForClients.insert(timer, client);
	clientsForTimers.insert(client, timer);
	connect(timer, &QTimer::timeout, this, &Server::resendData);
	timer->start();

	while (udpServer->hasPendingDatagrams()) {
		QNetworkDatagram datagram = udpServer->receiveDatagram();
	}

	consoleOut() << "File " << file.fileName << " start downloading" << endl;
	consoleOut() << "Size: " << file.size / 1024 / 1024 << "mb" << endl;
}

void Server::appendToFile(FileInfo& fileInfo, QByteArray& data) {
	if (!QDir(serverDir).exists()) {
		QDir().mkdir(serverDir);
	}

	QFile file(serverDir + "/" + fileInfo.fileName);

	if (!file.open(QIODevice::Append)) {
		return;
	}

	file.write(data);
	file.close();

	fileInfo.progress += data.size();

	consoleOut() << "File " << file.fileName() <<
		" download " << static_cast<int>(fileInfo.progress * 100.0 / fileInfo.size) << "%" << endl;
}

void Server::sendFilesList(ClientAddress* clientAddress) {
	QStringList filesList;
	QDir dir(serverDir);
	QFileInfoList filesInfoList = dir.entryInfoList();

	for (const auto& fileInfo : filesInfoList) {
		if (!fileInfo.isDir()) {
			filesList.append(fileInfo.fileName());
		}
	}

	//QDataStream socketStream(tcpSocket);
	//socketStream << filesList;

	consoleOut() << "Send files list" << endl;
}

void Server::sendFileInfo(QString& fileName, ClientAddress* clientAddress) {
	QFile file(serverDir + fileName);
	QFileInfo fileInfo(file);

	if (!file.exists()) {
		return;
	}

	QHash<QString, QVariant> fileHashMap;

	fileHashMap.insert("Name", fileInfo.fileName());
	fileHashMap.insert("Size", file.size());
	fileHashMap.insert("Progress", 0);

	//QDataStream socketStream(tcpSocket);
	//socketStream << fileHashMap;

	FileInfo fileParam;
	fileParam.fileName = fileHashMap["Name"].toString();
	fileParam.size = fileHashMap["Size"].toLongLong();
	fileParam.progress = fileHashMap["Progress"].toLongLong();

	//filesDownload.insert(tcpSocket, fileParam);

	consoleOut() << "Send file info" << endl;
}

void Server::uploadFile(ClientAddress* clientAddress) {
	/*if (!filesDownload.contains(tcpSocket)) {
		return;
	}

	FileInfo fileInfo = filesDownload[tcpSocket];

	QFile file(serverDir + fileInfo.fileName);

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

	filesDownload.remove(tcpSocket);*/
}

void Server::resendData() {
	QTimer* timer = static_cast<QTimer*>(sender());
	QByteArray replyByteArray;

	if (!timersForClients.contains(timer)) {
		delete timer;
		return;
	}

	ClientAddress* clientAddress = timersForClients[timer];

	if (files.contains(clientAddress)) {
		QByteArray data;
		data.resize(udpServer->pendingDatagramSize());

		udpServer->readDatagram(data.data(), data.size(), &clientAddress->ip, &clientAddress->port);

		udpServer->flush();

		if (data.size()) {
			appendToFile(files[clientAddress], data);

			return;
		}
	}

	if (!clientAddress->attemptsLeft) {
		disconnected(clientAddress);
		consoleOut() << "Attempts left: " << clientAddress->attemptsLeft << endl;

		return;
	}

	if (filesBlock.contains(clientAddress)) {
		qint64 result = udpServer->writeDatagram(filesBlock[clientAddress], clientAddress->ip, clientAddress->port);

		if (result == -1) {
			consoleOut() << "Write datagram error: " << udpServer->error() << endl;
		}
	}

	if (filesStatus.contains(clientAddress)) {
		sendReply(clientAddress, status, filesStatus[clientAddress]);
		consoleOut() << "Retry send status: " << filesStatus[clientAddress].toString() << endl;
	}

	clientAddress->attemptsLeft -= 1;

	consoleOut() << "Attempts left: " << clientAddress->attemptsLeft << endl;
}

template<typename T>
ClientAddress* Server::isContainsClient(ClientAddress clientAddress, T container) {
	QHostAddress ip = clientAddress.ip;
	qint64 port = clientAddress.port;

	for (auto client : container.keys()) {
		if (client->ip == ip && client->port == port) {
			return client;
		}
	}

	return nullptr;
}

void Server::sendReply(ClientAddress* clientAddress, QString message, QVariant parametr) {
	QHash<QString, QVariant> reply;
	reply.insert(message, parametr);

	QByteArray replyByteArray;
	QDataStream stream(&replyByteArray, QIODevice::ReadWrite);

	stream << reply;

	qint64 result = udpServer->writeDatagram(replyByteArray, clientAddress->ip, clientAddress->port);

	if (result == -1) {
		consoleOut() << "Write datagram error: " << udpServer->error() << endl;
	}
}
