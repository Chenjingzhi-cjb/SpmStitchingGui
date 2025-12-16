#ifndef SPM_STITCHING_HPP
#define SPM_STITCHING_HPP

#include "spm_algorithm.hpp"


class SpmStitching : public SpmRegexParse, StringOperations {
public:
    SpmStitching() = default;

    ~SpmStitching() = default;

public:
    bool loadSpmfromSpmPath(std::vector<std::string> &spm_path_list, const std::string &image_type,
                            std::vector<SpmReader> &spm_reader_list, std::vector<cv::Mat> &image_f1_list) {
        // 实例化 spm 对象，进行一阶拉平处理并保存图像
        spm_reader_list.clear();
        image_f1_list.clear();

        for (auto &i : spm_path_list) {
            SpmReader spm(i, image_type);
            if (!spm.readSpm()) {
                std::cout << "loadSpmfromSpmPath() [Error]: Failed to read SPM file: " << i << std::endl;
                return false;
            }

            auto &spm_image = spm.getImageSingle();
            SpmAlgorithm::flattenFirst(spm_image);
            cv::Mat image = SpmAlgorithm::spmImageToImage(spm_image);

            // add
            image_f1_list.emplace_back(image);
            spm_reader_list.emplace_back(std::move(spm));
        }

        return true;
    }

    std::vector<std::vector<double>> execStitchingImage(std::vector<cv::Mat> &image_f1_list,
                                                        cv::Mat *stitched_image = nullptr) {
        // 图像数据拼接
        int stitching_status;
        auto stitching_image_data = stitchingImage(image_f1_list, &stitching_status);
        if (stitching_status != 0) {
            std::cout << "execStitchingImage() [Error]: Image stitching failed! Status code: "
                      << stitching_status << std::endl;
            return {};
        }

        if (stitched_image) {
            cv::Mat _stitched_image = SpmAlgorithm::array2DToImage(stitching_image_data);
            cv::normalize(_stitched_image, _stitched_image, 255, 0, cv::NORM_MINMAX, CV_8U);
            _stitched_image.copyTo(*stitched_image);
        }

        return stitching_image_data;
    }

    bool execStitching(std::vector<SpmReader> &spm_reader_list,
                       std::vector<cv::Mat> &image_f1_list,
                       const std::string &output_spm_path,
                       cv::Mat *stitched_image = nullptr) {
        std::vector<std::vector<double>> stitching_image_data = execStitchingImage(image_f1_list, stitched_image);
        if (stitching_image_data.empty()) return false;

        // 计算新的 scan size
        auto &spm_image_first = spm_reader_list[0].getImageSingle();
        int new_scan_size = (int) ((double) stitching_image_data.size() * spm_image_first.getScanSize() / spm_image_first.getRows());

        // 计算新的 z scale 并 保留 7 位小数 + .1
        double z_scale = calcNewZScale(spm_reader_list[0], stitching_image_data) * 1.5;  // "x1.5" 以避免超量程
        z_scale = (std::round(z_scale * 10000000.0) + 1) / 10000000.0;
        std::string z_scale_str = doubleToDecimalString(z_scale, 7);

        // 基于拼图的 real data 计算 raw data 并变换为 byte data
        auto byte_data = calcRawDataToByteData(spm_reader_list[0], stitching_image_data, z_scale);

        // 构建文件头
        if (!buildOutputSpmHeader(spm_reader_list[0].getSpmPath(), output_spm_path,
                                  spm_reader_list[0].getImageTypeList()[0],
                                  (int) byte_data.size(), z_scale,
                                  (int) stitching_image_data[0].size(),
                                  (int) stitching_image_data.size(), new_scan_size)) {
            std::cout << "execStitching() [Error]: Failed to build output SPM header." << std::endl;
            return false;
        }

        // 文件头填充空数据
        if (!fillNullToHeader(output_spm_path)) return false;

        // 填充拼接后的图像数据 byte data
        if (!fillStitchedImageData(output_spm_path, byte_data)) return false;

        return true;
    }

private:
    static std::vector<std::vector<double>> stitchingImage(std::vector<cv::Mat> &image_f1_list, int *status = nullptr) {
        if (image_f1_list.empty()) {
            std::cout << "stitchingImage() [Error]: Input image list is empty." << std::endl;
            if (status) *status = -1;
            return {};
        }

        // 检查图像有效性
        for (size_t i = 0; i < image_f1_list.size(); ++i) {
            if (image_f1_list[i].empty()) {
                std::cout << "stitchingImage() [Error]: Image " << i << " is empty or invalid." << std::endl;
                if (status) *status = -2;
                return {};
            }
        }

        // 计算全局 min / max，以保持一致的缩放
        double global_min = DBL_MAX;
        double global_max = -DBL_MAX;
        for (const auto &img : image_f1_list) {
            double min_val, max_val;
            cv::minMaxLoc(img, &min_val, &max_val);
            global_min = cv::min(global_min, min_val);
            global_max = cv::max(global_max, max_val);
        }

        if (global_max - global_min < 1e-12) {
            std::cout << "stitchingImage() [Error]: Global min and max are too close — cannot normalize." << std::endl;
            if (status) *status = -3;
            return {};
        }

        // 归一化并转换为 3 通道图像
        std::vector<cv::Mat> image_u3_list;
        for (const auto &float_image : image_f1_list) {
            cv::Mat normalized, image_u1, image_u3;

            normalized = (float_image - global_min) / (global_max - global_min) * 255.0;  // 使用全局最小最大值进行归一化到 [0, 255]
            normalized.convertTo(image_u1, CV_8U);  // 转换为 8 位单通道
            cv::cvtColor(image_u1, image_u3, cv::COLOR_GRAY2BGR);  // 转换为 3 通道 BGR 图像

            image_u3_list.emplace_back(image_u3);
        }

        // 创建拼接器
        cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA);

