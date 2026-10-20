#include <opencv2/core.hpp>
#include <opencv2/features.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

const std::string IMAGE_1 = "../data/1.jpg";
const std::string IMAGE_2 = "../data/2.jpg";
const std::string WINDOW_KEYPOINTS = "P1 keypoints";
const std::string WINDOW_MATCHES = "P1 matches";
const int MAX_WIDTH = 1000;
const int DISPLAY_HEIGHT = 500;

/** Lee la imagen y la reduce a MAX_WIDTH de ancho si es más grande. */
cv::Mat load(const std::string& path) {
    cv::Mat img = cv::imread(path);
    if (img.empty()) {
        std::cout << "Cannot read " << path << std::endl;
        std::exit(1);
    }
    if (img.cols > MAX_WIDTH) {
        double scale = double(MAX_WIDTH) / img.cols;
        cv::resize(img, img, cv::Size(), scale, scale, cv::INTER_AREA);
    }
    return img;
}

/** Muestra la imagen reducida a DISPLAY_HEIGHT de alto si es más alta. Solo cambia lo que se ve. */
void show(const std::string& window, const cv::Mat& img) {
    cv::Mat shown = img;
    if (img.rows > DISPLAY_HEIGHT) {
        double scale = double(DISPLAY_HEIGHT) / img.rows;
        cv::resize(img, shown, cv::Size(), scale, scale, cv::INTER_AREA);
    }
    cv::imshow(window, shown);
}

/**
 * Keypoints y descriptores SIFT de la imagen.
 *
 * @param img          imagen BGR
 * @param keypoints    (salida) los keypoints
 * @param descriptors  (salida) matriz CV_32F de N filas x 128 columnas, un descriptor por keypoint, en
 *                     el mismo orden
 */
void detectFeatures(const cv::Mat& img, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors) {
    //// TODO: CODE HERE
    //// END TODO
}

/**
 * Emparejamientos de desc1 con desc2.
 *
 * Para cada descriptor de desc1 se buscan sus 2 vecinos más cercanos en desc2. El emparejamiento con
 * el más cercano se acepta solo si está claramente más cerca que el segundo:
 * distancia_1 < ratio * distancia_2.
 *
 * PISTA: matcher.knnMatch -> rellena knn.
 * Cada knn[i] tiene 2 cv::DMatch, ordenados del más cercano al más lejano.
 * La distancia está en .distance.
 *
 * @param desc1, desc2  descriptores SIFT de la imagen 1 y de la imagen 2
 * @param ratio         [0,1] -> cuanto más pequeño, más exigente
 * @return los emparejamientos
 */
std::vector<cv::DMatch> matchFeatures(const cv::Mat& desc1, const cv::Mat& desc2, double ratio) {
    cv::BFMatcher matcher(cv::NORM_L2);
    std::vector<std::vector<cv::DMatch>> knn;   // knn[i] = los 2 vecinos más cercanos del descriptor i
    //// TODO: CODE HERE
    std::vector<cv::DMatch> matches;
    //// END TODO
    return matches;
}

int main() {
    //// 1. Las dos imágenes
    cv::Mat img1 = load(IMAGE_1);
    cv::Mat img2 = load(IMAGE_2);

    //// 2. Keypoints y descriptores: se calculan una vez
    std::vector<cv::KeyPoint> kp1, kp2;
    cv::Mat desc1, desc2;
    detectFeatures(img1, kp1, desc1);
    detectFeatures(img2, kp2, desc2);
    std::cout << "Keypoints: " << kp1.size() << " in image 1, " << kp2.size() << " in image 2" << std::endl;
    cv::Mat both;
    cv::drawMatches(img1, kp1, img2, kp2, std::vector<cv::DMatch>(), both, cv::Scalar::all(-1),
                    cv::Scalar(0, 255, 0), std::vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    show(WINDOW_KEYPOINTS, both);

    //// 3. Emparejamientos, con el cociente en un trackbar
    cv::namedWindow(WINDOW_MATCHES);
    cv::createTrackbar("ratio x100", WINDOW_MATCHES, nullptr, 100);
    cv::setTrackbarPos("ratio x100", WINDOW_MATCHES, 75);

    double lastRatio = -1;
    while (true) {
        double ratio = std::max(1, cv::getTrackbarPos("ratio x100", WINDOW_MATCHES)) / 100.0;
        if (ratio != lastRatio) {
            lastRatio = ratio;
            std::vector<cv::DMatch> matches;
            if (!desc1.empty() && !desc2.empty()) {
                matches = matchFeatures(desc1, desc2, ratio);
            }
            cv::Mat shown;
            cv::drawMatches(img1, kp1, img2, kp2, matches, shown, cv::Scalar::all(-1), cv::Scalar::all(-1),
                            std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
            show(WINDOW_MATCHES, shown);
            cv::setWindowTitle(WINDOW_MATCHES, cv::format("ratio %.2f: %d matches", ratio, (int)matches.size()));
        }
        if (cv::waitKey(50) == 27) {
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
