#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>

using namespace cv;
using namespace std::chrono;

#define USE_NV12_TO_BGR_OPENCV 0

void BGRToNV12(const Mat& bgr, Mat& nv12) {
    Mat yuv_i420;
    cvtColor(bgr, yuv_i420, COLOR_BGR2YUV_I420);

    int width = bgr.cols;
    int height = bgr.rows;

    nv12.create(height * 3 / 2, width, CV_8UC1);

    // Copy Y plane
    int y_size = width * height;
    memcpy(nv12.data, yuv_i420.data, y_size);

    // Interleave U and V planes
    uint8_t* uv_plane = nv12.data + y_size;
    uint8_t* u_plane = yuv_i420.data + y_size;
    uint8_t* v_plane = yuv_i420.data + y_size + (y_size / 4);

    for (int i = 0; i < y_size / 4; ++i) {
        uv_plane[i * 2] = u_plane[i];
        uv_plane[i * 2 + 1] = v_plane[i];
    }
}

#if USE_NV12_TO_BGR_OPENCV
#else
void NV12ToBGR(const Mat& nv12, Mat& bgr, int width, int height) {
    bgr.create(height, width, CV_8UC3);

    const uint8_t* y_plane = nv12.data;
    const uint8_t* uv_plane = nv12.data + width * height;

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int y_index = j * width + i;
            int uv_index = (j / 2) * width + (i & ~1);

            uint8_t y = y_plane[y_index];
            uint8_t u = uv_plane[uv_index];
            uint8_t v = uv_plane[uv_index + 1];

            int c = y - 16;
            int d = u - 128;
            int e = v - 128;

            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;

            bgr.at<Vec3b>(j, i)[2] = (r < 0) ? 0 : (r > 255) ? 255 : r;
            bgr.at<Vec3b>(j, i)[1] = (g < 0) ? 0 : (g > 255) ? 255 : g;
            bgr.at<Vec3b>(j, i)[0] = (b < 0) ? 0 : (b > 255) ? 255 : b;
        }
    }
}
#endif

int main() {
    // JPEGâÊëúÇì«Ç›çûÇﬁ
    Mat img_jpg = imread("image.jpg", IMREAD_COLOR);
    if (img_jpg.empty()) {
        std::cerr << "Failed to load image." << std::endl;
        return -1;
    }

    // BGRÇ©ÇÁYUV (NV12)Ç…ïœä∑
    Mat img_nv12;
    auto start = high_resolution_clock::now();
    BGRToNV12(img_jpg, img_nv12);
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start).count();
    std::cout << "BGR to NV12 conversion time: " << duration << " ms" << std::endl;

    // YUV (NV12)Ç©ÇÁBGRÇ…ïœä∑
    Mat img_bgr;
    start = high_resolution_clock::now();
#if USE_NV12_TO_BGR_OPENCV
	cvtColor(img_nv12, img_bgr, COLOR_YUV2BGR_NV12);
#else
    NV12ToBGR(img_nv12, img_bgr, img_jpg.cols, img_jpg.rows);
#endif
    end = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start).count();
    std::cout << "NV12 to BGR conversion time (CPU, single-threaded): " << duration << " ms" << std::endl;

    // BGRÇ©ÇÁBMPÇ…ïœä∑
    start = high_resolution_clock::now();
    imwrite("output.bmp", img_bgr);
    end = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end - start).count();
    std::cout << "BGR to BMP conversion time: " << duration << " ms" << std::endl;

    std::cout << "Conversion completed." << std::endl;
    return 0;
}


