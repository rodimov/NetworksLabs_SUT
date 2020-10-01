#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSslSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QWebEngineView>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow) {
	
	ui->setupUi(this);
	ui->send->setFocus();

	socket = new QSslSocket(this);

	connect(socket, &QSslSocket::disconnected, this, &MainWindow::disconnected);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (socket->isOpen()) {
		sendMessage("QUIT");

		if (waitForResponse() && responseCode / 100 == 2) {
			QMessageBox messageBox;
			messageBox.setIcon(QMessageBox::Warning);
			messageBox.setStandardButtons(QMessageBox::Ok);
			messageBox.setText("Disconnected!");
			messageBox.exec();
		}

		socket->disconnect();
	}

	event->accept();
}

void MainWindow::connectToServer() {
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
}

void MainWindow::disconnected() {
	
}

void MainWindow::readyRead() {
	QString reply = QString::fromUtf8(socket->readAll());
	
	QMessageBox* messageBox = new QMessageBox(this);
	messageBox->setAttribute(Qt::WA_DeleteOnClose);
	messageBox->setIcon(QMessageBox::Warning);
	messageBox->setStandardButtons(QMessageBox::Ok);
	messageBox->setText(reply);
	messageBox->show();
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

	QMessageBox* messageBox = new QMessageBox(this);
	messageBox->setAttribute(Qt::WA_DeleteOnClose);
	messageBox->setIcon(QMessageBox::Information);
	messageBox->setStandardButtons(QMessageBox::Ok);
	messageBox->setText(responseText);
	messageBox->show();
}

void MainWindow::openWebPage(QString& url) {
	QDialog* webViewDialog = new QDialog(this);
	QWebEngineView* webView = new QWebEngineView(webViewDialog);
	webView->load(QUrl(url));
	QHBoxLayout* layout = new QHBoxLayout(webViewDialog);
	layout->addWidget(webView);
	webViewDialog->setLayout(layout);
	webViewDialog->setAttribute(Qt::WA_DeleteOnClose);
	webViewDialog->exec();
}
