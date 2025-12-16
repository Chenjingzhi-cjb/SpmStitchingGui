#ifndef SPM_READER_HPP
#define SPM_READER_HPP

#include <iostream>
#include <fstream>
#include <windows.h>
#include <regex>
#include <cmath>
#include <utility>
#include <string>
#include <vector>
#include <unordered_map>


class StringOperations {
protected:
    /**
     * @brief string 转 wstring
     *
     * @param str src string
     * @param CodePage The encoding format of the file calling this function. CP_ACP for gbk, CP_UTF8 for utf-8.
     * @return dst string
     */
    static std::wstring string2wstring(const std::string &str, _In_ UINT CodePage = CP_ACP) {
        int len = MultiByteToWideChar(CodePage, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(len, L'\0');
        MultiByteToWideChar(CodePage, 0, str.c_str(), -1, const_cast<wchar_t *>(wstr.data()), len);
        wstr.resize(wcslen(wstr.c_str()));
        return wstr;
    }

    /**
     * @brief wstring 转 string
     *
     * @param wstr src wstring
     * @param CodePage The encoding format of the file calling this function. CP_ACP for gbk, CP_UTF8 for utf-8.
     * @return dst string
     */
    static std::string wstring2string(const std::wstring &wstr, _In_ UINT CodePage = CP_ACP) {
        int len = WideCharToMultiByte(CodePage, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(len, '\0');
        WideCharToMultiByte(CodePage, 0, wstr.c_str(), -1, const_cast<char *>(str.data()), len, nullptr, nullptr);
        str.resize(strlen(str.c_str()));
        return str;
    }

    static std::string doubleToDecimalString(double value, int decimal_num) {
        double temp = value;
        int digits = 0;
        int sign_bit = value >= 0 ? 1 : -1;

        if (sign_bit == 1) {
            while (temp >= sign_bit) {
                digits += 1;
                temp /= 10;
            }

            if (digits == 0) digits = 1;
        } else {
            digits += 1;

            while (temp <= sign_bit) {
                digits += 1;
                temp /= 10;
            }

            if (digits == 1) digits = 2;
        }

        return std::to_string(value).substr(0, digits + decimal_num + 1);
    }
};


class SpmRegexParse {
public:
    SpmRegexParse() = default;

    ~SpmRegexParse() = default;

protected:
    static int getIntFromTextByRegex(const std::string &regex_string, std::string &spm_file_text) {
        std::regex pattern(regex_string);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 2) {
            return std::stoi(matches[1].str());
        } else {
            return 0;
        }
    }

    static double getDoubleFromTextByRegex(const std::string &regex_string, std::string &spm_file_text) {
        std::regex pattern(regex_string);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 2) {
            return std::stod(matches[1].str());
        } else {
            return 0;
        }
    }

    static std::string getStringFromTextByRegex(const std::string &regex_string, std::string &spm_file_text) {
        std::regex pattern(regex_string);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 2) {
            return matches[1].str();
        } else {
            return "";
        }
    }

    static int getIntFromTextByRegexToNM(const std::string &regex_string, std::string &spm_file_text) {
        std::regex pattern(regex_string);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 3) {
            if (matches[2].str() == "nm") {
                return std::stoi(matches[1].str());
            } else if (matches[2].str() == "um") {
                return (int) (std::stod(matches[1].str()) * 1000);
            } else {  // error
                return 0;
            }
        } else {
            return 0;
        }
    }

    static long long getLongLongFromTextByRegexToNM(const std::string &regex_string, std::string &spm_file_text) {
        std::regex pattern(regex_string);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 3) {
            if (matches[2].str() == "nm") {
                return std::stoll(matches[1].str());
            } else if (matches[2].str() == "um") {
                return (long long) (std::stod(matches[1].str()) * 1000);
            } else if (matches[2].str() == "mm") {
                return (long long) (std::stoll(matches[1].str()) * 1000 * 1000);
            } else {  // error
                return 0;
            }
        } else {
            return 0;
        }
    }

    static bool replaceIntFromTextByRegex(const std::string &regex_string, std::string &spm_file_text, int new_value) {
        std::regex pattern(regex_string);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 2) {
            auto match_str_begin_pos = spm_file_text.find(matches[0].str());
            std::string old_value_str = matches[1].str();
            auto old_value_str_begin_pos = spm_file_text.find(old_value_str, match_str_begin_pos);
            spm_file_text = spm_file_text.substr(0, old_value_str_begin_pos) + std::to_string(new_value) +
                            spm_file_text.substr(old_value_str_begin_pos + old_value_str.size());
            return true;
        } else {
            return false;
        }
    }

    static double
    replaceDoubleFromTextByRegex(const std::string &regex_string, std::string &spm_file_text, double new_value) {
        std::regex pattern(regex_string);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 2) {
            auto match_str_begin_pos = spm_file_text.find(matches[0].str());
            std::string old_value_str = matches[1].str();
            auto old_value_str_begin_pos = spm_file_text.find(old_value_str, match_str_begin_pos);
            spm_file_text = spm_file_text.substr(0, old_value_str_begin_pos) + std::to_string(new_value) +
                            spm_file_text.substr(old_value_str_begin_pos + old_value_str.size());
            return true;
        } else {
            return false;
        }
    }
};


