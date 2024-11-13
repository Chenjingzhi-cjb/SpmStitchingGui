#ifndef SPM_STITCHING_HPP
#define SPM_STITCHING_HPP

#include "spm_algorithm.hpp"


class SpmStitching : public SpmRegexParse, StringOperations {
public:
    SpmStitching() = default;

    ~SpmStitching() = default;

public:
    std::vector<std::vector<double>> execStitchingPreview(std::vector<SpmReader> &spm_reader_list) {
        // 计算各图像对于上一幅图像的偏移量
        std::vector<std::pair<int, int>> image_xy_offset_info;
        for (int i = 0; i < spm_reader_list.size() - 1; ++i) {
            image_xy_offset_info.emplace_back(calcImageOffsetInfo(spm_reader_list[i], spm_reader_list[i + 1]));
        }

        // 计算各图像在最终图像中的位置信息
        std::vector<std::vector<int>> image_pos_info = calcImagePosInfo(
            spm_reader_list[0].getImageSingle().getCols(),
            spm_reader_list[0].getImageSingle().getRows(),
            image_xy_offset_info);

        // 拼图
        std::vector<std::vector<double>> stitching_image_data = stitchingImage(spm_reader_list, image_pos_info);

        m_stitched_image = SpmAlgorithm::array2DToImage(stitching_image_data);
        cv::normalize(m_stitched_image, m_stitched_image, 255, 0, cv::NORM_MINMAX, CV_8U);

        // 显示拼图结果
        //        cv::Mat image_stitching = SpmAlgorithm::array2DToImage(stitching_image_data);
        //        cv::normalize(image_stitching, image_stitching, 255, 0, cv::NORM_MINMAX, CV_8U);
        //
        //        namedWindow("img_stitching", cv::WINDOW_AUTOSIZE);
        //        imshow("img_stitching", image_stitching);
        //        cv::waitKey(0);

        return stitching_image_data;
    }

    void execStitching(std::vector<SpmReader> &spm_reader_list,
                       const std::string &tmpl_spm_path,
                       const std::string &output_spm_path,
                       const std::string &image_type) {
        std::vector<std::vector<double>> stitching_image_data = execStitchingPreview(spm_reader_list);

        // 计算新的 z scale 并 保留 7 位小数 + .1
        double z_scale = calcNewZScale(spm_reader_list[0], stitching_image_data) * 1.5;  // "x1.5" 以避免超量程
        z_scale = (std::round(z_scale * 10000000.0) + 1) / 10000000.0;
        std::string z_scale_str = doubleToDecimalString(z_scale, 7);

        // 基于拼图的 real data 计算 raw data 并变换为 byte data
        std::vector<char> byte_data = calcRawDataToByteData(spm_reader_list[0], stitching_image_data, z_scale);

        // 构建文件头
        if (!buildOutputSpmHeader(tmpl_spm_path, output_spm_path, image_type, (int) byte_data.size(), z_scale,
                                  (int) stitching_image_data[0].size(), (int) stitching_image_data.size(), 100))
            return;

        // 文件头填充空数据
        if (!fillNullToHeader(output_spm_path)) return;

        // 填充拼接后的图像数据 byte data
        if (!fillStitchedImageData(output_spm_path, byte_data)) return;
    }

