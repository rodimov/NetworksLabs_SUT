#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QUdpSocket>
#include <QDataStream>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->serverTextEdit->setReadOnly(true);
	ui->send->setDefault(true);
	clientSocket = new QUdpSocket(this);

	connect(clientSocket, &QUdpSocket::readyRead, this, &MainWindow::readyRead);
	connect(ui->send, &QPushButton::clicked, this, &MainWindow::send);
	connect(ui->clear, &QPushButton::clicked, this, &MainWindow::clear);
}

MainWindow::~MainWindow()
{
	if (clientSocket->isOpen()) {
		clientSocket->disconnect();
	}

	delete ui;
}

void MainWindow::readyRead() {
	QByteArray buffer;
	buffer.resize(clientSocket->pendingDatagramSize());

	QHostAddress sender;
	quint16 senderPort;

	clientSocket->readDatagram(buffer.data(), buffer.size(),
		&sender, &senderPort);

	QString message(buffer);

	ui->serverTextEdit->append(message);
}

void MainWindow::send() {
	QString data = ui->clientTextEdit->toPlainText();

	clientSocket->writeDatagram(QByteArray().append(data), QHostAddress(ip), port);

	ui->clientTextEdit->clear();
}

void MainWindow::clear() {
	ui->serverTextEdit->clear();
}
