#include "portdialog.h"
#include "./ui_portdialog.h"

PortDialog::PortDialog(QWidget* parent)
	: QDialog(parent),
	ui(new Ui::PortDialog) {

	ui->setupUi(this);

	setFixedSize(width(), height());
	ui->ip->setText("127.0.0.1");
	ui->port->setFocus();

	connect(ui->ok, &QPushButton::clicked, this, &PortDialog::accept);
}

PortDialog::~PortDialog() {
	delete ui;
}

int PortDialog::getPort() {
	return ui->port->text().toInt();
}

QString PortDialog::getIP() {
	return ui->ip->text();
}
