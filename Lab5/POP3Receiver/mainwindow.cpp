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
#include <QThread>
#include <QTextEdit>

#include <mimetic/mimetic.h>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow) {
	
	ui->setupUi(this);

	ui->senderAddress->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->senderName->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->attached->setTextInteractionFlags(Qt::TextSelectableByMouse);

	socket = new QSslSocket(this);
	setSocketConnectState(true);

	ui->splitter->setStretchFactor(0, 1);
	ui->splitter->setStretchFactor(1, 5);

	ui->previous->setEnabled(false);

	connect(socket, &QSslSocket::disconnected, this, &MainWindow::disconnected);
	connect(ui->messages, &QListWidget::currentItemChanged, this, &MainWindow::messageChanged);
	connect(ui->attachSave, &QPushButton::clicked, this, &MainWindow::saveAttachment);
	connect(ui->refresh, &QToolButton::clicked, this, &MainWindow::refresh);
	connect(ui->previous, &QToolButton::clicked, this, &MainWindow::previous);
	connect(ui->next, &QToolButton::clicked, this, &MainWindow::next);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event) {
	setSocketConnectState(false);

	if (socket->isOpen()) {
		sendMessage("QUIT");

		if (waitForResponse()) {
			showMessageBox(responseText, QMessageBox::Information, true);
		}

		socket->disconnect();
	}

	event->accept();
}

void MainWindow::connectToServer() {
	setSocketConnectState(false);
	socket->connectToHostEncrypted(ip, port);

	login();
	getMessagesIndexes();
	getMessages();

	setSocketConnectState(true);
}

void MainWindow::disconnected() {
	showMessageBox(QString("Disconnected!"), QMessageBox::Warning);
}

void MainWindow::readyRead() {
	QString reply = QString::fromUtf8(socket->readAll());
	showMessageBox(reply, QMessageBox::Warning, true);
}

bool MainWindow::waitForResponse(bool isShowError) {
	if (!socket->waitForReadyRead(responseTimeout)) {
		if (isShowError) {
			showError(ResponseTimeoutError);
		}

		responseText = "";
		isResponseOk = false;

		return isResponseOk;
	}

	responseText = QString::fromUtf8(socket->readAll());
	isResponseOk = responseText.contains(okText);

	return true;
}