class SpmImage : public SpmRegexParse {
public:
    explicit SpmImage(int scan_size)
            : m_scan_size(scan_size) {}

    ~SpmImage() = default;

public:
    void parseImageAttributes(std::string &spm_file_text) {
        // Can be modified: 可添加需要的属性
        m_data_length = getIntFromTextByRegex(data_length_regex, spm_file_text);
        m_data_offset = getIntFromTextByRegex(data_offset_regex, spm_file_text);
        m_bytes_per_pixel = getIntFromTextByRegex(bytes_per_pixel_regex, spm_file_text);
        m_frame_direction = getStringFromTextByRegex(frame_direction_regex, spm_file_text);
        m_capture_start_line = getIntFromTextByRegex(capture_start_line_regex, spm_file_text);
        m_color_table_index = getIntFromTextByRegex(color_table_index_regex, spm_file_text);
        m_relative_frame_time = getDoubleFromTextByRegex(relative_frame_time_regex, spm_file_text);
        m_samps_per_line = getIntFromTextByRegex(samps_per_line_regex, spm_file_text);
        m_number_of_lines = getIntFromTextByRegex(number_of_lines_regex, spm_file_text);
    }

    void setZScale(std::string &spm_file_text, std::string &head_text) {
        std::pair<double, std::string> z_scale_info = getZScaleInfoFromTextByRegex(spm_file_text);

        m_z_scale = z_scale_info.first;

        std::string z_scale_sens_regex = R"(\@)" + z_scale_info.second + R"(: V (\d+(\.\d+)?) .*)";
        m_z_scale_sens = getDoubleFromTextByRegex(z_scale_sens_regex, head_text);
    }

    bool setImageData(std::vector<char> &byte_data) {
        // set raw data
        if (m_bytes_per_pixel == 2) {
            std::vector<short> raw_data_16;
            raw_data_16.assign(reinterpret_cast<const short *>(byte_data.data()),
                               reinterpret_cast<const short *>(byte_data.data() + byte_data.size()));
            m_raw_data.assign(raw_data_16.begin(), raw_data_16.end());
        } else if (m_bytes_per_pixel == 4) {
            m_raw_data.assign(reinterpret_cast<const int *>(byte_data.data()),
                              reinterpret_cast<const int *>(byte_data.data() + byte_data.size()));
        } else {
            return false;
        }

        // calc real data
        for (int r = (int) (m_number_of_lines - 1); r >= 0; r--) {
            std::vector<double> line_data;
            line_data.reserve(m_samps_per_line);
            for (int c = 0; c < m_samps_per_line; c++) {
                line_data.emplace_back(
                        m_raw_data[r * m_samps_per_line + c] * m_z_scale_sens * m_z_scale /
                        std::pow(2, 8 * m_bytes_per_pixel));
            }
            m_real_data.emplace_back(line_data);
        }

        return true;
    }

