#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "downloaddialog.h"

#include <QUdpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDragEnterEvent>
#include <QMimeData>

const qint64 BUFFER_SIZE = 4096;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	setAcceptDrops(true);

	ui->status->setAlignment(Qt::AlignCenter);
	ui->status->setText("Waiting...");
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(100);
	ui->progressBar->setValue(0);
	ui->upload->setDefault(true);
	socket = new QUdpSocket(this);
	//replyTimer = new QTimer(this);
	//replyTimer->setInterval(timerIntetval);

	connect(socket, &QUdpSocket::readyRead, this, &MainWindow::readyRead);
	//connect(replyTimer, &QTimer::timeout, this, &MainWindow::resendData);
	connect(ui->upload, &QPushButton::clicked, this, &MainWindow::upload);
	connect(ui->download, &QPushButton::clicked, this, &MainWindow::download);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::disconnected() {
	//if (0/*isDownloading*/) {
	//	QString fileName = fileInfo["Name"].toString();
	//	QFile file(clientFolder + "/" + fileName);

	//	if (file.exists()) {
	//		file.remove();
	//	}
	//}

	close();
}

void MainWindow::readyRead() {
	QByteArray data;
	data.resize(socket->pendingDatagramSize());

	QHostAddress sender;
	quint16 senderPort;

	socket->readDatagram(data.data(), data.size(),
		&sender, &senderPort);

	QDataStream stream(data);
	QHash<QString, QVariant> reply;
	stream >> reply;

	if (reply.contains(filesList)) {
		downloadFiles(reply[filesList].toStringList());
	} else if (reply.contains("Name") && reply.contains("Size")) {
		requestFirstPart(reply);
	} else if (reply.contains(getFileUpload)) {
		downloading(reply);
	} else if (reply.contains(getFile) || reply.contains(status)) {
		if (reply.contains(getFile)) {
			//timer->stop();
			fileTransfer(reply);
			//timer->start();
		} else if (reply[status] == fileDownloaded) {
			//timer->stop();
			ui->upload->setDisabled(false);
			ui->download->setDisabled(false);
			ui->status->setText("Waiting...");
		}
	}
}

void MainWindow::upload() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
		tr("All Files(*.*)"));
	
	if (fileName.isEmpty()) {
		return;
	}

	uploadFile(fileName);
}

void MainWindow::uploadFile(const QString& fileName) {
	filePath = fileName;
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox messageBox;
		messageBox.setText("Can't open file!!!");
		messageBox.setStandardButtons(QMessageBox::Ok);
		messageBox.setIcon(QMessageBox::Warning);
		messageBox.exec();

		return;
	}

	ui->upload->setDisabled(true);
	ui->download->setDisabled(true);

	QHash<QString, QVariant> fileInfo;
	fileInfo.insert("Name", QFileInfo(file).fileName());
	fileInfo.insert("Size", file.size());

	file.close();

	QByteArray replyByteArray;
	QDataStream stream(&replyByteArray, QIODevice::ReadWrite);

	stream << fileInfo;

	qint64 result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);

	if (result == -1) {
		showError();
	}

	//replyTimer->start();
}

void MainWindow::fileTransfer(QHash<QString, QVariant> reply) {
	QString fileName = reply[getFile].toString();
	qint64 blockNumber = reply[blockAddr].toLongLong();
	qint64 blocksTotal = reply[blockTotal].toLongLong();
	qint64 blockLength = reply[blockSize].toLongLong();

	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox messageBox;
		messageBox.setText("Can't open file!!!");
		messageBox.setStandardButtons(QMessageBox::Ok);
		messageBox.setIcon(QMessageBox::Warning);
		messageBox.exec();

		return;
	}

	if (fileName != QFileInfo(file).fileName()) {
		return;
	}

	QString textStatus = "Uploading file: " + fileName;
	ui->status->setText(textStatus);

	QByteArray data;
	file.skip(blockLength * blockNumber);
	data = file.read(blockLength);

	QHash<QString, QVariant> dataMap;
	dataMap.insert(getFile, fileName);
	dataMap.insert(blockAddr, blockNumber);
	dataMap.insert(blockTotal, blocksTotal);
	dataMap.insert(blockSize, blockLength);
	dataMap.insert(dataField, data);

	QByteArray replyByteArray;
	QDataStream stream(&replyByteArray, QIODevice::ReadWrite);
	stream << dataMap;
	qint64 result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);

	if (result == -1) {
		showError();
	}

	ui->progressBar->setValue((blockNumber + 1) * 100 / blocksTotal);

	file.close();
}

void MainWindow::download() {
	QHash<QString, QVariant> request;
	request.insert(getFilesList, 0);

	QByteArray replyByteArray;
	QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
	dataStream << request;
	qint64 result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);

	if (result == -1) {
		showError();
	}
}

void MainWindow::downloadFiles(QStringList& filesList) {
	DownloadDialog downloadDialog;
	downloadDialog.setFiles(filesList);

	if (downloadDialog.exec() != QDialog::Accepted) {
		return;
	}

	QString fileName = downloadDialog.getFile();

	if (fileName.isEmpty()) {
		return;
	}

	QHash<QString, QVariant> request;
	request.insert(getFileDownload, fileName);

	QByteArray replyByteArray;
	QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
	dataStream << request;
	qint64 result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);

	if (result == -1) {
		showError();
	}
}

