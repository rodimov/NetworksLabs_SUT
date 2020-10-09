#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	void decryptFile(const QString& fileName);
	void encodeFile(const QString& fileName);

protected:
	virtual void dragEnterEvent(QDragEnterEvent* event) override;
	virtual void dropEvent(QDropEvent* event) override;

private:
	QString encodeBytes(QByteArray bytes);
	QByteArray decryptString(QString string);

	const QString base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz0123456789+/";

private slots:
	void encode();
	void encodeFile();
	void decrypt();
	void decryptFile();

private:
	Ui::MainWindow* ui;
};
#endif // MAINWINDOW_H
