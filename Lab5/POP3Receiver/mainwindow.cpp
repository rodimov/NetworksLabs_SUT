#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSslSocket>
#include <QFileDialog>
#include <QCloseEvent>
#include <QWebEngineView>
#include <QUrl>
#include <QCryptographicHash>
#include <QFile>
#include <QTimer>
#include <QFontDialog>
#include <QColorDialog>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow) {
	
	ui->setupUi(this);
	//ui->send->setFocus();

	socket = new QSslSocket(this);
	setSocketConnectState(true);

	timer = new QTimer(this);
	timer->setInterval(sendNoopInterval);

	connect(socket, &QSslSocket::disconnected, this, &MainWindow::disconnected);
	connect(ui->reconnect, &QPushButton::clicked, this, &MainWindow::connectToServer);
	/*connect(ui->send, &QPushButton::clicked, this, &MainWindow::sendMail);
	connect(ui->attach, &QPushButton::clicked, this, &MainWindow::attach);
	connect(ui->bold, &QToolButton::clicked, this, &MainWindow::bold);
	connect(ui->font, &QToolButton::clicked, this, &MainWindow::font);
	connect(ui->color, &QToolButton::clicked, this, &MainWindow::color);
	connect(ui->backgroundColor, &QToolButton::clicked,
		this, &MainWindow::backgroundColor);
	connect(ui->alignLeft, &QToolButton::clicked, this, &MainWindow::align);
	connect(ui->alignCenter, &QToolButton::clicked, this, &MainWindow::align);
	connect(ui->alignRight, &QToolButton::clicked, this, &MainWindow::align);
	connect(ui->alignJustify, &QToolButton::clicked, this, &MainWindow::align);*/
	connect(timer, &QTimer::timeout, this, &MainWindow::sendNoop);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event) {
	timer->stop();
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
	timer->stop();
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
	timer->stop();
	showMessageBox(QString("Disconnected!"), QMessageBox::Warning);
	//ui->send->setDisabled(true);
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
	case SendMailError:
		messageBox->setText("Send mail error");
		break;
	case NoopError:
		messageBox->setText("Noop error");
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
	//ui->send->setDisabled(true);
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
	//ui->send->setDisabled(false);
	ui->reconnect->setDisabled(true);
	timer->start();
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

void MainWindow::showMessageBox(const QString& text, QMessageBox::Icon icon, bool isExec) {
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

void MainWindow::sendMail() {
	timer->stop();
	setSocketConnectState(false);

	QString sender = QByteArray::fromBase64(username.toUtf8());
	sendMessage("MAIL FROM:<" + sender + ">");
	waitForResponse();

	if (responseCode != 250) {
		showError(SendMailError);
		return;
	}

	//sendMessage("RCPT TO: " + ui->recipientAddress->text());
	waitForResponse();

	if (responseCode != 250) {
		showError(SendMailError);
		return;
	}

	sendMessage("DATA");
	waitForResponse();

	if (responseCode != 354) {
		showError(SendMailError);
		return;
	}

	sendMessage(createMIME());
	sendMessage(".");
	waitForResponse();

	if (responseCode != 250) {
		showError(SendMailError);
		return;
	}

	showMessageBox("Done!", QMessageBox::Information);

	setSocketConnectState(true);
	files.clear();

	for (auto file : files) {
		delete file;
	}

	//ui->name->clear();
	//ui->recipientAddress->clear();
	//ui->recipientName->clear();
	//ui->subject->clear();
	//ui->body->clear();
	//ui->attached->clear();

	timer->start();
}

QString MainWindow::createMIME() {
	QString mime = "From:";
	/*QString senderName = ui->name->text();
	QString senderAddress = QByteArray::fromBase64(username.toUtf8());

	if (!senderName.isEmpty()) {
		mime += " =?utf-8?B?" + toBase64(senderName) + "?=";
	}

	mime += " <" + senderAddress + ">\r\n";

	mime += "To:";
	QString recipientAddress = ui->recipientAddress->text();
	QString recipientName = ui->recipientName->text();

	if (!recipientName.isEmpty()) {
		mime += " =?utf-8?B?" + toBase64(recipientName) + "?=";
	}

	mime += " <" + recipientAddress + ">\r\n";

	QDateTime currentTime = QDateTime::currentDateTime();

	mime += "Subject: ";
	mime += "=?utf-8?B?" + toBase64(ui->subject->text()) + "?=\r\n";
	mime += "MIME-Version: 1.0\r\n";
	mime += QString("Date: %1\r\n").arg(currentTime.toString(Qt::RFC2822Date));

	qsrand(currentTime.toSecsSinceEpoch());
	QCryptographicHash md5(QCryptographicHash::Md5);
	md5.addData(QByteArray().append(qrand()));
	QString boundary = md5.result().toHex();

	mime += "Content-Type: multipart/mixed; boundary=" + boundary + "\r\n";
	
	mime += "--" + boundary + "\r\n";
	
	mime += "Content-Type: text/html; charset=utf-8\r\n";
	mime += "Content-Transfer-Encoding: 8bit\r\n";
	mime += "\r\n";
	mime += ui->body->toHtml();
	mime += "\r\n";

	mime += "--" + boundary + "\r\n";

	for (auto file : files) {
		QString fileName = QFileInfo(*file).fileName();

		if (!file->open(QIODevice::ReadOnly)) {
			showMessageBox("Can't open " + fileName, QMessageBox::Warning);
			continue;
		}

		mime += "Content-Type: application/octet-stream; name=\"" + fileName +"\"\r\n";
		mime += "Content-Transfer-Encoding: base64\r\n";
		mime += "Content-disposition: attachment\r\n";
		mime += "\r\n";
		mime += file->readAll().toBase64();
		mime += "\r\n";
		mime += "\r\n";
		mime += "--" + boundary + "\r\n";

		file->close();
	}

	mime += "--" + boundary + "--\r\n";*/

	return mime;
}

void MainWindow::attach() {
	/*QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
		tr("All Files(*.*)"));

	if (fileName.isEmpty()) {
		return;
	}

	if (QFileInfo(QFile(fileName)).size() > maxFileSize) {
		showMessageBox("File is too long", QMessageBox::Warning);
		return;
	}

	QFile* file = new QFile(fileName, this);
	files.push_back(file);

	if (!ui->attached->text().isEmpty()) {
		ui->attached->setText(ui->attached->text() + "; " + QFileInfo(*file).fileName());
	} else {
		ui->attached->setText(QFileInfo(*file).fileName());
	}*/
}

QString MainWindow::toBase64(const QString& text) {
	return QByteArray().append(text).toBase64();
}

void MainWindow::sendNoop() {
	setSocketConnectState(false);

	sendMessage("NOOP");
	waitForResponse();

	if (responseCode != 250) {
		showError(NoopError);
	}

	setSocketConnectState(true);
}

void MainWindow::bold() {
	/*QFont font(ui->body->currentFont());
	font.setBold(ui->bold->isChecked());
	ui->body->setCurrentFont(font);*/
}

void MainWindow::font() {
	/*QFont font(ui->body->currentFont());
	bool ok;
	QFont newFont = QFontDialog::getFont(&ok, font, this);

	if (!ok) {
		return;
	}

	ui->body->setCurrentFont(newFont);*/
}

void MainWindow::color() {
	/*QColor currentColor = ui->body->textColor();
	QColor newColor = QColorDialog::getColor(currentColor, this, "Select text color");
	ui->body->setTextColor(newColor);*/
}

void MainWindow::backgroundColor() {
	/*QColor currentColor = ui->body->textBackgroundColor();
	QColor newColor = QColorDialog::getColor(currentColor, this,
		"Select background text color");
	ui->body->setTextBackgroundColor(newColor);*/
}

void MainWindow::align() {
	/*QToolButton* button = static_cast<QToolButton*>(sender());

	ui->alignLeft->setChecked(false);
	ui->alignCenter->setChecked(false);
	ui->alignRight->setChecked(false);
	ui->alignJustify->setChecked(false);
	
	if (button == ui->alignLeft) {
		ui->body->setAlignment(Qt::AlignLeft);
		ui->alignLeft->setChecked(true);
	} else if (button == ui->alignCenter) {
		ui->body->setAlignment(Qt::AlignCenter);
		ui->alignCenter->setChecked(true);
	} else if (button == ui->alignRight) {
		ui->body->setAlignment(Qt::AlignRight);
		ui->alignRight->setChecked(true);
	} else if (button == ui->alignJustify) {
		ui->body->setAlignment(Qt::AlignJustify);
		ui->alignJustify->setChecked(true);
	}*/
}
