#include <opencv2/core.hpp>
#include <opencv2/features.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "viewer3d.hpp"

// Lee la calibración stereo
bool loadStereoCalibration(const std::string &path, cv::Mat &Kl, cv::Mat &Dl, cv::Mat &Kr, cv::Mat &Dr,
                           cv::Mat &R, cv::Mat &t)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;
    fs["cameraMatrixL"] >> Kl;
    fs["distCoeffsL"] >> Dl;
    fs["cameraMatrixR"] >> Kr;
    fs["distCoeffsR"] >> Dr;
    fs["R"] >> R;
    fs["T"] >> t;
    return !Kl.empty() && !Dl.empty() && !Kr.empty() && !Dr.empty() && !R.empty() && !t.empty();
}

// Lee el par de imágenes de la carpeta: img0_l e img0_r (.jpg o .png). Devuelve false si no están las dos.
bool readStereoPair(const std::string &folder, cv::Mat &imgL, cv::Mat &imgR)
{
    for (std::string ext : {".jpg", ".png"}) {
        std::string left = folder + "/img0_l" + ext;
        std::string right = folder + "/img0_r" + ext;
        if (std::filesystem::exists(left) && std::filesystem::exists(right)) {
            imgL = cv::imread(left);
            imgR = cv::imread(right);
            return !imgL.empty() && !imgR.empty();
        }
    }
    return false;
}

// Matriz antisimétrica de t: [t]x · v = t x v (producto vectorial)
cv::Mat skew(const cv::Mat &t)
{
    cv::Mat v;
    t.convertTo(v, CV_64F);
    double x = v.at<double>(0), y = v.at<double>(1), z = v.at<double>(2);
    return cv::Mat(cv::Matx33d(0, -z, y,
                               z, 0, -x,
                               -y, x, 0));
}

// Dibuja los emparejamientos: verde los que cumplen la restricción epipolar y rojo los que no
cv::Mat drawMatches(const cv::Mat &imgL, const std::vector<cv::KeyPoint> &kpL, const cv::Mat &imgR,
                    const std::vector<cv::KeyPoint> &kpR, const std::vector<cv::DMatch> &matches,
                    const std::vector<cv::DMatch> &goodMatches)
{
    cv::Mat canvas;
    cv::hconcat(imgL, imgR, canvas);
    std::set<std::pair<int, int>> good;
    for (const cv::DMatch &m : goodMatches)
        good.insert({m.queryIdx, m.trainIdx});
    for (const cv::DMatch &m : matches) {
        bool isGood = good.count({m.queryIdx, m.trainIdx}) > 0;
        cv::Scalar color = isGood ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
        cv::Point2f p1 = kpL[m.queryIdx].pt;
        cv::Point2f p2 = kpR[m.trainIdx].pt + cv::Point2f(imgL.cols, 0);
        cv::line(canvas, p1, p2, color, 1, cv::LINE_AA);
    }
    return canvas;
}

// Guarda los puntos en formato OBJ (una línea "v x y z r g b" por punto, color entre 0 y 1), para verlos con MeshLab
void writeObj(const std::string &path, const std::vector<cv::Point3f> &points, const std::vector<cv::Vec3b> &colors)
{
    std::ofstream file(path);
    for (size_t i = 0; i < points.size(); i++) {
        const cv::Point3f &p = points[i];
        const cv::Vec3b &c = colors[i];
        file << cv::format("v %.4f %.4f %.4f %.3f %.3f %.3f\n", p.x, p.y, p.z, c[2] / 255.0, c[1] / 255.0,
                           c[0] / 255.0);
    }
}



// TODO 1: calcular la matriz fundamental F a partir de la calibración (igual que el TODO 1 de la práctica P2):
// F = Kr^-T · [t]x · R · Kl^-1
// Devuelve F
cv::Mat computeFundamental(const cv::Mat &Kl, const cv::Mat &Kr, const cv::Mat &R, const cv::Mat &t)
{
    // --- TO DO ---
    // cv::Mat F = ...
    // Nota: .inv() es la inversa, .t() la traspuesta y * el producto de matrices
    // Nota: [t]x se calcula con la función skew(t), definida más arriba
    // Nota: opcional, dividir F entre su norma, cv::norm(F), para que tenga norma 1
    return cv::Mat();
}

