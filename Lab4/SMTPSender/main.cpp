#include "mainwindow.h"

#include <QApplication>

#include "portdialog.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
	PortDialog portDialog;

	if (portDialog.exec() != QDialog::Accepted) {
		return 0;
	}

	w.setPort(portDialog.getPort());
	w.setIP(portDialog.getIP());
	w.setUsername(portDialog.getUsername());
	w.setPassword(portDialog.getPassword());
	w.show();
	w.connectToServer();

	return a.exec();
}
