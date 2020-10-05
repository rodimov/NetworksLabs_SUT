#include "portdialog.h"
#include "./ui_portdialog.h"

PortDialog::PortDialog(QWidget* parent)
	: QDialog(parent),
	ui(new Ui::PortDialog) {

	ui->setupUi(this);

	setFixedSize(width(), height());
	ui->smtpAddress->setFocus();

	connect(ui->ok, &QPushButton::clicked, this, &PortDialog::accept);
}

PortDialog::~PortDialog() {
	delete ui;
}

int PortDialog::getSMTPPort() {
	return ui->smtpPort->value();
}

QString PortDialog::getSMTPAddress() {
	return ui->smtpAddress->text();
}

int PortDialog::getPOPPort() {
	return ui->popPort->value();
}

QString PortDialog::getPOPAddress() {
	return ui->popAddress->text();
}

QString PortDialog::getUsername() {
	return ui->username->text();
}

QString PortDialog::getPassword() {
	return ui->password->text();
}