    cv::Mat getStitchedImage() {
        return m_stitched_image;
    }

private:
    static std::pair<int, int> calcImageOffsetInfo(SpmReader &spm_tmpl, SpmReader &spm_offset) {
        // 提取图像数据并进行相关处理
        auto &spm_image_tmpl = spm_tmpl.getImageSingle();
        cv::Mat image_tmpl = SpmAlgorithm::spmImageToImage(spm_image_tmpl);
        cv::Mat image_tmpl_normal;
        cv::normalize(image_tmpl, image_tmpl_normal, 255, 0, cv::NORM_MINMAX, CV_8U);

        auto &spm_image_offset = spm_offset.getImageSingle();
        cv::Mat image_offset = SpmAlgorithm::spmImageToImage(spm_image_offset);
        cv::Mat image_offset_normal;
        cv::normalize(image_offset, image_offset_normal, 255, 0, cv::NORM_MINMAX, CV_8U);

        // 根据扫描偏移量计算模板匹配的相关参数
        int x_diff = (int) ((spm_offset.getEngageXPosNM() + spm_offset.getXOffsetNM()) -
                            (spm_tmpl.getEngageXPosNM() + spm_tmpl.getXOffsetNM()));
        int y_diff = (int) ((spm_offset.getEngageYPosNM() + spm_offset.getYOffsetNM()) -
                            (spm_tmpl.getEngageYPosNM() + spm_tmpl.getYOffsetNM()));

        int image_tmpl_sub_x_start, image_tmpl_sub_y_start, image_tmpl_sub_width, image_tmpl_sub_height;
        if (x_diff >= 0) {
            image_tmpl_sub_x_start = (int) (spm_image_offset.getCols() * 0.1);
            image_tmpl_sub_width = (int) ((1 - (double) x_diff / spm_image_offset.getScanSize() - 0.2) * spm_image_offset.getCols());
        } else {  // x_diff < 0
            image_tmpl_sub_x_start = (int) ((-1 * (double) x_diff / spm_image_offset.getScanSize() + 0.1) * spm_image_offset.getCols());
            image_tmpl_sub_width = (int) (spm_image_offset.getCols() - image_tmpl_sub_x_start - spm_image_offset.getCols() * 0.1);
        }
        if (y_diff >= 0) {
            image_tmpl_sub_y_start = (int) (((double) y_diff / spm_image_offset.getScanSize() + 0.1) * spm_image_offset.getRows());
            image_tmpl_sub_height = (int) (spm_image_offset.getRows() - image_tmpl_sub_y_start - spm_image_offset.getRows() * 0.1);
        } else {  // y_diff < 0
            image_tmpl_sub_y_start = (int) (spm_image_offset.getRows() * 0.1);
            image_tmpl_sub_height = (int) ((1 + (double) y_diff / spm_image_offset.getScanSize() - 0.2) * spm_image_offset.getRows());
        }

        // 模板匹配
        cv::Mat image_tmpl_sub = image_tmpl_normal(
                cv::Rect(image_tmpl_sub_x_start, image_tmpl_sub_y_start, image_tmpl_sub_width, image_tmpl_sub_height));
        std::pair<int, int> offset_value = SpmAlgorithm::calcMatchTemplate(image_tmpl_sub, image_offset);

        cv::Mat image_offset_sub = image_offset_normal(
                cv::Rect(offset_value.first, offset_value.second, image_tmpl_sub_width, image_tmpl_sub_height));

        // 显示匹配结果
//        cv::rectangle(image_tmpl_normal,
//                      cv::Rect(image_tmpl_sub_x_start, image_tmpl_sub_y_start, image_tmpl_sub_width,
//                               image_tmpl_sub_height),
//                      cv::Scalar(255), 2);
//        cv::rectangle(image_offset_normal, cv::Rect(offset_value.first, offset_value.second, image_tmpl_sub_width,
//                                             image_tmpl_sub_height), cv::Scalar(255), 2);
//
//        namedWindow("img_tmpl", cv::WINDOW_AUTOSIZE);
//        imshow("img_tmpl", image_tmpl_normal);
//        namedWindow("img_offset", cv::WINDOW_AUTOSIZE);
//        imshow("img_offset", image_offset_normal);
//        cv::waitKey(0);

        // 返回偏移结果
        return std::pair<int, int>{(int) std::round(image_tmpl_sub_x_start - offset_value.first),
                                   (int) std::round(image_tmpl_sub_y_start - offset_value.second)};
    }