void MainWindow::requestFirstPart(QHash<QString, QVariant> reply) {
	FileInfo downloadFileInfo;
	downloadFileInfo.fileName = reply["Name"].toString();
	downloadFileInfo.size = reply["Size"].toLongLong();
	downloadFileInfo.progress = 0;

	if (!QDir(clientFolder).exists()) {
		QDir().mkdir(clientFolder);
	}

	QFile fileVar(clientFolder + "/" + downloadFileInfo.fileName);

	if (!fileVar.open(QIODevice::ReadWrite)) {
		QMessageBox* messageBox = new QMessageBox(this);
		messageBox->setAttribute(Qt::WA_DeleteOnClose);
		messageBox->setIcon(QMessageBox::Warning);
		messageBox->setStandardButtons(QMessageBox::Ok);
		messageBox->setText("Can't open file: " + fileVar.fileName());
		messageBox->show();
		return;
	}

	QDataStream fileStream(&fileVar);
	qint64 fileBlockSize = 45000;
	qint64 fileBlockNum = downloadFileInfo.size / fileBlockSize;

	for (int i = 0; i < fileBlockNum; i++) {
		QByteArray data;
		data.append(fileBlockSize, 0);
		fileVar.write(data);
		fileVar.flush();
	}

	if (downloadFileInfo.size % fileBlockSize) {
		qint64 lastBlockSize = downloadFileInfo.size % fileBlockSize;
		QByteArray data;
		data.append(lastBlockSize, 0);
		fileVar.write(data);
		fileVar.flush();
	}

	fileVar.close();

	qint64 bufferBlocksNum = downloadFileInfo.size / BUFFER_SIZE;
	bufferBlocksNum = downloadFileInfo.size % BUFFER_SIZE ? bufferBlocksNum + 1 : bufferBlocksNum;

	QHash<QString, QVariant> request;
	request.insert(getFileUpload, downloadFileInfo.fileName);
	request.insert(blockAddr, 0);
	request.insert(blockTotal, bufferBlocksNum);
	request.insert(blockSize, BUFFER_SIZE);

	QByteArray replyByteArray;
	QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
	dataStream << request;
	qint64 result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);

	if (result == -1) {
		showError();
	}

	ui->download->setDisabled(true);
	ui->upload->setDisabled(true);
}

void MainWindow::downloading(QHash<QString, QVariant> reply) {
	QString fileName = reply[getFileUpload].toString();
	qint64 blockNumber = reply[blockAddr].toLongLong();
	qint64 blocksNum = reply[blockTotal].toLongLong();
	qint64 blockLength = reply[blockSize].toLongLong();

	QFile file(clientFolder + "/" + fileName);

	if (!file.open(QIODevice::ReadWrite)) {
		return;
	}

	file.skip(blockNumber * blockLength);
	file.write(reply[dataField].toByteArray());
	file.close();

	QString textStatus = "Downloading file: " + fileName;
	ui->status->setText(textStatus);

	ui->progressBar->setValue((blockNumber + 1) * 100 / blocksNum);

	qint64 result;

	if (blocksNum + 1 == blockNumber) {
		QHash<QString, QVariant> reply;
		reply.insert(fileDownloaded, fileName);

		QByteArray replyByteArray;
		QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
		dataStream << reply;
		
		result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);

		ui->upload->setDisabled(false);
		ui->download->setDisabled(false);
		ui->status->setText("Waiting...");
	} else {
		QHash<QString, QVariant> request;
		request.insert(getFileUpload, fileName);
		request.insert(blockAddr, blockNumber + 1);
		request.insert(blockTotal, reply[blockTotal].toLongLong());
		request.insert(blockSize, blockLength);

		QByteArray replyByteArray;
		QDataStream dataStream(&replyByteArray, QIODevice::ReadWrite);
		dataStream << request;
		
		result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);
	}

	if (result == -1) {
		showError();
	}
}

void MainWindow::resendData() {
	/*replyTimer->stop();

	while (socket->hasPendingDatagrams()) {
		readyRead();
		return;
	}

	QByteArray replyByteArray;
	QDataStream stream(&replyByteArray, QIODevice::ReadWrite);
	qint64 result;

	if (lastData.contains(lastBlock)) {
		stream << lastData[lastBlock].toByteArray();
		result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);
	} else {
		stream << lastData;
		result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);
	}

	if (result == -1) {
		showError();
	}

	replyTimer->start();*/
}

void MainWindow::showError() {
	QMessageBox* messageBox = new QMessageBox(this);
	messageBox->setAttribute(Qt::WA_DeleteOnClose);
	messageBox->setIcon(QMessageBox::Warning);
	messageBox->setStandardButtons(QMessageBox::Ok);
	messageBox->setText("Write datagram error: " + QString::number(socket->error()));
	messageBox->show();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	if (ui->upload->isEnabled() && event->mimeData()->urls().size() == 1) {
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent* event)
{
	for (const QUrl& url : event->mimeData()->urls()) {
		QString fileName = url.toLocalFile();
		uploadFile(fileName);
	}
}
