#include <opencv2/core.hpp>
#include <opencv2/features.hpp>   // OpenCV 5: SIFT, BFMatcher y drawMatches están en el módulo features
#include <opencv2/geometry.hpp>   // OpenCV 5: findHomography está en el módulo geometry
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stitching.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

const std::vector<std::string> IMAGES = {"../data/1.jpg", "../data/2.jpg", "../data/3.jpg"};
const int REFERENCE = 1;
const std::string WINDOW_PANORAMA = "P2 panorama";
const std::string WINDOW_STITCHER = "P2 Stitcher";
const int MAX_WIDTH = 1000;
const int DISPLAY_HEIGHT = 500;
const double RATIO = 0.60;
const double RANSAC_THRESHOLD = 3.0;
const double MAX_CANVAS = 5;


/** Keypoints y descriptores SIFT de la imagen. Copiar la solución de la P1. */
void detectFeatures(const cv::Mat& img, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors) {
    //// TODO: CODE HERE
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    sift->detectAndCompute(gray, cv::noArray(), keypoints, descriptors);
    //// END TODO
}

/** Emparejamientos que pasan el test. Copiar la solución de la P1. */
std::vector<cv::DMatch> matchFeatures(const cv::Mat& desc1, const cv::Mat& desc2, double ratio) {
    cv::BFMatcher matcher(cv::NORM_L2);
    std::vector<std::vector<cv::DMatch>> knn;   // knn[i] = los 2 vecinos más cercanos del descriptor i
    //// TODO: CODE HERE
    matcher.knnMatch(desc1, desc2, knn, 2);

    std::vector<cv::DMatch> matches;
    for (const auto& neighbours : knn) {
        cv::DMatch best = neighbours[0];
        cv::DMatch second = neighbours[1];
        if (best.distance < ratio * second.distance) {
            matches.push_back(best);
        }
    }
    //// END TODO
    return matches;
}

/**
 * Homografía H que lleva los puntos de la imagen 2 a la imagen 1: x1 ~ H x2.
 *
 * PISTA: los puntos de la imagen 2 son kp2[m.trainIdx].pt y los de la 1, kp1[m.queryIdx].pt.
 *
 * @param kp1, kp2  keypoints de la imagen 1 y de la imagen 2
 * @param matches   emparejamientos (queryIdx en la imagen 1, trainIdx en la 2)
 * @param inliers   (salida) un 0 o un 1 por emparejamiento (sale del mask de cv::findHomography)
 * @return H, matriz CV_64F de 3 x 3, o una matriz vacía si no se puede estimar
 */
cv::Mat estimateHomography(const std::vector<cv::KeyPoint>& kp1, const std::vector<cv::KeyPoint>& kp2,
                           const std::vector<cv::DMatch>& matches, std::vector<char>& inliers) {
    double threshold = RANSAC_THRESHOLD;
    //// TODO: CODE HERE
    std::vector<cv::Point2f> pts1, pts2;
    for (const auto& m : matches) {
        pts1.push_back(kp1[m.queryIdx].pt);
        pts2.push_back(kp2[m.trainIdx].pt);
    }
    cv::Mat mask;
    cv::Mat H = cv::findHomography(pts2, pts1, cv::RANSAC, threshold, mask);
    if (!H.empty()) {
        for (int i = 0; i < mask.rows; i++) {
            inliers[i] = mask.at<uchar>(i);
        }
    }
    //// END TODO
    return H;
}

/**
 * @param img1, img2  imágenes BGR
 * @param H           homografía 3 x 3 (CV_64F) que lleva la imagen 2 a la 1
 * @return la panorámica (BGR), o una matriz vacía si el lienzo sale demasiado grande (H mal estimada)
 */
