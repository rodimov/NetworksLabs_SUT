#include "portdialog.h"
#include "./ui_portdialog.h"

PortDialog::PortDialog(QWidget* parent)
	: QDialog(parent),
	ui(new Ui::PortDialog) {

	ui->setupUi(this);

	setFixedSize(width(), height());
	ui->port->setFocus();

	ui->ip->setText("pop.mail.ru");
	ui->username->setText("mailtester.2020");
	ui->password->setText("Zxcvbnm1+");

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
	return ui->username->text();
}

QString PortDialog::getPassword() {
	return ui->password->text();
}
