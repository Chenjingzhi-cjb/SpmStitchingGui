#ifndef SPM_ALGORITHM_HPP
#define SPM_ALGORITHM_HPP

#include "spm_reader.hpp"

#include <stdexcept>
#include <numeric>

#include "opencv2/opencv.hpp"


class SpmAlgorithm : public StringOperations {
private:
    SpmAlgorithm() = default;

    ~SpmAlgorithm() = default;

public:
    /**
     * @brief 一阶拉平处理
     *
     * @param data The SPM image real data.
     * @return None
     */
    template<typename T>
    static void flattenFirst(T &data) {
        const int num_cols = (int) data[0].size();

        double sum_m_mu = 0.0;
        double x_mean = (num_cols - 1) / 2.0;
        for (int c = 0; c < num_cols; ++c) {
            sum_m_mu += std::pow(c - x_mean, 2);
        }

        for (auto &row : data) {
            double row_mean = std::accumulate(row.begin(), row.end(), 0.0) / num_cols;

            double sum_m_zi = 0.0;
            for (auto it = row.begin(); it != row.end(); ++it) {
                int c = (int) std::distance(row.begin(), it);
                sum_m_zi += (c - x_mean) * (*it - row_mean);
            }

            double m = sum_m_zi / sum_m_mu;
            double b = row_mean - m * x_mean;

            for (auto &element : row) {
                int c = (int) (&element - &row[0]);
                element -= (m * c + b);
            }
        }
    }

    /**
     * @brief 一阶拉平处理
     *
     * @param spm_image The SPM image.
     * @return None
     */
    static void flattenFirst(SpmImage &spm_image) {
        flattenFirst(spm_image.getRealData());
    }

    /**
     * @brief 将二维数组转为 OpenCV Mat 对象
     *
     * @param array 2D-array
     * @return mat object
     */
    template<typename T>
    static cv::Mat array2DToImage(const T &array) {
        int rows = (int) array.size();
        int cols = (int) array[0].size();

        cv::Mat image(rows, cols, CV_64FC1);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                image.at<double>(r, c) = array[r][c];
            }
        }

        return image;
    }

    /**
     * @brief 将 SPM Image 的 Real Data 转为 OpenCV Mat 对象
     *
     * @param spm_image The SPM image.
     * @return mat object
     */
    static cv::Mat spmImageToImage(SpmImage &spm_image) {
        return array2DToImage(spm_image.getRealData());
    }

    /**
     * @brief 将 SPM Image 的 Real Data 保存为 BMP、JPG、PNG 等图像文件
     *
     * @param spm_image The SPM image.
     * @param file_path The file path to save the SPM image.
     * @return None
     */
    static void saveSpmImageToImage(SpmImage &spm_image, const std::string &file_path) {
        cv::Mat image = array2DToImage(spm_image.getRealData());
        cv::normalize(image, image, 0, 255, cv::NORM_MINMAX);

        cv::imwrite(file_path, image);
    }

    static std::pair<int, int> calcMatchTemplate(cv::Mat &image_tmpl, cv::Mat &image_offset) {
        if (image_tmpl.empty()) {
            throw std::invalid_argument(
                    "calcMatchTemplate() Error: Unable to load image_tmpl or image_tmpl loading error!");
        }

        if (image_offset.empty()) {
            throw std::invalid_argument(
                    "calcMatchTemplate() Error: Unable to load image_offset or image_offset loading error!");
        }

        cv::Mat image_tmpl_float = image_tmpl.clone();
        cv::Mat image_offset_float = image_offset.clone();
        image_tmpl_float.convertTo(image_tmpl_float, CV_32FC1);
        image_offset_float.convertTo(image_offset_float, CV_32FC1);

        // 进行模板匹配
        cv::Mat temp;
        matchTemplate(image_offset_float, image_tmpl_float, temp, cv::TM_CCOEFF_NORMED);

        // 定位匹配的位置
        cv::Point max_loc;
        minMaxLoc(temp, nullptr, nullptr, nullptr, &max_loc);

        return std::pair<int, int>{max_loc.x, max_loc.y};
    }
};


#endif //SPM_ALGORITHM_HPP