    unsigned int getDataLength() const { return m_data_length; }

    unsigned int getDataOffset() const { return m_data_offset; }

    std::vector<int> &getRawData() { return m_raw_data; }

    std::vector<std::vector<double>> &getRealData() { return m_real_data; }

    int getRows() const { return (int) m_number_of_lines; }

    int getCols() const { return (int) m_samps_per_line; }

    int getBytesPerPixel() const { return (int) m_bytes_per_pixel; }

    int getScanSize() const { return m_scan_size; }

    double getZScaleSens() const { return m_z_scale_sens; }

private:
    static std::pair<double, std::string> getZScaleInfoFromTextByRegex(std::string &spm_file_text) {
        std::string z_scale_regex = R"(\@2:Z scale: V \[(.*?)\] \(.*?\) (\d+\.\d+) (.*))";
        std::regex pattern(z_scale_regex);
        std::smatch matches;
        if (std::regex_search(spm_file_text, matches, pattern) && matches.size() >= 4) {
            double value = std::stod(matches[2].str());

            std::string unit = matches[3].str();
            if (unit == "mV") value /= 1000;  // convert mV to V uniformly

            return {value, matches[1].str()};
        } else {
            return {0, ""};
        }
    }

public:
    // TODO: 1. 完善 Image Type
    enum class ImageType {
        Height,
        HeightSensor,
        HeightTrace,
        HeightRetrace,
        AmplitudeError,
        All
    };
    static const std::vector<std::string> image_type_str;

    // Can be modified: 可添加需要的属性正则表达式
    const std::string data_length_regex = R"(\Data length: (\d+))";
    const std::string data_offset_regex = R"(\Data offset: (\d+))";
    const std::string bytes_per_pixel_regex = R"(\Bytes/pixel: ([24]))";
    const std::string frame_direction_regex = R"(\Frame direction: ([A-Za-z]+))";
    const std::string capture_start_line_regex = R"(\Capture start line: (\d+))";
    const std::string color_table_index_regex = R"(\Color Table Index: (\d+))";
    const std::string relative_frame_time_regex = R"(\Relative frame time: (\d+(\.\d+)?))";
    const std::string samps_per_line_regex = R"(\Samps/line: (\d+))";
    const std::string number_of_lines_regex = R"(\Number of lines: (\d+))";

private:
    // Image attributes
    // Can be modified: 可添加需要的属性
    unsigned int m_data_length{};
    unsigned int m_data_offset{};
    unsigned int m_bytes_per_pixel{};
    std::string m_frame_direction;
    unsigned int m_capture_start_line{};
    unsigned int m_color_table_index{};
    double m_relative_frame_time{};
    unsigned int m_samps_per_line{};
    unsigned int m_number_of_lines{};

    int m_scan_size{};
    double m_z_scale{};
    double m_z_scale_sens{};

    // Image data
    std::vector<int> m_raw_data;  // uniformly converted to 4 bytes (int)
    std::vector<std::vector<double>> m_real_data;
};


class SpmReader : public SpmRegexParse, StringOperations {
public:
    SpmReader(std::string spm_path, const std::string &image_type)
            : m_spm_path(std::move(spm_path)) {
        m_image_type_list.emplace_back(image_type);
    }

    explicit SpmReader(std::string spm_path, const SpmImage::ImageType &image_type = SpmImage::ImageType::HeightSensor)
            : m_spm_path(std::move(spm_path)) {
        if (image_type == SpmImage::ImageType::All) {
            m_image_type_list.assign(SpmImage::image_type_str.begin(), SpmImage::image_type_str.end());
        } else if ((int) image_type >= 0 && (int) image_type < SpmImage::image_type_str.size()) {
            m_image_type_list.emplace_back(SpmImage::image_type_str[(int) image_type]);
        }
    }

    SpmReader(std::string spm_path, std::vector<std::string> image_type_list)
            : m_spm_path(std::move(spm_path)),
              m_image_type_list(std::move(image_type_list)) {}

