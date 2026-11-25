#include <opencv2/calib.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// Esquinas interiores del tablero y lado de una casilla (se leen por línea de comandos)
cv::Size boardSize;
float squareSize;


// Busca los pares de imágenes de la carpeta: img0_l / img0_r, img1_l / img1_r, ... (.jpg o .png).
void findStereoPairs(const std::string &folder, std::vector<std::string> &imagesLeft,
                     std::vector<std::string> &imagesRight)
{
    imagesLeft.clear();
    imagesRight.clear();
    for (int i = 0;; i++) {
        bool found = false;
        for (std::string ext : {".jpg", ".png"}) {
            std::string left = folder + "/img" + std::to_string(i) + "_l" + ext;
            std::string right = folder + "/img" + std::to_string(i) + "_r" + ext;
            if (std::filesystem::exists(left) && std::filesystem::exists(right)) {
                imagesLeft.push_back(left);
                imagesRight.push_back(right);
                found = true;
                break;
            }
        }
        if (!found) break;
    }
}

// Coordenadas 3D de las esquinas del tablero (Z = 0)
std::vector<cv::Point3f> getBoardPoints3D()
{
    std::vector<cv::Point3f> corners3D;
    for (int i = 0; i < boardSize.height; i++)
        for (int j = 0; j < boardSize.width; j++)
            corners3D.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
    return corners3D;
}

// Detecta y refina las esquinas del tablero
bool detectChessboard(const cv::Mat &gray, std::vector<cv::Point2f> &corners)
{
    bool found = cv::findChessboardCorners(gray, boardSize, corners,
                                           cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
    if (found) {
        cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
        cv::cornerSubPix(gray, corners, cv::Size(7, 7), cv::Size(-1, -1), criteria);
    }
    return found;
}

// TODO 1: calibrar una cámara con los puntos 3D y 2D de todas las imágenes (como en la clase 7).
// Devuelve el error de reproyección.
double calibrate(const std::vector<std::vector<cv::Point3f>> &objectPoints,
                 const std::vector<std::vector<cv::Point2f>> &imagePoints,
                 cv::Size imageSize, cv::Mat &cameraMatrix, cv::Mat &distCoeffs)
{
    // --- TO DO ---
    std::vector<cv::Mat> R, t;
    // return cv::calibrateCamera(...);
    return -1;
}

// TODO 2: calibrar extrínsecos del par stereo: rotación R y traslación t de la cámara derecha respecto a la izquierda.
// Las intrínsecas de cada cámara ya están calculadas y se mantienen.
// Devuelve el error de reproyección.
double calibrateStereo(const std::vector<std::vector<cv::Point3f>> &objectPoints,
                       const std::vector<std::vector<cv::Point2f>> &imagePointsL,
                       const std::vector<std::vector<cv::Point2f>> &imagePointsR,
                       cv::Mat &cameraMatrixL, cv::Mat &distCoeffsL, cv::Mat &cameraMatrixR, cv::Mat &distCoeffsR,
                       cv::Size imageSize, cv::Mat &R, cv::Mat &t)
{
    // --- TO DO ---
    cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 100, 1e-6);
    cv::Mat E, F;   // stereoCalibrate también devuelve E y F
    // return cv::stereoCalibrate(...);
    // Nota: Usar flags = cv::CALIB_FIX_INTRINSIC (no se tocan las intrínsecas de cada cámara) y el criteria
    //       de arriba como criterio de parada
    return -1;
}

// Guarda la calibración stereo en un .yml.
bool saveStereoCalibrationToFile(const std::string &path, const cv::Mat &cameraMatrixL, const cv::Mat &distCoeffsL,
                                 const cv::Mat &cameraMatrixR, const cv::Mat &distCoeffsR, const cv::Mat &R,
                                 const cv::Mat &t)
{
    cv::FileStorage fs(path, cv::FileStorage::WRITE);
    if (!fs.isOpened()) return false;
    fs << "cameraMatrixL" << cameraMatrixL << "distCoeffsL" << distCoeffsL;
    fs << "cameraMatrixR" << cameraMatrixR << "distCoeffsR" << distCoeffsR;
    fs << "R" << R << "T" << t;
    return true;
}

