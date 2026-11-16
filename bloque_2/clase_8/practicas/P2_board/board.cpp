#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>


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


// TODO 1: detectar los marcadores de la imagen
void detectMarkers(const cv::aruco::ArucoDetector &detector, const cv::Mat &img,
                   std::vector<std::vector<cv::Point2f>> &corners, std::vector<int> &ids)
{
    // --- TO DO ---
    // detector.detectMarkers(...);
}

// TODO 2: estimar la pose (rvec, tvec) del board respecto a la cámara.
// Devuelve el número de marcadores usados (0 si no se ha podido estimar).
int estimateBoardPose(const cv::aruco::Board &board, const std::vector<std::vector<cv::Point2f>> &corners,
                      const std::vector<int> &ids, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
                      cv::Mat &rvec, cv::Mat &tvec)
{
    // --- TO DO ---
    std::vector<cv::Point3f> objectPoints;
    std::vector<cv::Point2f> imagePoints;
    // board.matchImagePoints(...);
    // Nota: empareja las esquinas detectadas con las esquinas 3D de esos marcadores en el board
    // cv::solvePnP(...)
    return 0;
}

// TODO 3: dibujar un cubo sobre el plano Z = 0 (y Z positiva)
void drawCube(cv::Mat &img, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs, const cv::Mat &rvec,
              const cv::Mat &tvec, float side)
{
    // --- TO DO ---
    std::vector<cv::Point3f> vertices;          // los 8 vértices del cubo
    std::vector<std::pair<int, int>> edges;     // las 12 aristas: pares de índices de vertices
    // cv::projectPoints(...); // proyectar puntos sobre la imagen
    // cv::line(...); // dibujar lineas del cubo
    // Pista: 4 vértices en la base (Z = 0) y 4 en la tapa (Z = side). Las aristas son las 4 de la base, las 4 de la
    // tapa y las 4 verticales que unen cada vértice de la base con el de la tapa que tiene encima
}

int main(int argc, char *argv[])
{

    //// 1. LEEMOS PARAMETROS
    if (argc != 4) {
        std::cerr << "Usage: video_file|camera_index calibration_file board_file" << std::endl;
        return -1;
    }
    std::string source = argv[1];
    std::string calibrationFile = argv[2];
    std::string boardFile = argv[3];



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
    // Lado de un marcador (el primero del mapa), para la longitud de los ejes y el lado del cubo
    const std::vector<cv::Point3f> &firstMarker = board->getObjPoints()[0];
    float markerLength = (float)cv::norm(firstMarker[1] - firstMarker[0]);

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
    cv::aruco::ArucoDetector detector(board->getDictionary());

    //// 6. PROCESAMOS CADA FRAME
    cv::Mat frame;
    while (cap.read(frame)) {
        // Copia de la imagen para dibujar encima
        cv::Mat canvas = frame.clone();

        // 6.1 DETECTAMOS LOS MARCADORES
        std::vector<std::vector<cv::Point2f>> corners;
        std::vector<int> ids;
        detectMarkers(detector, frame, corners, ids);
        cv::aruco::drawDetectedMarkers(canvas, corners, ids);

        // 6.2 POSE DEL BOARD
        cv::Mat rvec, tvec;
        int used = ids.empty() ? 0 : estimateBoardPose(*board, corners, ids, cameraMatrix, distCoeffs, rvec, tvec);
        if (used > 0) {
            cv::drawFrameAxes(canvas, cameraMatrix, distCoeffs, rvec, tvec, 2 * markerLength);
            drawCube(canvas, cameraMatrix, distCoeffs, rvec, tvec, markerLength);
        }

        // 6.3 MOSTRAR IMAGEN
        cv::putText(canvas, "[space] pause   [ESC] exit", cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 255, 0), 2);
        cv::imshow("P2 board", canvas);
        int key = cv::waitKey(30);
        if (key == 27) break;
        if (key == ' ') cv::waitKey(0);
    }
    cv::destroyAllWindows();
    return 0;
}
