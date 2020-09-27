#include "downloaddialog.h"
#include "./ui_downloaddialog.h"

DownloadDialog::DownloadDialog(QWidget* parent)
	: QDialog(parent),
	ui(new Ui::DownloadDialog) {

	ui->setupUi(this);

	connect(ui->ok, &QPushButton::clicked, this, &DownloadDialog::accept);
}

DownloadDialog::~DownloadDialog() {
	delete ui;
}

void DownloadDialog::setFiles(const QStringList& files) {
	ui->filesList->addItems(files);
}

QString DownloadDialog::getFile() {
	if (!ui->filesList->currentItem()) {
		return "";
	}

	return ui->filesList->currentItem()->text();
}
