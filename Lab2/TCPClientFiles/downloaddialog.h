#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class DownloadDialog; }
QT_END_NAMESPACE

class DownloadDialog : public QDialog
{
	Q_OBJECT

public:
	DownloadDialog(QWidget* parent = nullptr);
	~DownloadDialog();

	void setFiles(const QStringList& files);
	QString getFile();

private:
	Ui::DownloadDialog*ui;
};
#endif // DOWNLOADDIALOG_H
