//
// Created by agabor on 2017.09.08..
//

#include "DicomReader.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include <opencv2/core/core.hpp>


template <typename T>
std::tuple<T,T> getRange(const T* data, size_t size) {
    T min = -1;
    T max = 0;
    for (size_t i = 0; i < size; ++i) {
        T item = data[i];
        if (item < min) {
            min = item;
        }
        if (item > max) {
            max = item;
        }
    }
    return std::tuple<T,T>(min, max);
}

template <typename IT, typename OT>
void DicomReader::normalize(const IT* data, OT* normalized) const {
    auto maxval = (OT)-1;
    IT d = (IT)max - (IT)min;
    auto m =(double)maxval/(double)d;
    for( size_t i = 0; i < size; ++i){
        OT x = (data[i] - min) * m;
        normalized[i] = x;
    }
}


template <typename IT, typename OT>
void DicomReader::normalizeToFloat(const IT* data, OT* normalized) const {
    auto d = (double)((IT)max - (IT)min);
    for( size_t i = 0; i < size; ++i){
        normalized[i] = (OT)((data[i] - min) / d);
    }
}

void DicomReader::config(DicomImage &img) {
    readImageProperties(img);
    configureNormalization(img);
    calculateValuableBits();
}

void DicomReader::readImageProperties(const DicomImage &img) {
    auto w = int(img.getWidth());
    auto h = int(img.getHeight());
    if (input_bits == 0) {
        depth = img.getDepth();
        width = w;
        height = h;
        input_bits = 1;
        while (input_bits < depth)
            input_bits *= 2;
    } else {
        if (depth != img.getDepth())
            throw std::exception();
        if (width != w)
            throw std::exception();
        if (height != h)
            throw std::exception();
    }
}

void DicomReader::calculateValuableBits() {
    auto interval = max - min;
    while (interval != 0) {
        interval /= 2;
        ++valuable_bits;
    }
}

void DicomReader::configureNormalization(DicomImage &img) {
    switch (input_bits) {
            case 8:
                configureNormalization<uint8_t>(img);
                break;
            case 16:
                configureNormalization<uint16_t>(img);
                break;
            case 32:
                configureNormalization<uint32_t>(img);
                break;
            case 64:
                configureNormalization<uint64_t>(img);
                break;
            default:
                throw std::exception();
        }
}

template<typename T>
void DicomReader::configureNormalization(DicomImage &img) {
    T min, max;
    size_t size = (size_t)width * (size_t)height;
    auto *data = getOutputData<T>(img);
    std::tie(min, max) = getRange<T>(data, size);
    if (min < this->min)
        this->min = min;
    if (max > this->max)
        this->max = max;
    if (this->size == 0)
        this->size = size;
    else if (this->size != size)
        throw std::exception();
}

cv::Mat DicomReader::read(const char* file_name, int cv_type) {
    auto result = std::vector<cv::Mat>();

    if (cv_type == -1)
        cv_type = guessCVType();
    DicomImage img(file_name);
    config(img);
    cv::Mat m;
    switch (input_bits) {
        case 8:
            m = createMat<uint8_t>(img, cv_type);
            break;
        case 16:
            m = createMat<uint16_t>(img, cv_type);
            break;
        case 32:
            m = createMat<uint32_t>(img, cv_type);
            break;
        case 64:
            m = createMat<uint64_t>(img, cv_type);
            break;
        default:
            throw std::exception();
    }
    return m;
}

int DicomReader::guessCVType() const {
    if (valuable_bits <= 8) {
        return CV_8U;
    } else if (valuable_bits <= 16) {
        return CV_16U;
    } else if (valuable_bits <= 32) {
        return CV_32F;
    } if (valuable_bits <= 64) {
        return CV_64F;
    }
    return CV_8U;
}

template<typename T>
cv::Mat DicomReader::createMat(DicomImage &img, int cv_type) const {
    cv::Mat result(height, width, cv_type);

    auto *img_data = getOutputData<T>(img);
    switch (cv_type) {
        case CV_8U:
            normalize<T, uint8_t>(img_data, reinterpret_cast<uint8_t *>(result.data));
            break;
        case CV_16U:
            normalize<T, uint16_t>(img_data, reinterpret_cast<uint16_t *>(result.data));
            break;
        case CV_32F:
            normalizeToFloat<T, float>(img_data, reinterpret_cast<float *>(result.data));
            break;
        case CV_64F:
            normalizeToFloat<T, double>(img_data, reinterpret_cast<double *>(result.data));
            break;
        default:
            throw std::exception();
    }

    return result;
}

template<typename T>
const T *DicomReader::getOutputData(DicomImage &img) const
{
    return static_cast<const T*>(img.getOutputData(sizeof(T) * 8));
}

bool DicomReader::isDicomFile(const char *file_name) {
    DicomImage img(file_name);
    EI_Status status = img.getStatus();
    return status == EIS_Normal;
}


