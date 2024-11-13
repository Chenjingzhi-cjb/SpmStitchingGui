#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent),
          ui(new Ui::MainWindow),
          m_path_select(new MultiSelectFileDialog) {
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/ui/resource/icon.ico"));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::printLog(const QString &text, const QString &level) {
    QString time = QString::fromStdString("[ " + getTime() + " ] ");

    if (level == "info") ui->textBrowser_log->setTextColor(Qt::black);
    else if (level == "error") ui->textBrowser_log->setTextColor(Qt::red);

    ui->textBrowser_log->append(time + text);
}

void MainWindow::on_btn_add_clicked() {
    if(m_path_select->exec()==QDialog::Accepted) {
        QStringList paths = m_path_select->selectedFiles();

        if (!paths.isEmpty()) {
            for (int i = 0; i < paths.size(); i++) {
                slashLeftToRight(paths[i]);

                SpmReader spm(paths[i].toStdString(), ui->comboBox_spm_type->currentText().toStdString());
                if (!spm.readSpm()) {
                    std::cout << "Reading spm file error!" << std::endl;
                    printLog("Reading spm file \"" + paths[i] + "\" error!", "error");
                    continue;
                }
                SpmAlgorithm::flattenFirst(spm.getImageSingle());

                // add spm offset nm
                m_spm_offset_nm_list.emplace_back(std::pair<int, int>{spm.getXOffsetNM(), spm.getYOffsetNM()});

                // add spm reader
                m_spm_reader_list.emplace_back(std::move(spm));

                // add spm path
                m_spm_path_list.emplace_back(paths[i].toStdString());
            }

            updateListWidgetFiles();
            printLog("Adding files completed!", "info");
        }
    }
}

void MainWindow::on_btn_sub_clicked() {
    int current_row = ui->listWidget_files->currentRow();

    if (current_row < 0) return;

    m_spm_path_list.erase(m_spm_path_list.begin() + current_row);

    reloadSpmReaderList();

    updateListWidgetFiles();
}

void MainWindow::on_btn_up_clicked() {
    int current_row = ui->listWidget_files->currentRow();

    if (current_row <= 0) return;

    std::swap(m_spm_path_list[current_row], m_spm_path_list[current_row - 1]);

    reloadSpmReaderList();

    updateListWidgetFiles();
}

void MainWindow::on_btn_down_clicked() {
    int current_row = ui->listWidget_files->currentRow();

    if (current_row < 0 || current_row == ui->listWidget_files->count() - 1) return;

    std::swap(m_spm_path_list[current_row], m_spm_path_list[current_row + 1]);

    reloadSpmReaderList();

    updateListWidgetFiles();
}

void MainWindow::on_btn_preview_clicked() {
    SpmStitching stitching;
    stitching.execStitchingPreview(m_spm_reader_list);

    cv::Mat stitched_image = stitching.getStitchedImage();
    applyColorMap(stitched_image, m_preview_image, cv::COLORMAP_PLASMA);

    updatePreviewImage();
    printLog("Previewed stitched image successfully!", "info");
}

void MainWindow::on_btn_save_clicked() {
    QString save_file = QFileDialog::getSaveFileName(this, tr("Save SPM File"), "E:/", tr("*.spm"));
    if(save_file.isNull()) {
        return;
    }
    slashLeftToRight(save_file);

    SpmStitching stitching;
    stitching.execStitching(m_spm_reader_list,
                            m_spm_path_list[0],
                            save_file.toStdString(),
                            ui->comboBox_spm_type->currentText().toStdString());

    cv::Mat stitched_image = stitching.getStitchedImage();
    applyColorMap(stitched_image, m_preview_image, cv::COLORMAP_PLASMA);

    updatePreviewImage();
    printLog("Saved stitched image to \"" + save_file + "\" successfully!", "info");
}

void MainWindow::updateListWidgetFiles() {
    ui->listWidget_files->clear();

    std::vector<std::string> spm_info_list;
    for (int i = 0; i < m_spm_path_list.size(); i++) {
        std::string spm_info = "(" + std::to_string(m_spm_offset_nm_list[i].first)
                               + ", " + std::to_string(m_spm_offset_nm_list[i].second) + ")";
        spm_info_list.emplace_back(spm_info);
    }

    int spm_info_first_length_max = 0;
    for (int i = 0; i < m_spm_path_list.size(); i++) {
        if (spm_info_list[i].size() > spm_info_first_length_max)
            spm_info_first_length_max = spm_info_list[i].size();
    }

    for (int i = 0; i < m_spm_path_list.size(); i++) {
        for (int n = spm_info_list[i].size(); n < spm_info_first_length_max; n++)
            spm_info_list[i] += " ";

        QFileInfo fileInfo(QString::fromStdString(m_spm_path_list[i]));
        QString fileName = fileInfo.fileName();

        spm_info_list[i] += "  |  " + fileName.toStdString();

        ui->listWidget_files->addItem(QString::fromStdString(spm_info_list[i]));
    }
}

void MainWindow::updatePreviewImage() {
    cv::resize(m_preview_image, m_preview_image, cv::Size(ui->label_preview_image->width(), ui->label_preview_image->height()));
    QImage qimage = QImage(m_preview_image.data, m_preview_image.cols, m_preview_image.rows, m_preview_image.step, QImage::Format_BGR888);
    QPixmap qpixmap = QPixmap::fromImage(qimage);
    ui->label_preview_image->setPixmap(qpixmap);
}

void MainWindow::reloadSpmReaderList() {
    m_spm_offset_nm_list.clear();
    m_spm_reader_list.clear();
    for (int i = 0; i < m_spm_path_list.size(); i++) {
        SpmReader spm(m_spm_path_list[i], ui->comboBox_spm_type->currentText().toStdString());
        if (!spm.readSpm()) {
            std::cout << "Reading spm file error!" << std::endl;
            continue;
        }
        SpmAlgorithm::flattenFirst(spm.getImageSingle());

        // add spm offset nm
        m_spm_offset_nm_list.emplace_back(std::pair<int, int>{spm.getXOffsetNM(), spm.getYOffsetNM()});

        // add spm reader
        m_spm_reader_list.emplace_back(std::move(spm));
    }
}

void MainWindow::slashLeftToRight(QString &str) {
    QString temp = "";

    for (auto &i : str) {
        if (i == '/') {
            temp += '\\';
        } else {
            temp += i;
        }
    }

    str = std::move(temp);
}

std::string MainWindow::getTime() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 将时间点转换为 time_t 以便进行时间格式化
    std::time_t time_t_seconds = std::chrono::system_clock::to_time_t(now);

    // 将 time_t 转换为本地时间
    std::tm tm_buffer;
    localtime_s(&tm_buffer, &time_t_seconds);  // 线程安全

    // 使用 ostringstream 来构建字符串
    std::ostringstream oss;
    oss << std::put_time(&tm_buffer, "%a %b %d %H:%M:%S %Y");

    // 返回格式化的时间字符串
    return oss.str();
}

