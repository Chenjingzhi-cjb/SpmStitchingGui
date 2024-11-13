#ifndef MULTI_SELECT_FILE_DIALOG_H
#define MULTI_SELECT_FILE_DIALOG_H

#include <QFileDialog>
#include <QWidget>
#include <QListView>
#include <QTreeView>
#include <QDialogButtonBox>


class MultiSelectFileDialog : public QFileDialog {
    Q_OBJECT

public:
    explicit MultiSelectFileDialog(QWidget *parent = 0);

public slots:
    void go();

private:
    void implementMulitSelect();
};


#endif // MULTI_SELECT_FILE_DIALOG_H
