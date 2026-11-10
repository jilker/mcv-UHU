#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <string>
#include <vector>


// Lee la calibración guardada por la práctica P1
bool loadCalibration(const std::string &path, cv::Mat &cameraMatrix, cv::Mat &distCoeffs)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;
    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    return !cameraMatrix.empty() && !distCoeffs.empty();
}



// TODO 1: quitar la distorsión de la imagen con la calibración de la cámara.
cv::Mat undistortImage(const cv::Mat &img, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs)
{
    // --- TO DO ---
    cv::Mat undistorted;
    // cv::undistort(...);
    return undistorted;
}

int main(int argc, char *argv[])
{

    //// 1. LEEMOS PARAMETROS
    if (argc != 3) {
        std::cerr << "Usage: calibration_file folder_with_images" << std::endl;
        return -1;
    }
    std::string calibrationFile = argv[1];
    std::string imagesFolder = argv[2];



    //// 2. LEEMOS CALIBRACION Y RUTAS IMAGENES
    cv::Mat cameraMatrix, distCoeffs;
    if (!loadCalibration(calibrationFile, cameraMatrix, distCoeffs)) {
        std::cerr << "Cannot read " << calibrationFile << std::endl;
        return -1;
    }

    std::vector<cv::String> images;
    for (std::string ext : {"/*.jpg", "/*.png"}) {
        std::vector<cv::String> found;
        cv::glob(imagesFolder + ext, found);
        images.insert(images.end(), found.begin(), found.end());
    }

    //// 3. QUITAMOS LA DISTORSIÓN DE CADA IMAGEN
    for (auto &file : images) {

        // 3.1 LEER IMAGEN
        cv::Mat frame = cv::imread(file);
        std::cout << "Reading.." << file << " (" << frame.cols << "x" << frame.rows << ")" << std::endl;

        // 3.2 QUITAMOS LA DISTORSIÓN
        cv::Mat undistorted = undistortImage(frame, cameraMatrix, distCoeffs);
        if (undistorted.empty()) {
            std::cerr << "Undistorted image is empty (is TODO 1 done?)" << std::endl;
            return -1;
        }

        // 3.3 MOSTRAMOS RESULTADO: original a la izquierda, sin distorsión a la derecha
        cv::Mat vis;
        cv::hconcat(frame, undistorted, vis);
        cv::imshow("original | undistorted", vis);
        cv::waitKey(0);
    }
    cv::destroyAllWindows();
    return 0;
}
