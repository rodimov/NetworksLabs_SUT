#include "mailer.h"

#include <QApplication>
#include <QtNetwork>

#include "portdialog.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

	if (!QSslSocket::supportsSsl()) {
		QMessageBox::information(0, "Secure Socket Client",
			"This system does not support SSL/TLS.");
		return -1;
	}

	Mailer w;
	PortDialog portDialog;

	if (portDialog.exec() != QDialog::Accepted) {
		return 0;
	}

	w.setPOPPort(portDialog.getPOPPort());
	w.setPOPAddress(portDialog.getPOPAddress());
	w.setSMTPPort(portDialog.getSMTPPort());
	w.setSMTPAddress(portDialog.getSMTPAddress());
	w.setUsername(portDialog.getUsername());
	w.setPassword(portDialog.getPassword());
	w.show();
	w.connectToServer();

	return a.exec();
}
