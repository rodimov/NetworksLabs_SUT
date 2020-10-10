#include "mailer.h"
#include "./ui_mailer.h"

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

Mailer::Mailer(QWidget* parent)
	: QMainWindow(parent),
	ui(new Ui::Mailer) {
	
	ui->setupUi(this);

	ui->senderAddress->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->senderName->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui->attached->setTextInteractionFlags(Qt::TextSelectableByMouse);

	socket = new QSslSocket(this);
	setSocketConnectState(true);

	ui->splitter->setStretchFactor(0, 1);
	ui->splitter->setStretchFactor(1, 5);

	ui->previous->setEnabled(false);

	webPage = new WebPage(this);
	ui->messageText->setPage(webPage);

	connect(socket, &QSslSocket::disconnected, this, &Mailer::disconnected);
	connect(ui->messages, &QListWidget::currentItemChanged, this, &Mailer::messageChanged);
	connect(ui->attachSave, &QPushButton::clicked, this, &Mailer::saveAttachment);
	connect(ui->refresh, &QToolButton::clicked, this, &Mailer::refresh);
	connect(ui->previous, &QToolButton::clicked, this, &Mailer::previous);
	connect(ui->next, &QToolButton::clicked, this, &Mailer::next);
	connect(ui->remove, &QToolButton::clicked, this, &Mailer::remove);
	connect(ui->send, &QToolButton::clicked, this, &Mailer::send);
}

Mailer::~Mailer() {
	delete ui;
}

void Mailer::closeEvent(QCloseEvent* event) {
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

void Mailer::connectToServer() {
	setSocketConnectState(false);
	socket->connectToHostEncrypted(popAddress, popPort);

	if (!socket->waitForConnected()) {
		showMessageBox("Connection error!", QMessageBox::Warning);
		setSocketConnectState(true);
		return;
	}

	login();
	getMessagesIndexes();
	getMessages();

	setSocketConnectState(true);
}

void Mailer::disconnected() {
	showMessageBox(QString("Disconnected!"), QMessageBox::Warning);
}

void Mailer::readyRead() {
	QString reply = QString::fromUtf8(socket->readAll());
	showMessageBox(reply, QMessageBox::Warning, true);
}

bool Mailer::waitForResponse(bool isShowError) {
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

bool Mailer::waitForLineResponse(bool isShowError) {
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

void Mailer::showError(ConnectionError error) {
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

bool Mailer::sendMessage(const QString& text, bool isShowError) {
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

void Mailer::login() {
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

void Mailer::refresh() {
	setSocketConnectState(false);

	startPage = 0;
	ui->previous->setEnabled(false);

	if (socket->isOpen()) {
		sendMessage("QUIT", false);
		waitForResponse(false);
		socket->disconnect();

		if (!socket->waitForDisconnected()) {
			showMessageBox("Disconnection error!", QMessageBox::Warning);
		}
	}

	connectToServer();

	setSocketConnectState(true);
}

void Mailer::previous() {
	startPage -= pageSize;

	if (startPage <= 0) {
		ui->previous->setEnabled(false);
	}

	getMessages();
}

void Mailer::next() {
	startPage += pageSize;

	if (startPage >= pageSize) {
		ui->previous->setEnabled(true);
	}

	getMessages();
}

void Mailer::showMessageBox(const QString& text, QMessageBox::Icon icon, bool isExec) {
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

void Mailer::setSocketConnectState(bool isConnected) {
	if (isConnected) {
		connect(socket, &QSslSocket::readyRead, this, &Mailer::readyRead);
	} else {
		disconnect(socket, &QSslSocket::readyRead, this, &Mailer::readyRead);
	}
}

void Mailer::getMessagesIndexes() {
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

void Mailer::getMessages() {
	setSocketConnectState(false);
	ui->messages->clear();
	webPage->setUrl(QUrl());
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

QString Mailer::getLongResponse() {
	QString longResponse;
	responseText = "";

	while (!responseText.contains(".\r\n")) {
		int attempts = 5;
		bool done = false;

		while (attempts && !done) {
			done = waitForLineResponse(false);
			attempts--;
		}

		if (!done) {
			showError(ResponseTimeoutError);
			return "";
		}

		longResponse += responseText;
	}

	return longResponse;
}

Message* Mailer::parseMIME(const QString& mime) {
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

void Mailer::messageChanged(QListWidgetItem* current, QListWidgetItem* previous) {
	webPage->setUrl(QUrl());
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
		webPage->setHtml(message->html);
	} else {
		QTextEdit textEdit;
		textEdit.setPlainText(message->text);
		QString textHtml = textEdit.toHtml();

		webPage->setHtml(textHtml);
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

void Mailer::remove() {
	setSocketConnectState(false);

	int index = ui->messages->currentRow();

	if (messagesList.size() < index || index < 0) {
		return;
	}

	Message* message = messagesList[index];

	sendMessage("DELE " + QString::number(message->messageIndex));
	waitForResponse();

	if (!isResponseOk) {
		return;
	}

	showMessageBox(responseText, QMessageBox::Warning);

	messageIndexes.removeAll(message->messageIndex);
	
	if (messagesList.size() == 1 && startPage != 0) {
		startPage -= pageSize;

		if (startPage <= 0) {
			ui->previous->setEnabled(false);
		}
	}

	getMessages();

	setSocketConnectState(true);
}

void Mailer::encodeBase64(QString& data) {
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

void Mailer::encodeBase64List(QString& data) {
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

void Mailer::saveAttachment() {
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

QString Mailer::toBase64(const QString& text) {
	return QByteArray().append(text).toBase64();
}

void Mailer::send() {
	if (mailSender) {
		mailSender->close();
		delete mailSender;
		mailSender = nullptr;
	}

	mailSender = new MailSender(this);
	mailSender->setPort(smtpPort);
	mailSender->setIP(smtpAddress);
	mailSender->setUsername(toBase64(username));
	mailSender->setPassword(toBase64(password));
	mailSender->show();
	mailSender->connectToServer();
}
