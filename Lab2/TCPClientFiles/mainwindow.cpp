#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "downloaddialog.h"

#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QDragEnterEvent>
#include <QMimeData>

const qint64 BUFFER_SIZE = 307704;

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
	serverSocket = new QTcpSocket(this);
	timer = new QTimer(this);

	connect(serverSocket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
	connect(serverSocket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
	connect(timer, &QTimer::timeout, this, &MainWindow::readyRead);
	connect(ui->upload, &QPushButton::clicked, this, &MainWindow::upload);
	connect(ui->download, &QPushButton::clicked, this, &MainWindow::download);
}

MainWindow::~MainWindow()
{
	if (serverSocket->isOpen()) {
		serverSocket->disconnect();
	}

	delete ui;
}

void MainWindow::connectToServer() {
	serverSocket->connectToHost(ip, port);

	if (serverSocket->state() != QAbstractSocket::ConnectedState) {
		close();
	}
}

void MainWindow::disconnected() {
	if (isDownloading) {
		QString fileName = fileInfo["Name"].toString();
		QFile file("client_files/" + fileName);

		if (file.exists()) {
			file.remove();
		}
	}

	close();
}

void MainWindow::readyRead() {
	QDataStream stream(serverSocket);

	if (isWaitingList) {
		QStringList filesList;
		stream >> filesList;
		downloadFiles(filesList);

		isWaitingList = false;
	} else if (isWaitingHash) {
		stream >> fileInfo;

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
		ui->upload->setDisabled(true);
	} else if (isDownloading) {
		QByteArray data = serverSocket->readAll();

		downloading(data);

		if (fileInfo["Progress"].toLongLong() == fileInfo["Size"].toLongLong()) {
			isDownloading = false;
			timer->stop();
			ui->status->setText("Waiting...");
			ui->download->setDisabled(false);
			ui->upload->setDisabled(false);
			downloadedBlocks = 0;
		}
	} else {
		QString message;

		stream >> message;

		if (message == "ok") {
			fileTransfer();
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
	file = new QFile(fileName);

	if (!file->open(QIODevice::ReadOnly)) {
		QMessageBox messageBox;
		messageBox.setText("Can't open file!!!");
		messageBox.setStandardButtons(QMessageBox::Ok);
		messageBox.setIcon(QMessageBox::Warning);
		messageBox.exec();

		delete file;
		file = nullptr;

		return;
	}

	ui->upload->setDisabled(true);
	ui->download->setDisabled(true);

	QHash<QString, QVariant> fileInfo;
	fileInfo.insert("Name", QFileInfo(*file).fileName());
	fileInfo.insert("Size", file->size());

	QDataStream stream(serverSocket);
	stream << fileInfo;
}

void MainWindow::download() {
	QHash<QString, QVariant> request;
	request.insert("download", 0);

	QDataStream socketStream(serverSocket);

	socketStream << request;

	isWaitingList = true;
}

void MainWindow::fileTransfer() {
	qint64 fileSize = file->size();
	qint64 numberOfBlocks = fileSize / BUFFER_SIZE;
	numberOfBlocks = fileSize % BUFFER_SIZE != 0 ? numberOfBlocks + 1 : numberOfBlocks;
	QTime timer;
	double speedSum = 0;

	QString textStatus = "Uploading file: " + QFileInfo(*file).fileName();
	ui->status->setText(textStatus);

	for (qint64 i = 0; i < numberOfBlocks; i++) {
		timer.start();

		QByteArray data;
		data = file->read(BUFFER_SIZE);

		serverSocket->write(data);
		serverSocket->waitForBytesWritten();

		double uploadMS = timer.elapsed();

		ui->progressBar->setValue((i + 1) * 100 / numberOfBlocks);

		if (uploadMS != 0) {
			double speed = (data.size() / 1024.0 / 1024.0) / (uploadMS / 1000.0);

			if (!(i % speedUpdateBlocks)) {
				speedSum = 0;
			}

			speedSum += speed;
			QString speedString = QString::number(speedSum / (i % speedUpdateBlocks)) + " mb/s\n";
			ui->status->setText(textStatus + " " + speedString);
		}

		QApplication::processEvents();
	}

	file->close();
	delete file;
	file = nullptr;

	ui->upload->setDisabled(false);
	ui->download->setDisabled(false);
	ui->status->setText("Waiting...");
}

void MainWindow::downloadFiles(const QStringList& filesList) {
	DownloadDialog downloadDialog;
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

	isWaitingHash = true;
}

void MainWindow::downloading(const QByteArray& data) {
	timer->stop();
	double downloadMS = timerDownload.elapsed();

	QString textStatus = "Downloading file: " + fileInfo["Name"].toString();
	ui->status->setText(textStatus);
	
	if (!QDir("client_files").exists()) {
		QDir().mkdir("client_files");
	}

	QFile file("client_files/" + fileInfo["Name"].toString());

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

	QApplication::processEvents();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	if (ui->upload->isEnabled() && event->mimeData()->urls().size() == 1) {
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent* event)
{
	for(const QUrl& url : event->mimeData()->urls()) {
		QString fileName = url.toLocalFile();
		uploadFile(fileName);
	}
}