    SpmReader(std::string spm_path, const std::vector<SpmImage::ImageType> &image_type_list)
            : m_spm_path(std::move(spm_path)) {
        for (auto &image_type: image_type_list) {
            if ((int) image_type < 0 || (int) image_type >= SpmImage::image_type_str.size()) continue;
            m_image_type_list.emplace_back(SpmImage::image_type_str[(int) image_type]);
        }
    }

    ~SpmReader() = default;

public:
    std::string getSpmPath() const { return m_spm_path; }

    std::vector<std::string> getImageTypeList() const { return m_image_type_list; }

    bool readSpm() {
        if (m_spm_path.empty() || m_image_type_list.empty()) return false;

        // spm file text structure: Head list + Image list x n
        std::unordered_map<std::string, std::string> spm_file_text_map = loadSpmFileTextMap();
        if (spm_file_text_map.size() < 2) {
            return false;
        }

        // Parse SPM file text to file head attributes
        parseFileHeadAttributes(spm_file_text_map.at("Head"));

        // Parse SPM file text to image attributes and load SPM image data
        for (auto &spm_file_text: spm_file_text_map) {
            if (spm_file_text.first != "Head") {
                SpmImage spm_image((int) m_scan_size);
                spm_image.parseImageAttributes(spm_file_text.second);
                spm_image.setZScale(spm_file_text.second, spm_file_text_map.at("Head"));
                std::vector<char> byte_data = loadSpmImageData(spm_image);
                bool status = spm_image.setImageData(byte_data);
                if (!status) return false;

                m_image_list.emplace(spm_file_text.first, std::move(spm_image));
            }
        }

        if (m_image_list.empty()) return false;

        return true;
    }

    // Suitable for single channel
    bool isImageAvailableSingle() {
        return m_image_list.begin() != m_image_list.end();
    }

    bool isImageAvailable(const std::string &image_type) {
        return m_image_list.find(image_type) != m_image_list.end();
    }

    bool isImageAvailable(const SpmImage::ImageType &image_type) {
        return m_image_list.find(SpmImage::image_type_str[(int) image_type]) != m_image_list.end();
    }

    // Suitable for single channel
    SpmImage &getImageSingle() {
        return m_image_list.begin()->second;
    }

    SpmImage &getImage(const std::string &image_type) {
        return m_image_list.at(image_type);
    }

    SpmImage &getImage(const SpmImage::ImageType &image_type) {
        return m_image_list.at(SpmImage::image_type_str[(int) image_type]);
    }

    // Suitable for single channel
    std::vector<std::vector<double>> &getImageRealDataSingle() {
        return m_image_list.begin()->second.getRealData();
    }

    std::vector<std::vector<double>> &getImageRealData(const std::string &image_type) {
        return m_image_list.at(image_type).getRealData();
    }

    std::vector<std::vector<double>> &getImageRealData(const SpmImage::ImageType &image_type) {
        return m_image_list.at(SpmImage::image_type_str[(int) image_type]).getRealData();
    }

    long long getEngageXPosNM() const { return m_engage_x_pos_nm; }

    long long getEngageYPosNM() const { return m_engage_y_pos_nm; }

    int getXOffsetNM() const { return m_x_offset_nm; }

