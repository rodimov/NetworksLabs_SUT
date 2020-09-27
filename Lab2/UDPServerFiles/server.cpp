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
		//QTimer* timer = clientsForTimers[client];
		//timer->stop();

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
		}

		return;
	}

	QDataStream stream(data);
	QHash<QString, QVariant> request;
	stream >> request;

	if (request.contains(getFilesList)) {
		sendFilesList(&clientAddress);
		return;
	} else if (request.contains(getFileDownload)) {
		sendFileInfo(request[getFileDownload].toString(), &clientAddress);
		return;
	} else if (request.contains(getFileUpload)) {
		uploadFile(isContainsClient(clientAddress, filesDownload), request);
		return;
	} else if (request.contains(fileDownloaded)) {
		consoleOut() << "File " << request[fileDownloaded].toString() << " dowloaded!" << endl;
		ClientAddress* client = isContainsClient(clientAddress, filesDownload);
		filesDownload.remove(client);
		disconnected(client);
		return;
	}

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
		fileVar.write(data);
		fileVar.flush();
	}

	if (file.size % fileBlockSize) {
		qint64 lastBlockSize = file.size % fileBlockSize;
		QByteArray data;
		data.append(lastBlockSize, 0);
		fileVar.write(data);
		fileVar.flush();
	}

	fileVar.close();

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

	if (result == -1) {
		consoleOut() << "Write datagram error: " << udpServer->error() << endl;
	}

	/*QTimer* timer = new QTimer(this);
	timer->setInterval(timerIntetval);
	timersForClients.insert(timer, client);
	clientsForTimers.insert(client, timer);
	connect(timer, &QTimer::timeout, this, &Server::resendData);
	timer->start();*/

	dataForResend.insert(client, reply);

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

	if (result == -1) {
		consoleOut() << "Write datagram error: " << udpServer->error() << endl;
	}

	consoleOut() << "File " << file.fileName() <<
		" download " << static_cast<int>(fileInfo.progress * 100.0 / fileInfo.size) << "%" << endl;
}

void Server::sendFilesList(ClientAddress* clientAddress) {
	QStringList filesStringList;
	QDir dir(serverDir);
	QFileInfoList filesInfoList = dir.entryInfoList();

	for (const auto& fileInfo : filesInfoList) {
		if (!fileInfo.isDir()) {
			filesStringList.append(fileInfo.fileName());
		}
	}

	QHash<QString, QVariant> reply;
	reply.insert(filesList, filesStringList);

	QByteArray replyByteArray;
	QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
	dataStream << reply;
	qint64 result = udpServer->writeDatagram(replyByteArray, clientAddress->ip, clientAddress->port);

	if (result == -1) {
		consoleOut() << "Write datagram error: " << udpServer->error() << endl;
	}

	consoleOut() << "Send files list" << endl;
}

void Server::sendFileInfo(QString& fileName, ClientAddress* clientAddress) {
	QFile file(serverDir + "/" + fileName);
	QFileInfo fileInfo(file);

	if (!file.exists()) {
		return;
	}

	QHash<QString, QVariant> fileHashMap;

	fileHashMap.insert("Name", fileInfo.fileName());
	fileHashMap.insert("Size", file.size());

	QByteArray replyByteArray;
	QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
	dataStream << fileHashMap;
	qint64 result = udpServer->writeDatagram(replyByteArray, clientAddress->ip, clientAddress->port);

	if (result == -1) {
		consoleOut() << "Write datagram error: " << udpServer->error() << endl;
	}

	FileInfo fileParam;
	fileParam.fileName = fileHashMap["Name"].toString();
	fileParam.size = fileHashMap["Size"].toLongLong();
	fileParam.progress = 0;

	ClientAddress* client = new ClientAddress;
	client->attemptsLeft = timerAttempts;
	client->ip = clientAddress->ip;
	client->port = clientAddress->port;

	filesDownload.insert(client, fileParam);

	consoleOut() << "Send file info" << endl;
}

void Server::uploadFile(ClientAddress* clientAddress, QHash<QString, QVariant> reply) {
	if (!clientAddress) {
		return;
	}

	QString fileName = reply[getFileUpload].toString();
	qint64 blockNumber = reply[blockAddr].toLongLong();
	qint64 blocksTotal = reply[blockTotal].toLongLong();
	qint64 blockLength = reply[blockSize].toLongLong();

	QFile file(serverDir + "/" + fileName);

	if (!file.open(QIODevice::ReadOnly)) {
		consoleOut() << "Can't create file" << endl;
		return;
	}

	QByteArray data;
	file.skip(blockLength * blockNumber);
	data = file.read(blockLength);

	QHash<QString, QVariant> dataMap;
	dataMap.insert(getFileUpload, fileName);
	dataMap.insert(blockAddr, blockNumber);
	dataMap.insert(blockTotal, blocksTotal);
	dataMap.insert(blockSize, blockLength);
	dataMap.insert(dataField, data);

	QByteArray replyByteArray;
	QDataStream stream(&replyByteArray, QIODevice::ReadWrite);
	stream << dataMap;
	qint64 result = udpServer->writeDatagram(replyByteArray, clientAddress->ip, clientAddress->port);

	if (result == -1) {
		consoleOut() << "Write datagram error: " << udpServer->error() << endl;
	}

	qint64 progress = (blockNumber + 1) * 100 / blocksTotal;
	consoleOut() << "Uploading file: " << fileName << " " << progress << "%" << endl;

	file.close();
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
