#include "multi_select_file_dialog.h"


MultiSelectFileDialog::MultiSelectFileDialog(QWidget *parent)
        : QFileDialog(parent) {
    implementMulitSelect();

    this->setNameFilter("SPM File(*.spm)");
}

void MultiSelectFileDialog::go() {
    QDialog::accept();
}

void MultiSelectFileDialog::implementMulitSelect() {
    this->setOption(QFileDialog::DontUseNativeDialog,true);

    QListView *listView = this->findChild<QListView*>("listView");
    if (listView) listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QTreeView *treeView = this->findChild<QTreeView*>();
    if (treeView) treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QDialogButtonBox *button = this->findChild<QDialogButtonBox *>("buttonBox");

    disconnect(button, SIGNAL(accepted()), this, SLOT(accept()));
    connect(button, SIGNAL(accepted()), this, SLOT(go()));
}
