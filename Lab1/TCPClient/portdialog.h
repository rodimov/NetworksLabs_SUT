#ifndef PORTDIALOG_H
#define PORTDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class PortDialog; }
QT_END_NAMESPACE

class PortDialog : public QDialog
{
	Q_OBJECT

public:
	PortDialog(QWidget* parent = nullptr);
	~PortDialog();
	int getPort();
	QString getIP();

private:
	Ui::PortDialog*ui;
};
#endif // PORTDIALOG_H