cv::Mat stitch(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& H) {
    // 1. Calcular el rectángulo que contiene las esquinas de ambas imágenes, considerando la homografía H.
    std::vector<cv::Point2f> corners2 = {{0, 0}, {(float)img2.cols, 0}, {(float)img2.cols, (float)img2.rows},
                                         {0, (float)img2.rows}};
    std::vector<cv::Point2f> corners;
    cv::perspectiveTransform(corners2, corners, H); // Solo deformamos las esquinas de la imagen 2
    corners.insert(corners.end(), {{0, 0}, {(float)img1.cols, 0}, {(float)img1.cols, (float)img1.rows},
                                   {0, (float)img1.rows}});
    float xMin = corners[0].x, xMax = corners[0].x, yMin = corners[0].y, yMax = corners[0].y;
    for (const auto& c : corners) {
        xMin = std::min(xMin, c.x);
        xMax = std::max(xMax, c.x);
        yMin = std::min(yMin, c.y);
        yMax = std::max(yMax, c.y);
    }
    double x0 = std::floor(xMin), y0 = std::floor(yMin);
    int width = (int)(std::ceil(xMax) - x0), height = (int)(std::ceil(yMax) - y0);
    // Sanity check: si el lienzo es demasiado grande, H está mal estimada
    if ((double)width * height > MAX_CANVAS * img1.cols * img1.rows) {
        return cv::Mat();
    }

    // 2. Calcular la traslación T que lleva (x0, y0) a (0, 0) para que warpPerspective no pierda las coordenadas negativas.
    cv::Mat T = cv::Mat::eye(3, 3, CV_64F);
    T.at<double>(0, 2) = -x0;
    T.at<double>(1, 2) = -y0;

    // 3. Deformar la imagen 2 con TH y copiar la imagen 1 en la posición correspondiente.
    cv::Mat panorama;
    cv::Mat TH = T * H;
    cv::warpPerspective(img2, panorama, TH, cv::Size(width, height)); // Deformamos la imagen 2 con la homografía compuesta TH
    img1.copyTo(panorama(cv::Rect((int)-x0, (int)-y0, img1.cols, img1.rows)));
    return panorama;
}

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

int main() {
    //// 1. Las imágenes
    std::vector<cv::Mat> images;
    for (const auto& path : IMAGES) {
        images.push_back(load(path));
    }
    cv::Mat panorama = images[REFERENCE];
    for (int i = 0; i < (int)images.size(); i++) {
        if (i == REFERENCE) {
            continue;
        }
        const cv::Mat& img = images[i];
        std::vector<cv::KeyPoint> kp1, kp2;
        cv::Mat desc1, desc2;
        detectFeatures(panorama, kp1, desc1);
        detectFeatures(img, kp2, desc2);
        std::vector<cv::DMatch> matches;
        if (!desc1.empty() && !desc2.empty()) {
            matches = matchFeatures(desc1, desc2, RATIO);
        }
        std::vector<char> inliers(matches.size(), 0);
        cv::Mat H;
        if (matches.size() >= 4) {   // findHomography necesita al menos 4 emparejamientos
            H = estimateHomography(kp1, kp2, matches, inliers);
        }
        int nInliers = cv::countNonZero(inliers);
        std::cout << "Image " << i + 1 << ": " << matches.size() << " matches, " << nInliers << " inliers"
                  << std::endl;
        std::cout << "H =" << std::endl << H << std::endl;
        cv::Mat shown;
        cv::drawMatches(panorama, kp1, img, kp2, matches, shown, cv::Scalar(0, 255, 0), cv::Scalar::all(-1),
                        inliers, cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
        std::string window = cv::format("P2 inliers, image %d", i + 1);
        show(window, shown);
        cv::setWindowTitle(window, cv::format("image %d: %d inliers of %d matches", i + 1, nInliers,
                                              (int)matches.size()));

        if (H.empty()) {
            std::cout << "Not enough matches to estimate H for image " << i + 1 << std::endl;
            continue;
        }
        cv::Mat stitched = stitch(panorama, img, H);
        if (stitched.empty()) {
            std::cout << "The panorama is too big: H of image " << i + 1 << " is wrong" << std::endl;
            continue;
        }
        panorama = stitched;
    }

    //// 2. Panorámica
    show(WINDOW_PANORAMA, panorama);
    cv::imwrite("panorama.jpg", panorama);

    //// 3. Lo mismo con cv::Stitcher, para comparar: además ajusta la exposición y difumina la costura
    cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA);
    cv::Mat reference;
    cv::Stitcher::Status status = stitcher->stitch(images, reference);
    if (status == cv::Stitcher::OK) {
        show(WINDOW_STITCHER, reference);
    } else {
        std::cout << "cv::Stitcher failed with status " << (int)status << std::endl;
    }

    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;
}
