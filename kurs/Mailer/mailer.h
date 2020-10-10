#ifndef MAILER_H
#define MAILER_H

#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QWebEnginePage>
#include <QDesktopServices>

#include "mailsender.h"

class QSslSocket;
class QFile;
class QListWidgetItem;
class WebPage;

QT_BEGIN_NAMESPACE
namespace Ui { class Mailer; }
QT_END_NAMESPACE

class OverrideCursor {
public:
	OverrideCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); };
	~OverrideCursor() { QApplication::restoreOverrideCursor(); };
};

struct Message
{
	QString subject;
	QHash<QString, QString> senders;
	QString text;
	QString html;
	QHash<QString, QByteArray> files;
	int messageIndex;
};

class Mailer : public QMainWindow
{
	Q_OBJECT

public:
	Mailer(QWidget* parent = nullptr);
	~Mailer();
	void setPOPPort(int port) { popPort = port; }
	int getPOPPort() { return popPort; }
	void setPOPAddress(QString address) { popAddress = address; }
	QString getPOPAddress() { return popAddress; }
	void setSMTPPort(int port) { smtpPort = port; }
	int getSMTPPort() { return smtpPort; }
	void setSMTPAddress(QString address) { smtpAddress = address; }
	QString getSMTPAddress() { return smtpAddress; }
	void setUsername(QString username) { this->username = username; }
	QString getUsername() { return username; }
	void setPassword(QString password) { this->password = password; }
	QString getPassword() { return password; }

protected:
	void closeEvent(QCloseEvent* event);

private:
	bool waitForResponse(bool isShowError = true);
	bool waitForLineResponse(bool isShowError = true);
	QString getLongResponse();
	void showError(ConnectionError error);
	void showMessageBox(const QString& text, QMessageBox::Icon icon, bool isExec = false);
	bool sendMessage(const QString& text, bool isShowError = true);
	void login();
	Message* parseMIME(const QString& mime);
	void encodeBase64(QString& data);
	void encodeBase64List(QString& data);
	void setSocketConnectState(bool isConnected);
	QString toBase64(const QString& text);

public slots:
	void connectToServer();

private slots:
	void disconnected();
	void readyRead();
	void getMessagesIndexes();
	void getMessages();
	void messageChanged(QListWidgetItem* current, QListWidgetItem* previous);
	void saveAttachment();
	void refresh();
	void previous();
	void next();
	void remove();
	void send();

private:
	Ui::Mailer* ui;
	int popPort = 0;
	QString popAddress;
	int smtpPort = 0;
	QString smtpAddress;
	QString username;
	QString password;
	QSslSocket* socket;
	QString responseText;
	QString okText = "+OK";
	QList<Message*> messagesList;
	QList<int> messageIndexes;
	QStringList filesTypes = { "application",  "audio", "example", "image", "model", "video" };
	WebPage* webPage;
	MailSender* mailSender = nullptr;
	bool isResponseOk;
	int sendMessageTimeout = 5000;
	int responseTimeout = 5000;
	int pageSize = 25;
	int startPage = 0;
};

class WebPage : public QWebEnginePage {
	Q_OBJECT

public:
	WebPage(QObject* parent = nullptr) : QWebEnginePage(parent) {}
	~WebPage() {}

	bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
	{
		if (type == QWebEnginePage::NavigationTypeLinkClicked)
		{
			QDesktopServices::openUrl(url);
			return false;
		}

		return true;
	}
};

#endif // MAILER_H
