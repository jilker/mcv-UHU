#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "models.hpp"

// Lado de la base del modelo 3D, en metros
const float MODEL_SIZE = 0.12f;


bool loadCalibration(const std::string &path, cv::Mat &cameraMatrix, cv::Mat &distCoeffs)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;
    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    return !cameraMatrix.empty() && !distCoeffs.empty();
}

// Lee un board (mapa de marcadores) de fichero: diccionario y, para cada marcador, su id y sus 4 esquinas
bool loadBoard(const std::string &path, cv::Ptr<cv::aruco::Board> &board)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;
    int dictionary = (int)fs["dictionary"];
    std::vector<int> ids;
    std::vector<std::vector<cv::Point3f>> objPoints;
    for (const cv::FileNode &marker : fs["markers"]) {
        cv::Mat corners;
        marker["corners"] >> corners;
        if (corners.rows != 4 || corners.cols != 3) return false;
        corners.convertTo(corners, CV_32F);
        std::vector<cv::Point3f> points;
        for (int i = 0; i < 4; i++)
            points.push_back(cv::Point3f(corners.at<float>(i, 0), corners.at<float>(i, 1), corners.at<float>(i, 2)));
        ids.push_back((int)marker["id"]);
        objPoints.push_back(points);
    }
    if (ids.empty()) return false;
    board = cv::makePtr<cv::aruco::Board>(objPoints, cv::aruco::getPredefinedDictionary(dictionary), ids);
    return true;
}

int main(int argc, char *argv[])
{

    //// 1. LEEMOS PARAMETROS
    if (argc != 5) {
        std::cerr << "Usage: video_file|camera_index calibration_file board_file model_id" << std::endl;
        for (size_t i = 0; i < MODEL_NAMES.size(); i++)
            std::cerr << "    model_id " << i << ": " << MODEL_NAMES[i] << std::endl;
        return -1;
    }
    std::string source = argv[1];
    std::string calibrationFile = argv[2];
    std::string boardFile = argv[3];
    int modelId = std::atoi(argv[4]);
    if (modelId < 0 || modelId >= (int)MODEL_NAMES.size()) {
        std::cerr << "Invalid model_id: " << argv[4] << std::endl;
        return -1;
    }

    //// 2. LEEMOS LA CALIBRACION
    cv::Mat cameraMatrix, distCoeffs;
    if (!loadCalibration(calibrationFile, cameraMatrix, distCoeffs)) {
        std::cerr << "Cannot read " << calibrationFile << std::endl;
        return -1;
    }

    //// 3. LEEMOS EL BOARD
    cv::Ptr<cv::aruco::Board> board;
    if (!loadBoard(boardFile, board)) {
        std::cerr << "Cannot read " << boardFile << std::endl;
        return -1;
    }

    //// 4. ABRIMOS EL VIDEO (O LA CAMARA, SI SE INDICA SU NUMERO)
    cv::VideoCapture cap;
    if (!source.empty() && std::all_of(source.begin(), source.end(), ::isdigit))
        cap.open(std::stoi(source));
    else
        cap.open(source);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open " << source << std::endl;
        return -1;
    }

    //// 5. CREAMOS EL DETECTOR DE MARCADORES
    // TODO 1: crear el detector de marcadores con el diccionario del board (como en P2).
    // --- TO DO ---
    // cv::aruco::ArucoDetector detector(...);
    // Pista: board->getDictionary() devuelve el diccionario del board

    //// 6. PROCESAMOS CADA FRAME
    cv::Mat frame;
    while (cap.read(frame)) {
        // Copia de la imagen para dibujar encima
        cv::Mat canvas = frame.clone();

        // 6.1 DETECTAMOS LOS MARCADORES Y ESTIMAMOS LA POSE DEL BOARD
        // TODO 2: detectar los marcadores y estimar la pose (rvec, tvec) del board (como en P2).
        bool poseFound = false;
        // --- TO DO ---

        // 6.2 DIBUJAMOS EL MODELO 3D SOBRE EL BOARD
        if (poseFound)
        {
            // Modelo en su sistema de referencia local (cambia con cada llamada según animación)
            Model model = getModel(modelId, MODEL_SIZE);

            // TODO 3: proyectar los vértices del modelo en la imagen.
            // --- TO DO ---
            std::vector<cv::Point2f> pointsReprojected;
            // cv::projectPoints(...);
            // Nota: model.vertices ya está en el sistema de referencia del board, así que se proyecta con su pose

            // TODO 4: dibujar las aristas del modelo.
            // --- TO DO ---
            // Cada elemento i de model.edges es un par (a, b) con los índices de los dos vértices que hay que unir:
            // unir pointsReprojected[a] y pointsReprojected[b] con cv::line(...) en canvas, con el color model.colors[i]
        }

        // 6.3 MOSTRAR IMAGEN
        cv::putText(canvas, "[space] pause   [ESC] exit", cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 255, 0), 2);
        cv::imshow("P3 ar", canvas);
        int key = cv::waitKey(30);
        if (key == 27) break;
        if (key == ' ') cv::waitKey(0);
    }
    cv::destroyAllWindows();
    return 0;
}
