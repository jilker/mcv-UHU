#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>


const cv::aruco::PredefinedDictionaryType DICTIONARY = cv::aruco::DICT_6X6_250;
float markerLength;


bool loadCalibration(const std::string &path, cv::Mat &cameraMatrix, cv::Mat &distCoeffs)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;
    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    return !cameraMatrix.empty() && !distCoeffs.empty();
}


// Object Points (puntos 3D) de las esquinas del marcador
// Centrado en el origen. Plano Z = 0. Eje Z hacia fuera.
std::vector<cv::Point3f> markerPoints3D(float length)
{
    float h = length / 2;
    return {{-h, h, 0}, {h, h, 0}, {h, -h, 0}, {-h, -h, 0}};
}


// TODO 1: detectar los marcadores de la imagen.
// Para cada marcador, corners tiene sus 4 esquinas e ids su identificador (en el mismo orden).
void detectMarkers(const cv::aruco::ArucoDetector &detector, const cv::Mat &img,
                   std::vector<std::vector<cv::Point2f>> &corners, std::vector<int> &ids)
{
    // --- TO DO ---
    // detector.detectMarkers(...);
}

// TODO 2: estimar la pose (rvec, tvec) del marcador respecto a la cámara a partir de sus 4 esquinas.
// Devuelve true si se ha podido estimar.
bool estimateMarkerPose(const std::vector<cv::Point2f> &corners, const cv::Mat &cameraMatrix,
                        const cv::Mat &distCoeffs, cv::Mat &rvec, cv::Mat &tvec)
{
    // --- TO DO ---
    std::vector<cv::Point3f> objectPoints = markerPoints3D(markerLength);
    // return cv::solvePnP(...);
    return false;
}

// TODO 3: dibujar los ejes de coordenadas del marcador.
void drawMarkerAxes(cv::Mat &img, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
                    const cv::Mat &rvec, const cv::Mat &tvec)
{
    // --- TO DO ---
    // cv::drawFrameAxes(...);
    // Nota: Usar como longitud de los ejes markerLength / 2
}

// TODO 4: distancia (en metros) de la cámara al centro del marcador.
double distanceToCamera(const cv::Mat &tvec)
{
    // --- TO DO ---
    // Pista: tvec es la posición del centro del marcador en el sistema de la cámara
    // return ...;
    return 0;
}

int main(int argc, char *argv[])
{

    //// 1. LEEMOS PARAMETROS
    if (argc != 4) {
        std::cerr << "Usage: video_file|camera_index calibration_file marker_length_m" << std::endl;
        return -1;
    }
    std::string source = argv[1];
    std::string calibrationFile = argv[2];
    markerLength = std::atof(argv[3]);
    if (markerLength <= 0) {
        std::cerr << "Invalid marker length: " << argv[3] << std::endl;
        return -1;
    }



    //// 2. LEEMOS LA CALIBRACION
    cv::Mat cameraMatrix, distCoeffs;
    if (!loadCalibration(calibrationFile, cameraMatrix, distCoeffs)) {
        std::cerr << "Cannot read " << calibrationFile << std::endl;
        return -1;
    }

    //// 3. ABRIMOS EL VIDEO (O LA CAMARA, SI SE INDICA SU NUMERO)
    cv::VideoCapture cap;
    if (!source.empty() && std::all_of(source.begin(), source.end(), ::isdigit))
        cap.open(std::stoi(source));
    else
        cap.open(source);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open " << source << std::endl;
        return -1;
    }

    //// 4. CREAMOS EL DETECTOR DE MARCADORES
    cv::aruco::ArucoDetector detector(cv::aruco::getPredefinedDictionary(DICTIONARY));

    //// 5. PROCESAMOS CADA FRAME
    cv::Mat frame;
    while (cap.read(frame)) {
        // Copia de la imagen para dibujar encima
        cv::Mat canvas = frame.clone();

        // 5.1 DETECTAMOS LOS MARCADORES
        std::vector<std::vector<cv::Point2f>> corners;
        std::vector<int> ids;
        detectMarkers(detector, frame, corners, ids);
        cv::aruco::drawDetectedMarkers(canvas, corners, ids);

        // 5.2 POSE DE CADA MARCADOR
        std::string distances;
        for (size_t i = 0; i < ids.size(); i++) {
            cv::Mat rvec, tvec;
            if (!estimateMarkerPose(corners[i], cameraMatrix, distCoeffs, rvec, tvec)) continue;
            drawMarkerAxes(canvas, cameraMatrix, distCoeffs, rvec, tvec);

            char text[32];
            std::snprintf(text, sizeof(text), "   id %d: %.2f m", ids[i], distanceToCamera(tvec));
            distances += text;
        }
        if (!distances.empty()) std::cout << "Distances:" << distances << std::endl;

        // 5.3 MOSTRAR IMAGEN
        cv::putText(canvas, "[space] pause   [ESC] exit", cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 255, 0), 2);
        cv::imshow("P1 marcador", canvas);
        int key = cv::waitKey(30);
        if (key == 27) break;
        if (key == ' ') cv::waitKey(0);
    }
    cv::destroyAllWindows();
    return 0;
}
