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

	ClientAddress clientAddress;
	clientAddress.ip = sender;
	clientAddress.port = senderPort;

	if (isContainsClient(clientAddress, files)) {
		ClientAddress* client = isContainsClient(clientAddress, files);
		FileInfo fileInfo = files[client];
		QTimer* timer = clientsForTimers[client];
		timer->stop();

		appendToFile(fileInfo, data, client);

		files[client] = fileInfo;
		client->attemptsLeft = timerAttempts;

		if (fileInfo.size == fileInfo.progress) {
			consoleOut() << "File " << fileInfo.fileName << " dowloaded!" << endl;

			QHash<QString, QVariant> reply;
			reply.insert(status, fileDownloaded);

			QByteArray replyByteArray;
			QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
			dataStream << reply;
			qint64 result = udpServer->writeDatagram(replyByteArray, client->ip, client->port);

			files.remove(client);
			disconnected(client);
		} else {
			timer->start();
		}

		while (udpServer->hasPendingDatagrams()) {
			udpServer->receiveDatagram();
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

	if (!QDir(serverDir).exists()) {
		QDir().mkdir(serverDir);
	}

	QFile fileVar(serverDir + "/" + file.fileName);

	if (!fileVar.open(QIODevice::ReadWrite)) {
		consoleOut() << "Can't create file" << endl;
		return;
	}

	QDataStream fileStream(&fileVar);
	qint64 fileBlockSize = 45000;
	qint64 fileBlockNum = file.size / fileBlockSize;

	for (int i = 0; i < fileBlockNum; i++) {
		QByteArray data;
		data.append(fileBlockSize, 0);
		fileStream << data;
	}

	if (file.size % fileBlockSize) {
		qint64 lastBlockSize = file.size % fileBlockSize;
		QByteArray data;
		data.append(lastBlockSize, 0);
		fileStream << data;
	}

	fileVar.close();

	fileInfoData.insert(client, data);

	files.insert(client, file);

	qint64 bufferBlocksNum = file.size / BUFFER_SIZE;
	bufferBlocksNum = file.size % BUFFER_SIZE ? bufferBlocksNum + 1 : bufferBlocksNum;

	QHash<QString, QVariant> reply;
	reply.insert(getFile, file.fileName);
	reply.insert(blockAddr, 0);
	reply.insert(blockTotal, bufferBlocksNum);
	reply.insert(blockSize, BUFFER_SIZE);

	QByteArray replyByteArray;
	QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
	dataStream << reply;
	qint64 result = udpServer->writeDatagram(replyByteArray, client->ip, client->port);

	QTimer* timer = new QTimer(this);
	timer->setInterval(timerIntetval);
	timersForClients.insert(timer, client);
	clientsForTimers.insert(client, timer);
	connect(timer, &QTimer::timeout, this, &Server::resendData);
	timer->start();

	dataForResend.insert(client, reply);

	while (udpServer->hasPendingDatagrams()) {
		udpServer->receiveDatagram();
	}

	consoleOut() << "File " << file.fileName << " start downloading" << endl;
	consoleOut() << "Size: " << file.size / 1024 / 1024 << "mb" << endl;
}

void Server::appendToFile(FileInfo& fileInfo, QByteArray& data, ClientAddress* clientAddress) {
	QDataStream stream(data);
	QHash<QString, QVariant> reply;
	stream >> reply;

	qint64 blockNumber = reply[blockAddr].toLongLong();
	qint64 blockLength = reply[blockSize].toLongLong();

	QFile file(serverDir + "/" + fileInfo.fileName);

	if (!file.open(QIODevice::ReadWrite)) {
		return;
	}

	file.skip(blockNumber * blockLength);
	file.write(reply[dataField].toByteArray());
	file.close();

	fileInfo.progress = blockNumber * blockLength + reply[dataField].toByteArray().size();

	QHash<QString, QVariant> request;
	request.insert(getFile, fileInfo.fileName);
	request.insert(blockAddr, blockNumber + 1);
	request.insert(blockTotal, reply[blockTotal].toLongLong());
	request.insert(blockSize, blockLength);

	QByteArray replyByteArray;
	QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
	dataStream << request;
	qint64 result = udpServer->writeDatagram(replyByteArray, clientAddress->ip, clientAddress->port);

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

	if (!timersForClients.contains(timer)) {
		delete timer;
		return;
	}

	ClientAddress* clientAddress = timersForClients[timer];

	if (files.contains(clientAddress)) {
		QByteArray data;
		data.resize(udpServer->pendingDatagramSize());

		udpServer->readDatagram(data.data(), data.size(), &clientAddress->ip, &clientAddress->port);

		if (data.size()) {
			appendToFile(files[clientAddress], data, clientAddress);
			return;
		}
	}

	if (!clientAddress->attemptsLeft) {
		disconnected(clientAddress);
		consoleOut() << "Attempts left: " << clientAddress->attemptsLeft << endl;

		return;
	}

	if (dataForResend.contains(clientAddress)) {
		QHash<QString, QVariant> resendHash = dataForResend[clientAddress];

		QByteArray replyByteArray;
		QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
		dataStream << resendHash;

		qint64 result = udpServer->writeDatagram(replyByteArray, clientAddress->ip, clientAddress->port);

		if (result == -1) {
			consoleOut() << "Write datagram error: " << udpServer->error() << endl;
		}
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
