#include <opencv2/calib.hpp>
#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stereo.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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

// Disparidad en color (rojo cerca, azul lejos) y negro donde no es válida
cv::Mat colorizeDisparity(const cv::Mat &disparity, int numDisparities)
{
    cv::Mat gray, color;
    disparity.convertTo(gray, CV_8U, 255.0 / numDisparities);
    cv::applyColorMap(gray, color, cv::COLORMAP_JET);
    color.setTo(cv::Scalar::all(0), disparity <= 0);
    return color;
}

// Aplica la rectificación a una imagen: como undistort en la clase 7, pero además gira la cámara (Rrect) y usa su
// nueva matriz de proyección (P)
cv::Mat rectifyImage(const cv::Mat &img, const cv::Mat &K, const cv::Mat &D, const cv::Mat &Rrect, const cv::Mat &P)
{
    cv::Mat mapX, mapY, rectified;
    cv::initUndistortRectifyMap(K, D, Rrect, P, img.size(), CV_32FC1, mapX, mapY);
    cv::remap(img, rectified, mapX, mapY, cv::INTER_LINEAR);
    return rectified;
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



// TODO 1: rectificación stereo (como en el TODO 3 de la práctica P2): calcular las rotaciones R1 y R2 que alinean las
// dos cámaras y sus nuevas matrices de proyección P1 y P2
void computeRectification(const cv::Mat &Kl, const cv::Mat &Dl, const cv::Mat &Kr, const cv::Mat &Dr, const cv::Mat &R,
                          const cv::Mat &t, cv::Size imageSize, cv::Mat &R1, cv::Mat &R2, cv::Mat &P1, cv::Mat &P2)
{
    // --- TO DO ---
    cv::Mat Q;
    // cv::stereoRectify(...);
    // Nota: Usar flags = cv::CALIB_ZERO_DISPARITY y alpha = -1 (escala por defecto)
}

// TODO 2: calcular el mapa de disparidad con block matching (cv::StereoBM).
// Devuelve la disparidad en pixels (CV_32F). Los pixels sin disparidad válida quedan negativos.
cv::Mat computeDisparityBM(const cv::Mat &grayL, const cv::Mat &grayR, int numDisparities, int blockSize)
{
    // --- TO DO ---
    cv::Mat disparity16, disparity;
    // cv::Ptr<cv::StereoBM> matcher = cv::StereoBM::create(...);
    // matcher->compute(...);

    // compute da la disparidad x16 en entero (CV_16S) para no usar decimales: se pasa a float y se divide entre 16
    disparity16.convertTo(disparity, CV_32F, 1.0 / 16.0);
    return disparity;
}

// TODO 3: calcular el mapa de disparidad con semi-global block matching (cv::StereoSGBM).
// Devuelve la disparidad en pixels (CV_32F). Los pixels sin disparidad válida quedan negativos.
cv::Mat computeDisparitySGBM(const cv::Mat &grayL, const cv::Mat &grayR, int numDisparities, int blockSize)
{
    // --- TO DO ---
    cv::Mat disparity16, disparity;
    // cv::Ptr<cv::StereoSGBM> matcher = cv::StereoSGBM::create(...);
    // Nota: Usar minDisparity = 0, P1 = 8 * blockSize * blockSize, P2 = 32 * blockSize * blockSize,
    //       disp12MaxDiff = 0, preFilterCap = 0, uniquenessRatio = 10, speckleWindowSize = 100 y speckleRange = 2
    // matcher->compute(...);

    // compute da la disparidad x16 en entero (CV_16S) para no usar decimales: se pasa a float y se divide entre 16
    disparity16.convertTo(disparity, CV_32F, 1.0 / 16.0);
    return disparity;
}

// TODO 4: pasar a 3D los pixels con disparidad válida (d > 0), con las ecuaciones del par rectificado.
// Rellena points y colors
void disparityToPoints(const cv::Mat &disparity, const cv::Mat &imgL, const cv::Mat &cameraMatrix, double baseline,
                       std::vector<cv::Point3f> &points, std::vector<cv::Vec3b> &colors)
{
    points.clear();
    colors.clear();
    double f = cameraMatrix.at<double>(0, 0);
    double cx = cameraMatrix.at<double>(0, 2);
    double cy = cameraMatrix.at<double>(1, 2);
    // --- TO DO ---
    // Pista: recorrer todos los pixels (x, y) y añadir punto 3D y color (si disparidad > 0)
    // La disparidad está en disparity.at<float>(y, x) y el color en imgL.at<cv::Vec3b>(y, x)
    // Nota: Z = baseline * f / d     X = (x - cx) * Z / f     Y = (y - cy) * Z / f
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
    cv::Mat imgL, imgR;
    if (!readStereoPair(folder, imgL, imgR)) {
        std::cerr << "Cannot read " << folder << "/img0_l.png + img0_r.png (or .jpg)" << std::endl;
        return -1;
    }
    std::cout << "Image " << imgL.cols << "x" << imgL.rows << std::endl;


    //// 3. RECTIFICAMOS EL PAR
    cv::Mat R1, R2, P1, P2;
    computeRectification(Kl, Dl, Kr, Dr, R, t, imgL.size(), R1, R2, P1, P2);
    if (R1.empty() || R2.empty() || P1.empty() || P2.empty()) {
        std::cerr << "Pair not rectified (is TODO 1 done?)" << std::endl;
        return -1;
    }
    cv::Mat rectL = rectifyImage(imgL, Kl, Dl, R1, P1);
    cv::Mat rectR = rectifyImage(imgR, Kr, Dr, R2, P2);
    cv::Mat K = P1.colRange(0, 3).clone();

    // baseline: distancia entre los centros de las dos cámaras (rectificar las gira, pero no las mueve)
    double baseline = cv::norm(t);
    std::cout << "Baseline: " << baseline << std::endl;


    //// 4. CREAMOS LAS VENTANAS: par rectificado, disparidad y 3D
    cv::Mat pair;
    cv::hconcat(rectL, rectR, pair);
    cv::putText(pair, "[s] save reconstruction.obj   [ESC] exit", cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(0, 255, 0), 2);
    cv::imshow("stereo", pair);
    cv::namedWindow("disparity");
    cv::createTrackbar("SGBM", "disparity", nullptr, 1);
    cv::createTrackbar("numDisp x16", "disparity", nullptr, 16);
    cv::createTrackbar("blockSize", "disparity", nullptr, 31);
    cv::setTrackbarPos("SGBM", "disparity", 0);
    cv::setTrackbarPos("numDisp x16", "disparity", 9);
    cv::setTrackbarPos("blockSize", "disparity", 9);
    Viewer3D viewer;
    viewerCreate(viewer, "3D", 1);


    //// 5. CALCULAMOS LA DISPARIDAD Y LOS PUNTOS 3D CADA VEZ QUE CAMBIA UN PARAMETRO
    // BM y SGBM trabajan con las imágenes rectificadas en gris
    cv::Mat grayL, grayR;
    cv::cvtColor(rectL, grayL, cv::COLOR_BGR2GRAY);
    cv::cvtColor(rectR, grayR, cv::COLOR_BGR2GRAY);
    cv::Vec3i lastParams(-1, -1, -1);
    std::vector<cv::Point3f> points;
    std::vector<cv::Vec3b> colors;
    int key = 0;
    while (key != 27) {
        int useSGBM = cv::getTrackbarPos("SGBM", "disparity");
        int numDisparities = 16 * std::max(1, cv::getTrackbarPos("numDisp x16", "disparity"));
        int blockSize = std::max(5, cv::getTrackbarPos("blockSize", "disparity")) | 1;     // impar y >= 5
        cv::Vec3i params(useSGBM, numDisparities, blockSize);

        if (params != lastParams) {
            lastParams = params;

            // disparidad
            cv::Mat disparity;
            if (useSGBM)
                disparity = computeDisparitySGBM(grayL, grayR, numDisparities, blockSize);
            else
                disparity = computeDisparityBM(grayL, grayR, numDisparities, blockSize);
            if (disparity.empty())
                disparity = cv::Mat(grayL.size(), CV_32F, cv::Scalar(-1));

            // puntos 3D
            disparityToPoints(disparity, rectL, K, baseline, points, colors);
            viewerSetPoints(viewer, points, colors);

            // mostramos la disparidad
            std::string name = useSGBM ? "SGBM" : "BM";
            std::cout << name << " numDisparities=" << numDisparities << " blockSize=" << blockSize << ": "
                      << points.size() << " points" << std::endl;
            cv::Mat disparityColor = colorizeDisparity(disparity, numDisparities);
            cv::putText(disparityColor, name, cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                        cv::Scalar(255, 255, 255), 2);
            cv::imshow("disparity", disparityColor);
        }

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
