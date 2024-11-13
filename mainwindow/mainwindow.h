#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRegularExpressionValidator>

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <chrono>
#include <ctime>

#include "multi_select_file_dialog.h"
#include "spm_stitching.hpp"


QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void printLog(const QString &text, const QString &level);

    void on_btn_add_clicked();

    void on_btn_preview_clicked();

    void on_btn_save_clicked();

    void on_btn_sub_clicked();

    void on_btn_up_clicked();

    void on_btn_down_clicked();

private:
    void updateListWidgetFiles();

    void updatePreviewImage();

    void reloadSpmReaderList();

    void slashLeftToRight(QString &str);

    std::string getTime();

private:
    Ui::MainWindow *ui;
    MultiSelectFileDialog *m_path_select;

    std::vector<std::string> m_spm_path_list;
    std::vector<SpmReader> m_spm_reader_list;
    std::vector<std::pair<int, int>> m_spm_offset_nm_list;
    cv::Mat m_preview_image;
};


#endif // MAINWINDOW_H
