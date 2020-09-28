#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	socket = new QTcpSocket(this);

	connect(socket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
	connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
	
	connectToServer();
	ui->send->setFocus();
}

MainWindow::~MainWindow()
{
	if (socket->isOpen()) {
		QDataStream stream(socket);
		QString reply = "QUIT";

		stream << reply;

		socket->disconnect();
	}

	delete ui;
}

void MainWindow::connectToServer() {
	socket->connectToHost(ip, port);

	if (socket->state() != QAbstractSocket::ConnectedState) {
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
