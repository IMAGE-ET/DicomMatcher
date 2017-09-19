//
// Created by agabor on 2017.09.08..
//

#ifndef CV_TEST_DICOM_H
#define CV_TEST_DICOM_H

#include <vector>
#include <memory>

namespace cv {
    class Mat;
}

class DicomImage;

class DicomReader {
public:
    static bool isDicomFile(const char* file_name);
    cv::Mat read(const char* file_name, int cv_type = -1);
private:

    void config(DicomImage &img);
    template <typename T>
    void configureNormalization(DicomImage &img);
    void configureNormalization(DicomImage &img);

    template<typename T>
    cv::Mat createMat(DicomImage &img, int cv_type) const;

    template <typename IT, typename OT>
    void normalize(const IT* data, OT* normalized) const;

    template <typename IT, typename OT>
    void normalizeToFloat(const IT* data, OT* normalized) const;

    int guessCVType() const;


    void calculateValuableBits();

    void readImageProperties(const DicomImage &img);

    template<typename T>
    const T *getOutputData(DicomImage &img) const;


    int depth = 0;
    int input_bits = 0;
    int valuable_bits = 0;
    int width = 0;
    int height = 0;
    size_t size = 0;
    u_int32_t min = (u_int32_t)-1;
    u_int32_t max = 0;
};


#endif //CV_TEST_DICOM_H
