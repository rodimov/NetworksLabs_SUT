#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QTcpSocket>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->serverTextEdit->setReadOnly(true);
	ui->send->setDefault(true);
	clientSocket = new QTcpSocket(this);

	connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
	connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
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

void MainWindow::connectToServer() {
	clientSocket->connectToHost(ip, port);

	if (clientSocket->state() != QAbstractSocket::ConnectedState) {
		close();
	}
}

void MainWindow::disconnected() {
	close();
}

void MainWindow::readyRead() {
	QTcpSocket* clientSocket = static_cast<QTcpSocket*>(QObject::sender());
	QDataStream stream(clientSocket);
	QString message;

	stream >> message;

	ui->serverTextEdit->append(message);
}

void MainWindow::send() {
	QDataStream stream(clientSocket);
	
	stream << ui->clientTextEdit->toPlainText();
	ui->clientTextEdit->clear();
}

void MainWindow::clear() {
	ui->serverTextEdit->clear();
}