    static std::vector<std::vector<int>>
    calcImagePosInfo(int image_width, int image_height, std::vector<std::pair<int, int>> &image_offset_info) {
        // {{all_x_start, all_x_end, all_y_start, all_y_end}, {first}, {second}, ...}
        std::vector<std::vector<int>> image_pos_info;

        image_pos_info.emplace_back(std::vector<int>{0, image_width, 0, image_height});  // all
        image_pos_info.emplace_back(std::vector<int>{0, image_width, 0, image_height});  // first

        for (int i = 0; i < image_offset_info.size(); ++i) {
            int current_x_start = image_pos_info[i + 1][0] + image_offset_info[i].first;
            int current_x_end = image_pos_info[i + 1][1] + image_offset_info[i].first;
            int current_y_start = image_pos_info[i + 1][2] + image_offset_info[i].second;
            int current_y_end = image_pos_info[i + 1][3] + image_offset_info[i].second;

            // change all
            if (current_x_start < image_pos_info[0][0]) {
                image_pos_info[0][0] = current_x_start;
            }
            if (current_x_end > image_pos_info[0][1]) {
                image_pos_info[0][1] = current_x_end;
            }
            if (current_y_start < image_pos_info[0][2]) {
                image_pos_info[0][2] = current_y_start;
            }
            if (current_y_end > image_pos_info[0][3]) {
                image_pos_info[0][3] = current_y_end;
            }

            image_pos_info.emplace_back(
                    std::vector<int>{current_x_start, current_x_end, current_y_start, current_y_end});  // add
        }

        return image_pos_info;
    }

    static std::vector<std::vector<double>>
    stitchingImage(std::vector<SpmReader> &spm_reader_list, std::vector<std::vector<int>> &image_pos_info) {
        // 结果图像的尺寸，应为 正方形 且为 64 的倍数
        int stitching_image_rows = image_pos_info[0][3] - image_pos_info[0][2];
        int stitching_image_cols = image_pos_info[0][1] - image_pos_info[0][0];
        if (stitching_image_rows < stitching_image_cols) {
            int col_add_num = 64 - stitching_image_cols % 64;
            image_pos_info[0][1] += col_add_num;
            stitching_image_cols += col_add_num;
            image_pos_info[0][3] += (stitching_image_cols - stitching_image_rows);
            stitching_image_rows = stitching_image_cols;
        } else {
            int row_add_num = 64 - stitching_image_rows % 64;
            image_pos_info[0][3] += row_add_num;
            stitching_image_rows += row_add_num;
            image_pos_info[0][1] += (stitching_image_rows - stitching_image_cols);
            stitching_image_cols = stitching_image_rows;
        }

        // 初始化
        std::vector<std::vector<double>> stitching_image_data(stitching_image_rows,
                                                              std::vector<double>(stitching_image_cols, 0));

        for (int index = 0; index < spm_reader_list.size(); ++index) {  // 图像层
            for (int r = 0; r < spm_reader_list[index].getImageSingle().getRows(); ++r) {
                for (int c = 0; c < spm_reader_list[index].getImageSingle().getCols(); ++c) {
                    double value = spm_reader_list[index].getImageRealDataSingle()[r][c];
                    int target_r = image_pos_info[index + 1][2] - image_pos_info[0][2] + r;
                    int target_c = image_pos_info[index + 1][0] - image_pos_info[0][0] + c;
                    if (stitching_image_data[target_r][target_c] == 0) {
                        stitching_image_data[target_r][target_c] = value;
                    }
//                    else {
//                        stitching_image_data[target_r][c] = (stitching_image_data[target_r][c] + value) / 2;
//                    }
                }
            }
        }

        return stitching_image_data;
    }

    static double calcNewZScale(SpmReader &spm_reader, std::vector<std::vector<double>> &stitching_image_data) {
        double min_value = stitching_image_data[0][0];
        double max_value = stitching_image_data[0][0];
        for (const auto &row : stitching_image_data) {
            for (double value : row) {
                if (value < min_value) min_value = value;
                if (value > max_value) max_value = value;
            }
        }

        double max_possible_value;
        if (spm_reader.getImageSingle().getBytesPerPixel() == 2)
            max_possible_value = std::numeric_limits<uint16_t>::max();
        else  // spm_reader.getImageSingle().getBytesPerPixel() == 4
            max_possible_value = std::numeric_limits<uint32_t>::max();

        double z_scale = ((max_value - min_value) * std::pow(2, 8 * spm_reader.getImageSingle().getBytesPerPixel())) /
                         (max_possible_value * spm_reader.getImageSingle().getZScaleSens());

        return z_scale;
    }

