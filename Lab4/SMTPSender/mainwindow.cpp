#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSslSocket>
#include <QFileDialog>
#include <QCloseEvent>
#include <QWebEngineView>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow) {
	
	ui->setupUi(this);
	ui->send->setFocus();

	socket = new QSslSocket(this);
	setSocketConnectState(true);

	connect(socket, &QSslSocket::disconnected, this, &MainWindow::disconnected);
	connect(ui->reconnect, &QPushButton::clicked, this, &MainWindow::connectToServer);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (socket->isOpen()) {
		sendMessage("QUIT");

		if (waitForResponse() && responseCode / 100 == 2) {
			showMessageBox(responseText, QMessageBox::Information, true);
		}

		socket->disconnect();
	}

	event->accept();
}

void MainWindow::connectToServer() {
	setSocketConnectState(false);
	socket->connectToHostEncrypted(ip, port);

	waitForResponse();

	if (responseCode / 100 != 2) {
		return;
	}

	sendMessage("EHLO QtMailer");
	waitForResponse();

	if (responseCode != 250) {
		return;
	}

	login();
	setSocketConnectState(true);
}

void MainWindow::disconnected() {
	showMessageBox(QString("Disconnected!"), QMessageBox::Warning);
	ui->send->setDisabled(true);
	ui->reconnect->setDisabled(false);
}

void MainWindow::readyRead() {
	QString reply = QString::fromUtf8(socket->readAll());
	showMessageBox(reply, QMessageBox::Warning, true);
}

bool MainWindow::waitForResponse() {
	if (!socket->waitForReadyRead(responseTimeout)) {
		showError(ResponseTimeoutError);

		responseText = "";
		responseCode = 0;

		return false;
	}

	responseText = QString::fromUtf8(socket->readAll());
	responseCode = responseText.left(3).toInt();

	if (responseCode / 100 == 4) {
		showError(ServerError);
	}

	if (responseCode / 100 == 5) {
		showError(ClientError);
	}

	return true;
}

void MainWindow::showError(SmtpError error) {
	QMessageBox* messageBox = new QMessageBox(this);
	messageBox->setAttribute(Qt::WA_DeleteOnClose);
	messageBox->setIcon(QMessageBox::Warning);
	messageBox->setStandardButtons(QMessageBox::Ok);
	
	switch (error)
	{
	case ConnectionTimeoutError:
		messageBox->setText("Connection timeout error");
		break;
	case ResponseTimeoutError:
		messageBox->setText("Response timeout error");
		break;
	case SendDataTimeoutError:
		messageBox->setText("Send data timeout error");
		break;
	case AuthenticationFailedError:
		messageBox->setText("Authentication failed error");
		break;
	case ServerError:
		messageBox->setText("Server error");
		break;
	case ClientError:
		messageBox->setText("Client error");
		break;
	default:
		break;
	}

	messageBox->show();
}

bool MainWindow::sendMessage(const QString& text) {
	socket->write(text.toUtf8() + "\r\n");
	
	if (!socket->waitForBytesWritten(sendMessageTimeout))
	{
		showError(SendDataTimeoutError);
		return false;
	}

	return true;
}

void MainWindow::login(bool isWebViewWasOpened) {
	ui->send->setDisabled(true);
	ui->reconnect->setDisabled(false);

	sendMessage("AUTH LOGIN");
	waitForResponse();

	if (responseCode != 334) {
		showError(AuthenticationFailedError);
		return;
	}

	sendMessage(username);
	waitForResponse();

	if (responseCode != 334) {
		showError(AuthenticationFailedError);
		return;
	}

	sendMessage(password);
	waitForResponse();

	if (responseCode != 235 && (!responseText.contains("http") || isWebViewWasOpened)) {
		showError(AuthenticationFailedError);
		return;
	} else if (responseText.contains("http") && !isWebViewWasOpened) {
		int indexOfHttp = responseText.indexOf("http");
		QString url = responseText.mid(indexOfHttp);
		openWebPage(url);
		login(true);
		return;
	}

	showMessageBox(responseText, QMessageBox::Information);
	ui->send->setDisabled(false);
	ui->reconnect->setDisabled(true);
}

void MainWindow::openWebPage(QString& url) {
	QDialog* webViewDialog = new QDialog(this);
	webViewDialog->resize(800, 600);
	QWebEngineView* webView = new QWebEngineView(webViewDialog);
	webView->load(QUrl(url));
	QHBoxLayout* layout = new QHBoxLayout(webViewDialog);
	layout->addWidget(webView);
	webViewDialog->setLayout(layout);
	webViewDialog->setAttribute(Qt::WA_DeleteOnClose);
	webViewDialog->exec();
}

void MainWindow::showMessageBox(QString& text, QMessageBox::Icon icon, bool isExec) {
	QMessageBox* messageBox = new QMessageBox(this);
	messageBox->setAttribute(Qt::WA_DeleteOnClose);
	messageBox->setIcon(icon);
	messageBox->setStandardButtons(QMessageBox::Ok);
	messageBox->setText(text);
	
	if (isExec) {
		messageBox->exec();
	} else {
		messageBox->show();
	}
}

void MainWindow::setSocketConnectState(bool isConnected) {
	if (isConnected) {
		connect(socket, &QSslSocket::readyRead, this, &MainWindow::readyRead);
	} else {
		disconnect(socket, &QSslSocket::readyRead, this, &MainWindow::readyRead);
	}
}
