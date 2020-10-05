#include "portdialog.h"
#include "./ui_portdialog.h"

PortDialog::PortDialog(QWidget* parent)
	: QDialog(parent),
	ui(new Ui::PortDialog) {

	ui->setupUi(this);

	setFixedSize(width(), height());
	ui->ip->setFocus();

	connect(ui->ok, &QPushButton::clicked, this, &PortDialog::accept);
}

PortDialog::~PortDialog() {
	delete ui;
}

int PortDialog::getPort() {
	return ui->port->value();
}

QString PortDialog::getIP() {
	return ui->ip->text();
}

QString PortDialog::getUsername() {
	return QString::fromLatin1(ui->username->text().toLatin1().toBase64());
}

QString PortDialog::getPassword() {
	return QString::fromLatin1(ui->password->text().toLatin1().toBase64());
}
