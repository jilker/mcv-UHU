#include <opencv2/calib.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <string>
#include <vector>

// Esquinas interiores del tablero (se lee por línea de comandos)
cv::Size boardSize;


// Coordenadas 3D de las esquinas del tablero (Z = 0, cada cuadro mide 1 unidad),
// en el mismo orden que findChessboardCorners
bool defineBoardPoints3D(std::vector<cv::Point3f> &corners3D)
{
    corners3D.clear();
    for (int i = 0; i < boardSize.height; i++)
        for (int j = 0; j < boardSize.width; j++)
            corners3D.push_back(cv::Point3f(j, i, 0));
    return corners3D.size() == (size_t)boardSize.area();
}



// TODO 1: detectar las esquinas del tablero en la imagen en gris.
// Devuelve true si se encuentran todas las esquinas.
bool detectChessboard(const cv::Mat &gray, std::vector<cv::Point2f> &corners)
{
    // --- TO DO ---
    // return cv::findChessboardCorners(...)
    // Nota: Usar flags: cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE
    return false;
}

// TODO 2: refinar las esquinas a precisión subpíxel.
void refineCorners(const cv::Mat &gray, std::vector<cv::Point2f> &corners)
{
    // --- TO DO ---
    cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
    // cv::cornerSubPix(...);
    // Nota: Usar winSize = cv::Size(7, 7) y zeroZone = cv::Size(-1, -1)
}

// TODO 3: calibrar la cámara con los puntos 3D y 2D de todas las imágenes.
// Devuelve el error de reproyección.
double calibrate(const std::vector<std::vector<cv::Point3f>> &objectPoints,
                 const std::vector<std::vector<cv::Point2f>> &imagePoints,
                 cv::Size imageSize, cv::Mat &cameraMatrix, cv::Mat &distCoeffs)
{
    // --- TO DO ---
    std::vector<cv::Mat> R, T;
    // return cv::calibrateCamera(...);
    return -1;
}

// Guarda la calibración en un .yml (claves que lee la práctica P2)
bool saveCalibrationToFile(const std::string &path, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs)
{
    cv::FileStorage fs(path, cv::FileStorage::WRITE);
    if (!fs.isOpened()) return false;
    fs << "cameraMatrix" << cameraMatrix << "distCoeffs" << distCoeffs;
    return true;
}

int main(int argc, char *argv[])
{

    //// 1. LEEMOS PARAMETROS
    if (argc != 4) {
        std::cerr << "Usage: folder_with_images corners_cols corners_rows" << std::endl;
        return -1;
    }
    std::string imagesFolder = argv[1];
    boardSize = cv::Size(std::atoi(argv[2]), std::atoi(argv[3]));



    //// 2. LEEMOS RUTAS IMAGENES
    std::vector<cv::String> images;
    for (std::string ext : {"/*.jpg", "/*.png"}) {
        std::vector<cv::String> found;
        cv::glob(imagesFolder + ext, found);
        images.insert(images.end(), found.begin(), found.end());
    }

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;
    cv::Size imageSize;

    //// 3. DETECTAMOS PATRÓN EN CADA IMAGEN
    for (auto &file : images) {

        // 3.1 LEER IMAGEN
        cv::Mat frame = cv::imread(file);
        std::cout << "Reading.." << file << " (" << frame.cols << "x" << frame.rows << ")" << std::endl;
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        imageSize = gray.size();

        // 3.2 DEFINIMOS OBJECT POINTS (Nota: puede hacerse fuera del bucle)
        std::vector<cv::Point3f> boardPoints3D;
        defineBoardPoints3D(boardPoints3D);

        // 3.3 DETECTAMOS IMAGE POINTS
        std::vector<cv::Point2f> corners;
        bool success = detectChessboard(gray, corners);

        // 3.4 MOSTRAMOS RESULTADO
        if (success) {
            refineCorners(gray, corners);
            objectPoints.push_back(boardPoints3D);
            imagePoints.push_back(corners);
        }
        cv::drawChessboardCorners(frame, boardSize, corners, success);

        cv::imshow("Image", frame);
        cv::waitKey(0);
    }
    cv::destroyAllWindows();

    if (imagePoints.empty()) {
        std::cerr << "No chessboard detected (is TODO 1 done?)" << std::endl;
        return -1;
    }

    //// 4. CALIBRAMOS LA CAMARA
    cv::Mat cameraMatrix, distCoeffs;
    double err = calibrate(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs);

    //// 5. MOSTRAMOS RESULTADO Y GUARDAMOS A FICHERO
    std::cout << "Reprojection error: " << err << std::endl;
    std::cout << "cameraMatrix : " << cameraMatrix << std::endl;
    std::cout << "distCoeffs : " << distCoeffs << std::endl;

    if (!saveCalibrationToFile("calibration.yml", cameraMatrix, distCoeffs)) {
        std::cerr << "Cannot write calibration.yml" << std::endl;
        return -1;
    }
    std::cout << "Saved calibration.yml" << std::endl;
    return 0;
}