// TODO 2: detectar keypoints y calcular sus descriptores SIFT (igual que detectFeatures de la clase 4).
void detectFeatures(const cv::Mat &img, std::vector<cv::KeyPoint> &keypoints, cv::Mat &descriptors)
{
    // --- TO DO ---
    // cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    // sift->detectAndCompute(...);
    // Nota: SIFT trabaja con la imagen en gris
}

// TODO 3: emparejar los descriptores de desc1 con los de desc2, con el ratio test (igual que matchFeatures de la
// clase 4): se acepta el vecino más cercano solo si distancia_1 < ratio * distancia_2.
// Devuelve los cv::DMatch (queryIdx = keypoint de desc1, trainIdx = keypoint de desc2).
std::vector<cv::DMatch> matchFeatures(const cv::Mat &desc1, const cv::Mat &desc2, double ratio)
{
    cv::BFMatcher matcher(cv::NORM_L2);
    std::vector<std::vector<cv::DMatch>> knn;   // knn[i] = los 2 vecinos más cercanos del descriptor i
    // --- TO DO ---
    std::vector<cv::DMatch> matches;
    // matcher.knnMatch(...);
    // Nota: cada knn[i] tiene los 2 vecinos más cercanos, ordenados, y la distancia está en .distance
    return matches;
}

// TODO 4: quedarse solo con los emparejamientos que cumplen la restricción epipolar: el punto derecho tiene que
// estar a menos de MAX_EPIPOLAR_DIST pixels de la línea epipolar (a, b, c) del punto izquierdo.
// Devuelve los cv::DMatch que la cumplen.
std::vector<cv::DMatch> filterEpipolar(const std::vector<cv::KeyPoint> &kpL, const std::vector<cv::KeyPoint> &kpR,
                                       const std::vector<cv::DMatch> &matches, const cv::Mat &F)
{
    const float MAX_EPIPOLAR_DIST = 2.0f;   // distancia máxima (en pixels) de un punto a su línea epipolar
    std::vector<cv::DMatch> goodMatches;
    if (matches.empty()) return goodMatches;
    std::vector<cv::Point2f> pointsL;
    for (const cv::DMatch &m : matches)
        pointsL.push_back(kpL[m.queryIdx].pt);
    // --- TO DO ---
    std::vector<cv::Vec3f> lines;
    // cv::computeCorrespondEpilines(...);
    // Nota: como en P2. lines[i] es la línea (a, b, c) del emparejamiento i
    // Pista: recorrer los emparejamientos y quedarse con los que estén cerca de su línea:
    // for (size_t i = 0; i < matches.size(); i++) {
    //     cv::Point2f p = kpR[matches[i].trainIdx].pt;    // punto derecho del emparejamiento i
    //     float dist = ...                                 // |a·x + b·y + c| / sqrt(a * a + b * b)
    //     if (dist < MAX_EPIPOLAR_DIST)
    //         goodMatches.push_back(matches[i]);
    // }
    return goodMatches;
}

// TODO 5: triangular los puntos emparejados (la cámara izquierda es el origen del sistema de coordenadas).
// Devuelve los puntos 3D.
std::vector<cv::Point3f> triangulate(const std::vector<cv::Point2f> &pointsL, const std::vector<cv::Point2f> &pointsR,
                                     const cv::Mat &Kl, const cv::Mat &Kr, const cv::Mat &R, const cv::Mat &t)
{
    // matrices de proyección de las dos cámaras (la izquierda es el origen): P1 = Kl·[I | 0] y P2 = Kr·[R | t]
    cv::Mat Rt;
    cv::hconcat(R, t, Rt);
    cv::Mat P1 = Kl * cv::Mat::eye(3, 4, CV_64F);
    cv::Mat P2 = Kr * Rt;
    // --- TO DO ---
    cv::Mat points4D;
    // cv::triangulatePoints(...);

    // triangulatePoints da los puntos en coordenadas homogéneas (X, Y, Z, W), 4 x N: se pasan a 3D (X / W, Y / W, Z / W)
    std::vector<cv::Point3f> points3D;
    if (!points4D.empty()) cv::convertPointsFromHomogeneous(points4D.t(), points3D);
    return points3D;
}

