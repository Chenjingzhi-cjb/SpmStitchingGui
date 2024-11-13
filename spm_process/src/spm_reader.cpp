#include "spm_reader.hpp"

// TODO: 1. 完善 Image Type
const std::vector<std::string> SpmImage::image_type_str = std::vector<std::string>{
        "Height", "Height Sensor", "Height Trace", "Height Retrace", "Amplitude Error"
};