    static std::vector<char>
    calcRawDataToByteData(SpmReader &spm_reader, std::vector<std::vector<double>> &stitching_image_data, double z_scale) {
        double z_scale_sens = spm_reader.getImageSingle().getZScaleSens();
        int power_num = 8 * spm_reader.getImageSingle().getBytesPerPixel();

        std::vector<char> byte_data;
        if (spm_reader.getImageSingle().getBytesPerPixel() == 2) {
            std::vector<short> raw_data;

            std::reverse(stitching_image_data.begin(), stitching_image_data.end());
            for (auto &r : stitching_image_data) {
                for (double c : r) {
                    raw_data.emplace_back(static_cast<short>(c / z_scale_sens / z_scale * std::pow(2, power_num)));
                }
            }

            byte_data.assign(reinterpret_cast<const char *>(raw_data.data()),
                             reinterpret_cast<const char *>(raw_data.data() + raw_data.size()));
        } else {  // spm_reader.getImageSingle().getBytesPerPixel() == 4
            std::vector<int> raw_data;

            std::reverse(stitching_image_data.begin(), stitching_image_data.end());
            for (auto &r : stitching_image_data) {
                for (double c : r) {
                    raw_data.emplace_back(static_cast<int>(c / z_scale_sens / z_scale * std::pow(2, power_num)));
                }
            }

            byte_data.assign(reinterpret_cast<const char *>(raw_data.data()),
                             reinterpret_cast<const char *>(raw_data.data() + raw_data.size()));
        }

        return byte_data;
    }

    bool buildOutputSpmHeader(const std::string &tmpl_spm_path, const std::string &output_spm_path,
                              const std::string &image_type,
                              int new_data_length, double new_z_scale, int new_samps_line, int new_number_of_lines,
                              int new_scan_size) {
        // calc index
#ifdef _MSC_VER
        FILE *spm_file_index = nullptr;
        errno_t err_index = _wfopen_s(&spm_file_index, string2wstring(tmpl_spm_path).c_str(), L"r");
#else
        FILE *spm_file_index = _wfopen(string2wstring(m_spm_path).c_str(), L"r");
#endif

        if (!spm_file_index) {
            std::cout << "Failed to open SPM file: " << tmpl_spm_path << std::endl;
            return {};
        }

        int image_index = 0;
        std::string text_index, line_index;
        std::wstring line_w_index;
        wchar_t buffer_index[1024];
        while (fgetws(buffer_index, sizeof(buffer_index) / sizeof(buffer_index[0]), spm_file_index)) {
            line_w_index = buffer_index;
            line_index = wstring2string(line_w_index);
            line_index.assign(line_index.begin(), line_index.end() - 1);  // 去除 '\n'
            if (line_index.substr(0, 2) == "\\*") {
                if (line_index == "\\*File list end") {  // end, last Image list
                    break;
                }
                if (line_index == "\\*Ciao image list") {
                    if (image_index == 0) {  // Head list
                        image_index++;
                    } else {  // Image list
                        std::string this_image_type = getStringFromTextByRegex(R"(\@2:Image Data: S \[.*?\] \"(.*?)\")",
                                                                               text_index);
                        if (this_image_type == image_type) {  // 是需要的图像
                            break;
                        }
                        image_index++;
                    }
                    text_index.clear();
                }
            }
            text_index.append(line_index);
            text_index.append("\n");
        }

        fclose(spm_file_index);

        // write
#ifdef _MSC_VER
        FILE *spm_file = nullptr;
        errno_t err = _wfopen_s(&spm_file, string2wstring(tmpl_spm_path).c_str(), L"r");
#else
        FILE *spm_file = _wfopen(string2wstring(tmpl_spm_path).c_str(), L"r");
#endif

        if (!spm_file) {
            std::cout << "Failed to open SPM file: " << tmpl_spm_path << std::endl;
            return false;
        }

        std::wofstream out_spm_file(output_spm_path);

        std::string text, line;
        std::wstring text_w, line_w;
        wchar_t buffer[1024];
        int image_num = 0;
        while (fgetws(buffer, sizeof(buffer) / sizeof(buffer[0]), spm_file)) {
            line_w = buffer;
            line = wstring2string(line_w);

            if (line == "\\*Ciao image list\n" || line == "\\*File list end\n") {
                if (image_num == 0) {  // head
                    out_spm_file << text_w;
                    m_data_length = getIntFromTextByRegex(R"(\Data length: (\d+))", text);
                } else if (image_num == image_index) {  // image
                    out_spm_file << text_w;
                    break;
                }

                image_num += 1;
                text.clear();
                text_w.clear();
            }

            if (image_num == image_index && line.substr(0, 13) == "\\Data length:") {
                replaceIntFromTextByRegex(R"(\Data length: (\d+))", line, new_data_length);
                line_w = string2wstring(line);
            }
            if (line.substr(0, 32) == "\\@2:Z scale: V [Sens. ZsensSens]") {
                replaceDoubleFromTextByRegex(R"(\@2:Z scale: V \[.*?\] .*? (\d+\.\d+) .*)", line, new_z_scale);
                line_w = string2wstring(line);
            }
            if (image_num == image_index && line.substr(0, 12) == "\\Samps/line:") {
                replaceIntFromTextByRegex(R"(\Samps/line: (\d+))", line, new_samps_line);
                line_w = string2wstring(line);
            }
            if (image_num == image_index && line.substr(0, 17) == "\\Number of lines:") {
                replaceIntFromTextByRegex(R"(\Number of lines: (\d+))", line, new_number_of_lines);
                line_w = string2wstring(line);
            }
            if (image_num == image_index && line.substr(0, 18) == "\\Valid data len X:") {
                replaceIntFromTextByRegex(R"(\Valid data len X: (\d+))", line, new_samps_line);
                line_w = string2wstring(line);
            }
            if (image_num == image_index && line.substr(0, 18) == "\\Valid data len Y:") {
                replaceIntFromTextByRegex(R"(\Valid data len Y: (\d+))", line, new_number_of_lines);
                line_w = string2wstring(line);
            }

            text.append(line);
            text_w.append(line_w);
        }
        out_spm_file << L"\\*File list end\n";

        fclose(spm_file);
        out_spm_file.close();

        return true;
    }

