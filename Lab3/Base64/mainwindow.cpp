#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	setAcceptDrops(true);

	connect(ui->encode, &QPushButton::clicked, this, &MainWindow::encode);
	connect(ui->encodeFile, &QPushButton::clicked, this, QOverload<>::of(&MainWindow::encodeFile));
	connect(ui->decrypt, &QPushButton::clicked, this, &MainWindow::decrypt);
	connect(ui->decryptFile, &QPushButton::clicked, this, QOverload<>::of(&MainWindow::decryptFile));
}

MainWindow::~MainWindow()
{
	delete ui;
}

QString MainWindow::encodeBytes(QByteArray bytes) {
	qint64 bytesTrioCount = bytes.size() / 3;
	QString result;

	for (int i = 0; i < bytesTrioCount; i++) {
		int address = i * 3;

		quint8 firstByte = bytes[address];
		quint8 secondByte = bytes[address + 1];
		quint8 thirdByte = bytes[address + 2];

		quint8 firstChar = firstByte >> 2;
		quint8 secondChar = ((firstByte << 4) | (secondByte >> 4)) & 0x3f;
		quint8 thirdChar = ((secondByte << 2) | (thirdByte >> 6)) & 0x3f;
		quint8 fouthChar = thirdByte & 0x3f;

		result += base64Chars[firstChar];
		result += base64Chars[secondChar];
		result += base64Chars[thirdChar];
		result += base64Chars[fouthChar];
	}

	int leftBytes = bytes.size() % 3;

	if (leftBytes > 0) {
		quint8 firstByte = bytes[static_cast<int>(bytesTrioCount) * 3];
		quint8 secondByte = leftBytes > 1 ? bytes[static_cast<int>(bytesTrioCount) * 3 + 1] : 0;

		quint8 firstChar = firstByte >> 2;
		quint8 secondChar = ((firstByte << 4) | (secondByte >> 4)) & 0x3f;
		quint8 thirdChar = (secondByte << 2) & 0x3f;

		result += base64Chars[firstChar];
		result += base64Chars[secondChar];

		if (leftBytes == 2) {
			result += base64Chars[thirdChar];
			result += "=";
		} else {
			result += "==";
		}
	}

	return result;
}

QByteArray MainWindow::decryptString(QString string) {
	qint64 charQuadCount = string.size() / 4;
	QByteArray result;

	for (int i = 0; i < charQuadCount; i++) {
		int address = i * 4;

		quint8 firstChar = base64Chars.indexOf(string[address].toLatin1());
		quint8 secondChar = base64Chars.indexOf(string[address + 1].toLatin1());
		quint8 thirdChar = base64Chars.indexOf(string[address + 2].toLatin1());
		quint8 fouthChar = base64Chars.indexOf(string[address + 3].toLatin1());

		result.append((firstChar << 2) | (secondChar >> 4));

		if (string[address + 2] != "=") {
			result.append((secondChar << 4) | (thirdChar >> 2));
		}
		
		if (string[address + 3] != "=") {
			result.append((thirdChar << 6) | fouthChar);
		}
	}

	return result;
}

void MainWindow::encode() {
	QString decrypted = ui->decrypted->toPlainText();
	ui->encoded->setText(encodeBytes(decrypted.toLocal8Bit()));
}

void MainWindow::encodeFile() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
		tr("All Files(*.*)"));

	if (fileName.isEmpty()) {
		return;
	}

	encodeFile(fileName);
}

void MainWindow::encodeFile(const QString& fileName) {
	QFile file(fileName);

	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}

	QString title = windowTitle();
	setWindowTitle(title + " - encoding...");

	QByteArray data = file.readAll();
	file.close();

	ui->encoded->setText(encodeBytes(data));
	ui->decrypted->clear();

	setWindowTitle(title);
}

void MainWindow::decrypt() {
	QString encoded = ui->encoded->toPlainText();
	ui->decrypted->setText(QString::fromLatin1(decryptString(encoded)));
}

void MainWindow::decryptFile() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "",
		tr("All Files(*.*)"));

	if (fileName.isEmpty()) {
		return;
	}

	decryptFile(fileName);
}

void MainWindow::decryptFile(const QString& fileName) {
	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly)) {
		return;
	}

	QString title = windowTitle();
	setWindowTitle(title + " - decrypting...");

	QString encoded = ui->encoded->toPlainText();
	file.write(decryptString(encoded));
	file.close();

	setWindowTitle(title);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->urls().size() == 1) {
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent* event)
{
	for (const QUrl& url : event->mimeData()->urls()) {
		QString fileName = url.toLocalFile();
		encodeFile(fileName);
	}
}