int main(int argc, char *argv[])
{

    //// 1. LEEMOS PARAMETROS
    if (argc != 2) {
        std::cerr << "Usage: path_to_data_folder" << std::endl;
        std::cerr << "Folder must contain: img0_l, img0_r (.jpg or .png) and calibration.yml" << std::endl;
        return -1;
    }
    std::string folder = argv[1];
    std::string calibrationFile = folder + "/calibration.yml";


    //// 2. LEEMOS CALIBRACION E IMAGENES
    cv::Mat Kl, Dl, Kr, Dr, R, t;
    if (!loadStereoCalibration(calibrationFile, Kl, Dl, Kr, Dr, R, t)) {
        std::cerr << "Cannot read " << calibrationFile << std::endl;
        return -1;
    }
    cv::Mat frameL, frameR;
    if (!readStereoPair(folder, frameL, frameR)) {
        std::cerr << "Cannot read " << folder << "/img0_l.jpg + img0_r.jpg (or .png)" << std::endl;
        return -1;
    }
    std::cout << "Image " << frameL.cols << "x" << frameL.rows << std::endl;
    cv::Mat F = computeFundamental(Kl, Kr, R, t);
    if (F.empty()) {
        std::cerr << "F is empty (is TODO 1 done?)" << std::endl;
        return -1;
    }


    //// 3. QUITAMOS LA DISTORSIÓN
    cv::Mat undistortedL, undistortedR;
    cv::undistort(frameL, undistortedL, Kl, Dl);
    cv::undistort(frameR, undistortedR, Kr, Dr);


    //// 4. KEYPOINTS Y DESCRIPTORES
    std::vector<cv::KeyPoint> kpL, kpR;
    cv::Mat descL, descR;
    detectFeatures(undistortedL, kpL, descL);
    detectFeatures(undistortedR, kpR, descR);


    //// 5. MATCHING
    std::vector<cv::DMatch> matches;
    if (!descL.empty() && !descR.empty())
        matches = matchFeatures(descL, descR, 0.8);      // ratio = 0.8


    //// 6. FILTRO EPIPOLAR
    std::vector<cv::DMatch> goodMatches = filterEpipolar(kpL, kpR, matches, F);
    std::cout << "Keypoints: " << kpL.size() << " | " << kpR.size() << ", matches: " << matches.size()
              << ", epipolar: " << goodMatches.size() << std::endl;


    //// 7. TRIANGULACIÓN (solo los puntos que quedan delante de la cámara)
    std::vector<cv::Point3f> points;
    std::vector<cv::Vec3b> colors;
    if (!goodMatches.empty()) {
        std::vector<cv::Point2f> pointsL, pointsR;
        for (const cv::DMatch &m : goodMatches) {
            pointsL.push_back(kpL[m.queryIdx].pt);
            pointsR.push_back(kpR[m.trainIdx].pt);
        }
        std::vector<cv::Point3f> points3D = triangulate(pointsL, pointsR, Kl, Kr, R, t);
        for (size_t j = 0; j < points3D.size(); j++) {
            if (points3D[j].z <= 0) continue;
            points.push_back(points3D[j]);
            colors.push_back(undistortedL.at<cv::Vec3b>((int)pointsL[j].y, (int)pointsL[j].x));
        }
        if (!points.empty()) {
            std::vector<float> z;
            for (const cv::Point3f &p : points) z.push_back(p.z);
            std::nth_element(z.begin(), z.begin() + z.size() / 2, z.end());
            std::cout << "Points 3D: " << points.size() << ", median Z: " << z[z.size() / 2] << std::endl;
        }
    }


    //// 8. MOSTRAMOS RESULTADO: emparejamientos y nube 3D
    cv::Mat canvas = drawMatches(undistortedL, kpL, undistortedR, kpR, matches, goodMatches);
    cv::putText(canvas, "[s] save reconstruction.obj   [ESC] exit", cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(0, 255, 0), 2);
    cv::putText(canvas, "green: good (close to the epipolar line)", cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(0, 255, 0), 2);
    cv::putText(canvas, "red: filtered (far from the epipolar line)", cv::Point(10, 75), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(0, 0, 255), 2);
    cv::imshow("matches", canvas);
    Viewer3D viewer;
    viewerCreate(viewer, "3D", 3);
    viewerSetPoints(viewer, points, colors);
    int key = 0;
    while (key != 27) {
        viewerRender(viewer);
        key = cv::waitKey(30);
        if (key == 's') {
            writeObj("reconstruction.obj", points, colors);
            std::cout << "Saved reconstruction.obj (" << points.size() << " points)" << std::endl;
        }
    }
    cv::destroyAllWindows();

    return 0;
}
