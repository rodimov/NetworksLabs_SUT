#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "downloaddialog.h"

#include <QUdpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QNetworkDatagram>

const qint64 BUFFER_SIZE = 4096;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->status->setAlignment(Qt::AlignCenter);
	ui->status->setText("Waiting...");
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(100);
	ui->progressBar->setValue(0);
	ui->upload->setDefault(true);
	socket = new QUdpSocket(this);
	timer = new QTimer(this);
	replyTimer = new QTimer(this);
	replyTimer->setInterval(timerIntetval);

	connect(socket, &QUdpSocket::readyRead, this, &MainWindow::readyRead);
	connect(timer, &QTimer::timeout, this, &MainWindow::readyRead);
	connect(replyTimer, &QTimer::timeout, this, &MainWindow::resendData);
	connect(ui->upload, &QPushButton::clicked, this, &MainWindow::upload);
	connect(ui->download, &QPushButton::clicked, this, &MainWindow::download);
}

MainWindow::~MainWindow()
{
	delete file;
	delete ui;
}

void MainWindow::disconnected() {
	if (isDownloading) {
		QString fileName = fileInfo["Name"].toString();
		QFile file(clientFolder + "/" + fileName);

		if (file.exists()) {
			file.remove();
		}
	}

	close();
}

void MainWindow::readyRead() {
	QByteArray data;
	data.resize(socket->pendingDatagramSize());

	QHostAddress sender;
	quint16 senderPort;

	socket->readDatagram(data.data(), data.size(),
		&sender, &senderPort);

	while (socket->hasPendingDatagrams()) {
		QNetworkDatagram datagram = socket->receiveDatagram();
		data += datagram.data();
	}

	socket->flush();

	if (isWaitingList) {
		/*QStringList filesList;
		stream >> filesList;
		downloadFiles(filesList);

		isWaitingList = false;*/
	} else if (isWaitingHash) {
		/*stream >> fileInfo;

		if (!fileInfo.contains("Name") || !fileInfo.contains("Size")
			|| !fileInfo.contains("Progress")) {

			isWaitingHash = false;
			return;
		}

		QHash<QString, QVariant> request;
		request.insert("startDownload", 0);

		stream << request;

		isWaitingHash = false;
		isDownloading = true;

		ui->download->setDisabled(true);
		ui->upload->setDisabled(true);*/
	} else if (isDownloading) {
		/*QByteArray data = serverSocket->readAll();

		downloading(data);

		if (fileInfo["Progress"].toLongLong() == fileInfo["Size"].toLongLong()) {
			isDownloading = false;
			timer->stop();
			ui->status->setText("Waiting...");
			ui->download->setDisabled(false);
			ui->upload->setDisabled(false);
			downloadedBlocks = 0;
		}*/
	} else {
		QDataStream stream(data);
		QHash<QString, QVariant> reply;
		stream >> reply;

		if (reply.contains(status) && (reply[status] == fileInfoWritten
			|| reply[status] == blockReady)) {
			replyTimer->stop();
			fileTransfer();
			replyTimer->start();

			if (numberOfBlocks == uploadedBlocks && reply[status] == blockReady) {
				numberOfBlocks = 0;
				uploadedBlocks = 0;
				isFileInit = false;
				speedSum = 0;
				replyTimer->stop();

				file->close();
				delete file;
				file = nullptr;

				ui->upload->setDisabled(false);
				ui->download->setDisabled(false);
				ui->status->setText("Waiting...");
			}
		}
	}

	while (socket->hasPendingDatagrams()) {
		QNetworkDatagram datagram = socket->receiveDatagram();
	}
}

void MainWindow::upload() {
	ui->upload->setDisabled(true);
	ui->download->setDisabled(true);

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
		tr("All Files(*.*)"));
	
	if (fileName.isEmpty()) {
		ui->upload->setDisabled(false);
		ui->download->setDisabled(false);
		return;
	}

	file = new QFile(fileName);
	
	if (!file->open(QIODevice::ReadOnly)) {
		QMessageBox messageBox;
		messageBox.setText("Can't open file!!!");
		messageBox.setStandardButtons(QMessageBox::Ok);
		messageBox.setIcon(QMessageBox::Warning);
		messageBox.exec();

		return;
	}

	QHash<QString, QVariant> fileInfo;
	fileInfo.insert("Name", QFileInfo(*file).fileName());
	fileInfo.insert("Size", file->size());
	lastData = fileInfo;

	QByteArray replyByteArray;
	QDataStream stream(&replyByteArray, QIODevice::ReadWrite);

	stream << fileInfo;

	qint64 result = socket->writeDatagram(replyByteArray, QHostAddress(ip), port);

	if (result == -1) {
		showError();
	}

	replyTimer->start();
}