int main(int argc, char *argv[])
{

    //// 1. LEEMOS PARAMETROS
    if (argc != 5) {
        std::cerr << "Usage: folder corners_cols corners_rows square_size" << std::endl;
        return -1;
    }
    std::string folder = argv[1];
    boardSize = cv::Size(std::atoi(argv[2]), std::atoi(argv[3]));
    squareSize = std::atof(argv[4]);


    //// 2. LEEMOS RUTAS IMAGENES
    std::vector<std::string> imagesLeft, imagesRight;
    findStereoPairs(folder, imagesLeft, imagesRight);
    if (imagesLeft.empty()) {
        std::cerr << "No image pairs in " << folder << " (img0_l.jpg + img0_r.jpg, img1_l.jpg + img1_r.jpg, ...)"
                  << std::endl;
        return -1;
    }

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePointsL, imagePointsR;
    cv::Size imageSize;


    //// 3. DETECTAMOS PATRÓN EN CADA PAR DE IMÁGENES
    for (size_t i = 0; i < imagesLeft.size(); i++) {

        // 3.1 LEER IMAGENES
        cv::Mat frameL = cv::imread(imagesLeft[i]);
        cv::Mat frameR = cv::imread(imagesRight[i]);
        std::cout << "Reading.." << imagesLeft[i] << " | " << imagesRight[i] << " (" << frameL.cols << "x"
                  << frameL.rows << ")" << std::endl;
        cv::Mat grayL, grayR;
        cv::cvtColor(frameL, grayL, cv::COLOR_BGR2GRAY);
        cv::cvtColor(frameR, grayR, cv::COLOR_BGR2GRAY);
        imageSize = grayL.size();

        // 3.2 DETECTAMOS IMAGE POINTS EN LAS DOS IMAGENES
        std::vector<cv::Point2f> cornersL, cornersR;
        bool successL = detectChessboard(grayL, cornersL);
        bool successR = detectChessboard(grayR, cornersR);

        // 3.3 SOLO SE USA EL PAR SI EL TABLERO APARECE EN LAS DOS
        bool success = successL && successR;
        if (success) {
            objectPoints.push_back(getBoardPoints3D());
            imagePointsL.push_back(cornersL);
            imagePointsR.push_back(cornersR);
        }

        // 3.4 MOSTRAMOS RESULTADO: izquierda | derecha
        cv::drawChessboardCorners(frameL, boardSize, cornersL, successL);
        cv::drawChessboardCorners(frameR, boardSize, cornersR, successR);
        cv::Mat vis;
        cv::hconcat(frameL, frameR, vis);
        cv::imshow("left | right", vis);
        cv::waitKey(0);
    }
    cv::destroyAllWindows();

    if (objectPoints.size() < 3) {
        std::cerr << "Chessboard found in " << objectPoints.size() << " pairs, at least 3 are needed" << std::endl;
        return -1;
    }


    //// 4. CALIBRAMOS CADA CAMARA POR SEPARADO
    cv::Mat cameraMatrixL, distCoeffsL, cameraMatrixR, distCoeffsR;
    double errL = calibrate(objectPoints, imagePointsL, imageSize, cameraMatrixL, distCoeffsL);
    double errR = calibrate(objectPoints, imagePointsR, imageSize, cameraMatrixR, distCoeffsR);
    if (cameraMatrixL.empty() || cameraMatrixR.empty()) {
        std::cerr << "Cameras not calibrated (is TODO 1 done?)" << std::endl;
        return -1;
    }


    //// 5. CALIBRAMOS EL PAR STEREO
    cv::Mat R, t;
    double err = calibrateStereo(objectPoints, imagePointsL, imagePointsR, cameraMatrixL, distCoeffsL,
                                 cameraMatrixR, distCoeffsR, imageSize, R, t);
    if (R.empty()) {
        std::cerr << "Stereo calibration not done (is TODO 2 done?)" << std::endl;
        return -1;
    }


    //// 6. MOSTRAMOS RESULTADO Y GUARDAMOS A FICHERO
    std::cout << "Pairs used: " << objectPoints.size() << "/" << imagesLeft.size() << std::endl;
    std::cout << "Reprojection error: left " << errL << ", right " << errR << ", stereo " << err << std::endl;
    std::cout << "R : " << R << std::endl;
    std::cout << "t : " << t << std::endl;
    std::cout << "Baseline |t| : " << cv::norm(t) << std::endl;

    if (!saveStereoCalibrationToFile("calibration.yml", cameraMatrixL, distCoeffsL, cameraMatrixR,
                                     distCoeffsR, R, t)) {
        std::cerr << "Cannot write calibration.yml" << std::endl;
        return -1;
    }
    std::cout << "Saved calibration.yml" << std::endl;

    return 0;
}