bool MainWindow::waitForLineResponse(bool isShowError) {
	if (!socket->canReadLine() && !socket->waitForReadyRead(responseTimeout)) {
		if (isShowError) {
			showError(ResponseTimeoutError);
		}

		responseText = "";
		isResponseOk = false;

		return isResponseOk;
	}

	responseText = QString::fromUtf8(socket->readLine());
	isResponseOk = responseText.contains(okText);

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

bool MainWindow::sendMessage(const QString& text, bool isShowError) {
	socket->write(text.toUtf8() + "\r\n");
	
	if (!socket->waitForBytesWritten(sendMessageTimeout))
	{
		if (isShowError) {
			showError(SendDataTimeoutError);
		}
		return false;
	}

	return true;
}

void MainWindow::login() {
	sendMessage("USER " + username, false);
	waitForResponse();

	if (!isResponseOk) {
		showError(AuthenticationFailedError);
		return;
	}

	sendMessage("PASS " + password);
	waitForResponse();

	if (!isResponseOk) {
		showError(AuthenticationFailedError);
		return;
	}

	QString oldResponse = responseText;

	waitForResponse(false);

	if (isResponseOk) {
		showMessageBox(responseText, QMessageBox::Information);
	} else {
		showMessageBox(oldResponse, QMessageBox::Information);
	}
}

void MainWindow::refresh() {
	setSocketConnectState(false);

	startPage = 1;
	ui->previous->setEnabled(false);
	sendMessage("QUIT");
	waitForResponse();

	connectToServer();

	setSocketConnectState(true);
}

void MainWindow::previous() {
	startPage -= pageSize;

	if (startPage <= 0) {
		ui->previous->setEnabled(false);
	}

	getMessages();
}

void MainWindow::next() {
	startPage += pageSize;

	if (startPage >= pageSize) {
		ui->previous->setEnabled(true);
	}

	getMessages();
}

void MainWindow::showMessageBox(const QString& text, QMessageBox::Icon icon, bool isExec) {
	if (text.isEmpty()) {
		return;
	}

	if (text.size() > 100) {
		return;
	}

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

void MainWindow::getMessagesIndexes() {
	setSocketConnectState(false);
	messageIndexes.clear();

	OverrideCursor overrideCursor;

	sendMessage("LIST");
	QString messagesIndexes = getLongResponse();

	for (auto& line : messagesIndexes.split("\n")) {
		QString number = line.split(" ")[0];
		bool isOk;
		int messageIndex = number.toInt(&isOk);

		if (!isOk) {
			continue;
		}

		messageIndexes.append(messageIndex);
	}

	setSocketConnectState(true);
}

void MainWindow::getMessages() {
	setSocketConnectState(false);
	ui->messages->clear();
	ui->messageText->setUrl(QUrl());
	ui->senderAddress->clear();
	ui->senderName->clear();
	ui->attached->clear();

	OverrideCursor overrideCursor;

	for (auto message : messagesList) {
		delete message;
	}

	messagesList.clear();

	for (int i = startPage; i < startPage + pageSize; i++) {
		if (i >= messageIndexes.size()) {
			break;
		}

		sendMessage("RETR " + QString::number(messageIndexes[i]));
		QString messageString = getLongResponse();
		QStringList messageList = messageString.split("\r\n");

		if (messageList.size() > 0 && messageList[0].contains(okText)) {
			messageList.removeFirst();
		}

		if (messageList.size() > 1) {
			messageList.removeLast();
			messageList.removeLast();
		}

		messageString = messageList.join("\r\n") + "\r\n";
		Message* message = parseMIME(messageString);
		message->messageIndex = messageIndexes[i];
		ui->messages->addItem(message->subject);
		messagesList.append(message);
	}

	if (startPage + pageSize >= messageIndexes.size()) {
		ui->next->setEnabled(false);
	} else {
		ui->next->setEnabled(true);
	}

	setSocketConnectState(true);
}

QString MainWindow::getLongResponse() {
	QString longResponse;
	responseText = "";

	while (!responseText.contains(".\r\n")) {
		if (!waitForLineResponse())
		{
			return "";
		}

		longResponse += responseText;
	}

	return longResponse;
}

Message* MainWindow::parseMIME(QString& mime) {
	Message* message = new Message;
	std::string stdData = mime.toStdString();
	std::istringstream stringStream(stdData);
	std::istreambuf_iterator<char> begin(stringStream), end;

	mimetic::MimeEntity me(begin, end);
	mimetic::Header mimeHeader = me.header();
	mimetic::Body mimeBody = me.body();

	QString subject = QString::fromStdString(mimeHeader.subject());
	encodeBase64List(subject);
	message->subject = subject.simplified();

	QHash<QString, QString> senders;

	for (const auto& sender : mimeHeader.from()) {
		QString senderAddress = QString::fromStdString(sender.mailbox() + "@" + sender.domain());
		QString senderName = QString::fromStdString(sender.label());
		encodeBase64(senderName);
		senders.insert(senderName, senderAddress);
	}

	message->senders = senders;

	mimetic::MimeEntityList entityList = mimeBody.parts();

	for (auto mimeEntity : entityList) {
		mimetic::Header mimeEnityHeader = mimeEntity->header();
		QString contentType = QString::fromStdString(mimeEnityHeader.contentType().type());
		QString contentSubType = QString::fromStdString(mimeEnityHeader.contentType().subtype());
		QString contentEcoding = QString::fromStdString(mimeEnityHeader.contentTransferEncoding().str());
		QString body = QString::fromStdString(mimeEntity->body());

		if (contentType == "text" && contentEcoding == "base64") {
			body = QByteArray::fromBase64(body.toLatin1());
		}

		if (contentType == "text" && contentSubType == "html") {
			message->html = body;
		} else if (contentType == "text" && contentSubType == "plain") {
			message->text = body;
		} else if (filesTypes.contains(contentType)) {
			QString fileName = QString::fromStdString(mimeEnityHeader.contentType().param("name"));

			if (fileName.isEmpty()) {
				continue;
			}

			QByteArray fileData = QByteArray::fromBase64(body.toLatin1());
			message->files.insert(fileName, fileData);
		}
	}

	return message;
}

void MainWindow::messageChanged(QListWidgetItem* current, QListWidgetItem* previous) {
	ui->messageText->setUrl(QUrl());
	ui->senderAddress->clear();
	ui->senderName->clear();
	ui->attached->clear();

	int index = ui->messages->row(current);

	if (messagesList.size() < index || index < 0) {
		return;
	}

	Message* message = messagesList[index];

	ui->attachSave->setEnabled(message->files.size());

	for (auto& senderName : message->senders.keys()) {
		if (!ui->senderAddress->text().isEmpty()) {
			ui->senderAddress->setText(ui->senderAddress->text() + " " + message->senders[senderName]);
		} else {
			ui->senderAddress->setText(message->senders[senderName]);
		}

		if (!ui->senderName->text().isEmpty()) {
			ui->senderName->setText(ui->senderName->text() + " " + senderName);
		}
		else {
			ui->senderName->setText(senderName);
		}
	}

	if (!message->html.isEmpty()) {
		ui->messageText->setHtml(message->html);
	} else {
		QTextEdit textEdit;
		textEdit.setPlainText(message->text);
		QString textHtml = textEdit.toHtml();

		ui->messageText->setHtml(textHtml);
	}

	for (auto file : message->files.keys()) {
		if (!ui->attached->text().isEmpty()) {
			ui->attached->setText(ui->attached->text() + " " + file);
		}
		else {
			ui->attached->setText(file);
		}
	}
}

void MainWindow::remove() {
	int index = ui->messages->currentRow();

	if (messagesList.size() < index || index < 0) {
		return;
	}

	Message* message = messagesList[index];


}

void MainWindow::encodeBase64(QString& data) {
	QString base64start = "=?utf-8?b?";
	QString base64end = "?=";

	if (data.contains(base64start, Qt::CaseInsensitive)) {
		int start = data.indexOf(base64start, 0, Qt::CaseInsensitive);
		int end = data.indexOf(base64end, 0, Qt::CaseInsensitive);

		if (start >= end) {
			return;
		}

		QString base64 = data.mid(start + base64start.size(), end - base64start.size() - start);
		QString subjectBegin = data.left(start);
		QString subjectEnding = data.mid(end + base64end.size(), data.size() - end + base64end.size());
		data = subjectBegin + QByteArray::fromBase64(base64.toLatin1()) + subjectEnding;
	}
}

void MainWindow::encodeBase64List(QString& data) {
	QString base64start = "=?utf-8?b?";
	QString base64end = "?=";

	while (data.contains(base64start, Qt::CaseInsensitive)) {
		int start = data.indexOf(base64start, 0, Qt::CaseInsensitive);
		int end = data.indexOf(base64end, 0, Qt::CaseInsensitive);

		if (start >= end) {
			break;
		}

		QString base64 = data.mid(start + base64start.size(), end - base64start.size() - start);
		QString subjectBegin = data.left(start);
		QString subjectEnding = data.mid(end + base64end.size(), data.size() - end + base64end.size());
		data = subjectBegin + QByteArray::fromBase64(base64.toLatin1()) + subjectEnding.simplified();
	}

	data.simplified();
}

void MainWindow::saveAttachment() {
	int index = ui->messages->currentRow();

	if (messagesList.size() < index || index < 0) {
		return;
	}

	Message* message = messagesList[index];

	QString saveDir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
		QString(),
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	if (saveDir.isEmpty()) {
		return;
	}

	OverrideCursor overrideCursor;

	for (auto fileName : message->files.keys()) {
		QFile file(saveDir + "/" + fileName);

		if (file.exists()) {
			showMessageBox("File: " + file.fileName() + " already exists!", QMessageBox::Warning);
			continue;
		}

		if (!file.open(QIODevice::WriteOnly)) {
			showMessageBox("Can't oprn file: " + file.fileName(), QMessageBox::Warning);
			continue;
		}

		file.write(message->files[fileName]);
		file.close();

		showMessageBox("File " + fileName +" saved!", QMessageBox::Information);
	}
}

QString MainWindow::toBase64(const QString& text) {
	return QByteArray().append(text).toBase64();
}