void MainWindow::download() {
	/*QHash<QString, QVariant> request;
	request.insert("download", 0);

	QDataStream socketStream(serverSocket);

	socketStream << request;

	isWaitingList = true;*/
}

void MainWindow::fileTransfer() {
	QString textStatus = "Uploading file: " + QFileInfo(*file).fileName();
	
	if (!isFileInit) {
		qint64 fileSize = file->size();
		numberOfBlocks = fileSize / BUFFER_SIZE;
		numberOfBlocks = fileSize % BUFFER_SIZE != 0 ? numberOfBlocks + 1 : numberOfBlocks;

		ui->status->setText(textStatus);
		isFileInit = true;
	}

	QTime timer;
	timer.start();

	QByteArray data;
	data = file->read(BUFFER_SIZE);

	qint64 result = socket->writeDatagram(data, QHostAddress(ip), port);

	if (result == -1) {
		showError();
	}

	lastData.clear();
	lastData.insert(lastBlock, data);

	double uploadMS = timer.elapsed();

	ui->progressBar->setValue((uploadedBlocks + 1) * 100 / numberOfBlocks);

	if (uploadMS != 0) {
		double speed = (data.size() / 1024.0 / 1024.0) / (uploadMS / 1000.0);

		if (!(uploadedBlocks % speedUpdateBlocks)) {
			speedSum = 0;
		}

		speedSum += speed;
		QString speedString = QString::number(speedSum / (uploadedBlocks % speedUpdateBlocks)) + " mb/s\n";
		ui->status->setText(textStatus + " " + speedString);
	}

	QApplication::processEvents();

	uploadedBlocks++;
}

void MainWindow::downloadFiles(QStringList& filesList) {
	/*DownloadDialog downloadDialog;
	downloadDialog.setFiles(filesList);

	if (downloadDialog.exec() != QDialog::Accepted) {
		return;
	}

	QString fileName = downloadDialog.getFile();

	if (fileName.isEmpty()) {
		return;
	}

	QDataStream socketStream(serverSocket);

	QHash<QString, QVariant> request;
	request.insert("fileName", fileName);

	socketStream << request;

	isWaitingHash = true;*/
}

void MainWindow::downloading(QByteArray& data) {
	/*timer->stop();
	double downloadMS = timerDownload.elapsed();

	QString textStatus = "Downloading file: " + fileInfo["Name"].toString();
	ui->status->setText(textStatus);
	
	if (!QDir(clientFolder).exists()) {
		QDir().mkdir(clientFolder);
	}

	QFile file(clientFolder + "/" + fileInfo["Name"].toString());

	if (!file.open(QIODevice::Append)) {
		return;
	}

	file.write(data);
	file.close();

	fileInfo["Progress"] = fileInfo["Progress"].toLongLong() + data.size();
	timerDownload.start();

	ui->progressBar->setValue(fileInfo["Progress"].toLongLong() * 100 / fileInfo["Size"].toLongLong());
	downloadedBlocks++;

	if (downloadMS != 0 && (downloadedBlocks % speedUpdateBlocks) != 0) {
		double speed = (data.size() / 1024.0 / 1024.0) / (downloadMS / 1000.0);

		if (!(downloadedBlocks % speedUpdateBlocks)) {
			speedDownloadSum = 0;
		}

		speedDownloadSum += speed;
		QString speedString = QString::number(speedDownloadSum / (downloadedBlocks % speedUpdateBlocks)) + " mb/s\n";
		ui->status->setText(textStatus + " " + speedString);
	}

	timer->start(1000);

	QApplication::processEvents();*/
}

void MainWindow::resendData() {
	replyTimer->stop();

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

	replyTimer->start();
}

void MainWindow::showError() {
	QMessageBox* messageBox = new QMessageBox(this);
	messageBox->setAttribute(Qt::WA_DeleteOnClose);
	messageBox->setIcon(QMessageBox::Warning);
	messageBox->setStandardButtons(QMessageBox::Ok);
	messageBox->setText("Write datagram error: " + QString::number(socket->error()));
	messageBox->show();
}
