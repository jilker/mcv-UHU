#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <string>
#include <vector>

struct CameraProperty
{
    std::string name;
    int id;
    int min, max;
};

const std::vector<CameraProperty> PROPERTIES = {
    {"brightness", cv::CAP_PROP_BRIGHTNESS, 0, 255},
    {"contrast", cv::CAP_PROP_CONTRAST, 0, 255},
    {"saturation", cv::CAP_PROP_SATURATION, 0, 255},
    {"hue", cv::CAP_PROP_HUE, -180, 180},
    {"gamma", cv::CAP_PROP_GAMMA, 0, 500},
    {"gain", cv::CAP_PROP_GAIN, 0, 255},
    {"sharpness", cv::CAP_PROP_SHARPNESS, 0, 255},
    {"backlight", cv::CAP_PROP_BACKLIGHT, 0, 10},
    {"auto white balance", cv::CAP_PROP_AUTO_WB, 0, 1},
    {"wb temperature", cv::CAP_PROP_WB_TEMPERATURE, 2000, 10000},
    {"auto exposure", cv::CAP_PROP_AUTO_EXPOSURE, 0, 3},
    {"exposure", cv::CAP_PROP_EXPOSURE, 0, 5000},
    {"autofocus", cv::CAP_PROP_AUTOFOCUS, 0, 1},
    {"focus", cv::CAP_PROP_FOCUS, 0, 255},
    {"zoom", cv::CAP_PROP_ZOOM, 0, 500},
};

int main()
{
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open camera 0" << std::endl;
        return -1;
    }

    cv::namedWindow("Controls");
    std::vector<int> values;
    for (auto &p : PROPERTIES) {
        int value = (int)cap.get(p.id);
        cv::createTrackbar(p.name, "Controls", nullptr, p.max);
        cv::setTrackbarMin(p.name, "Controls", p.min);
        cv::setTrackbarPos(p.name, "Controls", value);
        values.push_back(value);
    }

    int captures = 0;
    while (true) {
        for (size_t i = 0; i < PROPERTIES.size(); i++) {
            int value = cv::getTrackbarPos(PROPERTIES[i].name, "Controls");
            if (value != values[i]) {
                cap.set(PROPERTIES[i].id, value);
                values[i] = value;
            }
        }

        cv::Mat frame;
        if (!cap.read(frame)) {
            std::cerr << "Cannot read from camera 0" << std::endl;
            return -1;
        }
        cv::Mat vis = frame.clone();
        cv::putText(vis, "[c] capture (" + std::to_string(captures) + ")   [ESC] exit", cv::Point(10, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Camera", vis);

        int key = cv::waitKey(1);
        if (key == 27)
            break;
        if (key == 'c') {
            std::string file = cv::format("capture_%03d.png", captures++);
            cv::imwrite(file, frame);
            std::cout << "Saved " << file << " (" << frame.cols << "x" << frame.rows << ")" << std::endl;
        }
    }
    cv::destroyAllWindows();
    return 0;
}