        // 设置拼接参数（可选）
        // stitcher->setRegistrationResol(0.6);  // 配准分辨率
        // stitcher->setSeamEstimationResol(0.1); // 接缝估计分辨率
        // stitcher->setCompositingResol(-1);     // 合成分辨率，-1表示使用原始分辨率

        // 执行拼接
        cv::Mat pano;
        cv::Stitcher::Status stitching_status = stitcher->stitch(image_u3_list, pano);

        if (stitching_status != cv::Stitcher::Status::OK) {
            std::cout << "stitchingImage() [Error]: OpenCV stitching failed (code = "
                      << static_cast<int>(stitching_status) << ")." << std::endl;
            if (status) *status = -5;
            return {};
        }

        if (pano.empty() || pano.rows <= 0 || pano.cols <= 0) {
            std::cout << "stitchingImage() [Error]: Output panorama is empty after stitching." << std::endl;
            if (status) *status = -6;
            return {};
        }

        // 反向转换
        cv::cvtColor(pano, pano, cv::COLOR_BGR2GRAY);
        pano.convertTo(pano, CV_64F);
        pano = pano / 255.0 * (global_max - global_min) + global_min;

        // 结果图像的尺寸，应为 正方形 且为 64 的倍数
        int target_size = std::max(pano.rows, pano.cols);
        if (target_size % 64 != 0) target_size += 64 - (target_size % 64);

        // 初始化
        std::vector<std::vector<double>> stitching_image(target_size, std::vector<double>(target_size, global_min));
        for (int r = 0; r < pano.rows; ++r) {
            for (int c = 0; c < pano.cols; ++c) {
                stitching_image[r][c] = pano.at<double>(r, c);
            }
        }

        std::cout << "stitchingImage() [Info]: Stitching successful. Output size: "
                  << target_size << "x" << target_size << std::endl;

        if (status) *status = 0;
        return stitching_image;
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
    calcRawDataToByteData(SpmReader &spm_reader, std::vector<std::vector<double>> &stitching_image_data,
                          double z_scale) {
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
            if (line.substr(0, 11) == "\\Scan Size:") {
                replaceIntFromTextByRegex(R"(\Scan Size: (\d+) nm)", line, new_scan_size);
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
};


#endif //SPM_STITCHING_HPP