    int getYOffsetNM() const { return m_y_offset_nm; }

private:
    std::unordered_map<std::string, std::string> loadSpmFileTextMap() {

#ifdef _MSC_VER
        FILE *spm_file = nullptr;
        errno_t err = _wfopen_s(&spm_file, string2wstring(m_spm_path).c_str(), L"r");
#else
        FILE *spm_file = _wfopen(string2wstring(m_spm_path).c_str(), L"r");
#endif

        if (!spm_file) {
            std::cout << "Failed to open SPM file: " << m_spm_path << std::endl;
            return {};
        }

        std::unordered_map<std::string, std::string> text_map;
        std::string text, line;
        std::wstring line_w;
        wchar_t buffer[1024];
        while (fgetws(buffer, sizeof(buffer) / sizeof(buffer[0]), spm_file)) {
            line_w = buffer;
            line = wstring2string(line_w);
            line.assign(line.begin(), line.end() - 1);  // 去除 '\n'
            if (line.substr(0, 2) == "\\*") {
                if (line == m_file_list_end_str) {  // end, last Image list
                    std::string image_type = getStringFromTextByRegex(image_type_regex, text);
                    auto image_type_it = std::find(m_image_type_list.begin(), m_image_type_list.end(), image_type);
                    if (image_type_it != m_image_type_list.end()) {  // 是需要的图像
                        text_map.emplace(image_type, text);
                    }
                    break;
                }
                if (line == m_ciao_image_list_str) {
                    if (text_map.empty()) {  // Head list
                        text_map.emplace("Head", text);
                    } else {  // !text_map.empty(), Image list
                        std::string image_type = getStringFromTextByRegex(image_type_regex, text);
                        auto image_type_it = std::find(m_image_type_list.begin(), m_image_type_list.end(), image_type);
                        if (image_type_it != m_image_type_list.end()) {  // 是需要的图像
                            text_map.emplace(image_type, text);
                        }
                    }
                    text.clear();
                }
            }
            text.append(line);
            text.append("\n");
        }

        fclose(spm_file);

        return text_map;
    }

    std::vector<char> loadSpmImageData(SpmImage &spm_image) {

#ifdef _MSC_VER
        FILE *spm_file = nullptr;
        errno_t err = _wfopen_s(&spm_file, string2wstring(m_spm_path).c_str(), L"rb");
#else
        FILE *spm_file = _wfopen(string2wstring(m_spm_path).c_str(), L"rb");
#endif

        if (!spm_file) {
            std::cout << "Failed to open SPM file: " << m_spm_path << std::endl;
            return {};
        }

        std::vector<char> jump_over(spm_image.getDataOffset());
        fread(jump_over.data(), 1, spm_image.getDataOffset(), spm_file);

        std::vector<char> image_data(spm_image.getDataLength());
        fread(image_data.data(), 1, spm_image.getDataLength(), spm_file);

        fclose(spm_file);

        return image_data;
    }

    void parseFileHeadAttributes(std::string &spm_file_text) {
        // Can be modified: 可添加需要的属性
        m_scan_size = getIntFromTextByRegex(scan_size_regex, spm_file_text);
        m_engage_x_pos_nm = getLongLongFromTextByRegexToNM(engage_x_pos_regex, spm_file_text);
        m_engage_y_pos_nm = getLongLongFromTextByRegexToNM(engage_y_pos_regex, spm_file_text);
        m_x_offset_nm = getIntFromTextByRegex(x_offset_regex, spm_file_text);
        m_y_offset_nm = getIntFromTextByRegex(y_offset_regex, spm_file_text);
    }

public:
    // Can be modified: 可添加需要的属性正则表达式
    const std::string image_type_regex = R"(\@2:Image Data: S \[.*?\] \"(.*?)\")";
    const std::string scan_size_regex = R"(\Scan Size: (\d+(\.\d+)?) nm)";
    const std::string engage_x_pos_regex = R"(\\Engage X Pos: ([0-9.-]*) ([num]*))";
    const std::string engage_y_pos_regex = R"(\\Engage Y Pos: ([0-9.-]*) ([num]*))";
    const std::string x_offset_regex = R"(\\X Offset: ([0-9.-]*) ([num]*))";
    const std::string y_offset_regex = R"(\\Y Offset: ([0-9.-]*) ([num]*))";

private:
    std::string m_spm_path;
    std::vector<std::string> m_image_type_list;

    const std::string m_file_list_end_str = "\\*File list end";
    const std::string m_ciao_image_list_str = "\\*Ciao image list";

    // File head general attributes
    // Can be modified: 可添加需要的属性
    unsigned int m_scan_size{};
    long long m_engage_x_pos_nm{};
    long long m_engage_y_pos_nm{};
    int m_x_offset_nm{};
    int m_y_offset_nm{};

    // Image
    std::unordered_map<std::string, SpmImage> m_image_list;
};


#endif //SPM_READER_HPP