    bool fillNullToHeader(const std::string &output_spm_path) const {
        // 获取输出文件的占用空间
        std::ifstream output_file(output_spm_path, std::ios::binary | std::ios::ate);
        if (!output_file.is_open()) {
            std::cout << "Failed to open output file: " << output_spm_path << std::endl;
            return false;
        }

        std::streamsize current_size = output_file.tellg();  // 获取当前文件大小
        output_file.close();

        // 计算需要填充的字节数
        if (current_size < m_data_length) {
            std::ofstream output_file_append(output_spm_path, std::ios::binary | std::ios::app);
            if (!output_file_append.is_open()) {
                std::cerr << "Failed to open output file: " << output_spm_path << std::endl;
                return false;
            }

            // 填充一个 '0x1A'
            char fill_byte = '\x1A';
            output_file_append.write(&fill_byte, sizeof(fill_byte));

            // 使用 '0x00' 填充直到达到所需大小
            std::streamsize bytes_to_append = m_data_length - current_size - 1;
            char zero_byte = '\0';
            for (std::streamsize i = 0; i < bytes_to_append; ++i) {
                output_file_append.write(&zero_byte, sizeof(zero_byte));
            }

            output_file_append.close();
        }

        return true;
    }

    static bool fillStitchedImageData(const std::string &output_spm_path, std::vector<char> &byte_data) {
        std::ofstream output_file_append(output_spm_path, std::ios::binary | std::ios::app);
        if (!output_file_append.is_open()) {
            std::cerr << "Failed to open output file: " << output_spm_path << std::endl;
            return false;
        }

        for (char a_byte : byte_data) {
            output_file_append.write(&a_byte, sizeof(a_byte));
        }

        output_file_append.close();

        return true;
    }

private:
    int m_data_length{};

    cv::Mat m_stitched_image;
};


#endif //SPM_STITCHING_HPP
