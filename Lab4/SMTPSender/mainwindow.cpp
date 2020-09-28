#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "downloaddialog.h"

#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

const qint64 BUFFER_SIZE = 307704;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	socket = new QTcpSocket(this);

	connect(socket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
	connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
	
	connectToServer();
}

MainWindow::~MainWindow()
{
	if (serverSocket->isOpen()) {
		serverSocket->disconnect();
	}

	delete ui;
}

void MainWindow::connectToServer() {
	socket->connectToHost(ip, port);

	if (serverSocket->state() != QAbstractSocket::ConnectedState) {
		close();
	}
}

void MainWindow::disconnected() {
	close();
}

void MainWindow::readyRead() {
	QDataStream stream(socket);
	QString reply;
	
	stream >> reply;
	
	QMessageBox* messageBox = new QMessageBox(this);
	messageBox->setAttribute(Qt::WA_DeleteOnClose);
	messageBox->setIcon(QMessageBox::Warning);
	messageBox->setStandardButtons(QMessageBox::Ok);
	messageBox->setText(reply);
	messageBox->show();
}
