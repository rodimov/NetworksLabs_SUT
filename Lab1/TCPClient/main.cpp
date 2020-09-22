#include "mainwindow.h"
#include "portdialog.h"

#include <QApplication>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	PortDialog portDialog;
	
	if (portDialog.exec() != QDialog::Accepted) {
		return 0;
	}

	MainWindow w;
	w.setPort(portDialog.getPort());
	w.setIP(portDialog.getIP());
	w.connectToServer();

	w.show();

	return a.exec();
}
