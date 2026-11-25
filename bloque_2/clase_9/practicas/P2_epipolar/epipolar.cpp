#include <opencv2/calib.hpp>
#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stereo.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int imageWidth = 0;
const std::vector<cv::Scalar> COLORS = {cv::Scalar(0, 0, 255),   cv::Scalar(0, 255, 0),   cv::Scalar(255, 0, 0),
                                        cv::Scalar(0, 255, 255), cv::Scalar(255, 0, 255), cv::Scalar(255, 255, 0)};


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


// Dibuja la recta a·x + b·y + c = 0 de lado a lado de la imagen
void drawLine(cv::Mat &img, const cv::Vec3f &line, const cv::Scalar &color)
{
    float a = line[0], b = line[1], c = line[2];
    if (std::abs(b) < 1e-9) return;
    int w = img.cols;
    cv::Point p1(0, cvRound(-c / b));
    cv::Point p2(w - 1, cvRound(-(c + a * (w - 1)) / b));
    cv::line(img, p1, p2, color, 1, cv::LINE_AA);
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


// Aplica la rectificación a una imagen: como undistort en la clase 7, pero además gira la cámara (Rrect) y usa su
// nueva matriz de proyección (P)
cv::Mat rectifyImage(const cv::Mat &img, const cv::Mat &K, const cv::Mat &D, const cv::Mat &Rrect, const cv::Mat &P)
{
    cv::Mat mapX, mapY, rectified;
    cv::initUndistortRectifyMap(K, D, Rrect, P, img.size(), CV_32FC1, mapX, mapY);
    cv::remap(img, rectified, mapX, mapY, cv::INTER_LINEAR);
    return rectified;
}

// TODO 1: calcular la matriz fundamental F a partir de la calibración: F = Kr^-T · [t]x · R · Kl^-1
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

// TODO 2: calcular la línea epipolar (a, b, c) en la imagen derecha que corresponde a un punto de la izquierda.
cv::Vec3f epipolarLine(const cv::Mat &F, const cv::Point2f &point)
{
    // --- TO DO ---
    std::vector<cv::Point2f> points = {point};
    std::vector<cv::Vec3f> lines;
    // cv::computeCorrespondEpilines(...);
    // Nota: rellena una línea (a, b, c) por cada punto. whichImage = 1 porque los puntos son de la imagen izquierda
    // Nota: devolver la primera y única línea (lines[0])
    return cv::Vec3f(0, 0, 0);
}

// TODO 3: rectificación stereo: calcular las rotaciones R1 y R2 que alinean las dos cámaras y sus nuevas matrices de
// proyección P1 y P2
void computeRectification(const cv::Mat &Kl, const cv::Mat &Dl, const cv::Mat &Kr, const cv::Mat &Dr, const cv::Mat &R,
                          const cv::Mat &t, cv::Size imageSize, cv::Mat &R1, cv::Mat &R2, cv::Mat &P1, cv::Mat &P2)
{
    // --- TO DO ---
    cv::Mat Q;
    // cv::stereoRectify(...);
    // Nota: Usar flags = cv::CALIB_ZERO_DISPARITY y alpha = -1 (escala por defecto)
}

// Matriz fundamental del par rectificado
cv::Mat rectifiedFundamental(const cv::Mat &P1, const cv::Mat &P2)
{
    cv::Mat K1 = P1.colRange(0, 3).clone();
    cv::Mat K2 = P2.colRange(0, 3).clone();
    cv::Mat t = K2.inv() * P2.col(3);
    return computeFundamental(K1, K2, cv::Mat::eye(3, 3, CV_64F), t);
}

// Dibuja izquierda | derecha: los puntos pinchados en la izquierda y sus líneas epipolares en la derecha
cv::Mat drawEpipolar(const cv::Mat &imgL, const cv::Mat &imgR, const cv::Mat &F, const std::vector<cv::Point> &points)
{
    cv::Mat canvasL = imgL.clone();
    cv::Mat canvasR = imgR.clone();
    for (size_t j = 0; j < points.size(); j++) {
        const cv::Scalar &color = COLORS[j % COLORS.size()];
        cv::circle(canvasL, points[j], 5, color, 2);
        drawLine(canvasR, epipolarLine(F, cv::Point2f(points[j])), color);
    }
    cv::Mat canvas;
    cv::hconcat(canvasL, canvasR, canvas);
    cv::putText(canvas, "[click left] point   [c] clear   [ESC] exit", cv::Point(10, 25),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
    return canvas;
}

// Clic izquierdo en la imagen izquierda: añade el punto a la lista de la ventana (userdata)
void onMouse(int event, int x, int y, int flags, void *userdata)
{
    std::vector<cv::Point> *points = (std::vector<cv::Point> *)userdata;
    if (event == cv::EVENT_LBUTTONDOWN && x < imageWidth)
        points->push_back(cv::Point(x, y));
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
    imageWidth = frameL.cols;


    //// 3. QUITAMOS LA DISTORSIÓN (trabajamos sin distorsión)
    cv::Mat undistortedL, undistortedR;
    cv::undistort(frameL, undistortedL, Kl, Dl);
    cv::undistort(frameR, undistortedR, Kr, Dr);


    //// 4. MATRIZ FUNDAMENTAL (a partir de K, R y t)
    cv::Mat F = computeFundamental(Kl, Kr, R, t);
    if (F.empty()) {
        std::cerr << "F is empty (is TODO 1 done?)" << std::endl;
        return -1;
    }
    std::cout << "F:\n" << F << std::endl;


    //// 5. RECTIFICAMOS EL PAR Y CALCULAMOS SU MATRIZ FUNDAMENTAL
    cv::Mat R1, R2, P1, P2, rectifiedL, rectifiedR, Frectified;
    computeRectification(Kl, Dl, Kr, Dr, R, t, frameL.size(), R1, R2, P1, P2);
    if (R1.empty() || R2.empty() || P1.empty() || P2.empty()) {
        std::cerr << "Pair not rectified (is TODO 3 done?)" << std::endl;
        rectifiedL = undistortedL;
        rectifiedR = undistortedR;
        Frectified = F;
    } else {
        // aplicar la rectificación (homografía + corrección de distorsión)
        rectifiedL = rectifyImage(frameL, Kl, Dl, R1, P1);
        rectifiedR = rectifyImage(frameR, Kr, Dr, R2, P2);
        Frectified = rectifiedFundamental(P1, P2);
    }


    //// 6. DIBUJAR LÍNEAS EPIPOLARES DE LOS PUNTOS CLICKADOS
    // Una ventana sin rectificar y otra rectificada, cada una con sus puntos
    std::vector<cv::Point> clickedPoints, clickedPointsRectified;
    cv::namedWindow("epipolar");
    cv::setMouseCallback("epipolar", onMouse, &clickedPoints);
    cv::namedWindow("rectified");
    cv::setMouseCallback("rectified", onMouse, &clickedPointsRectified);
    while (true) {
        cv::imshow("epipolar", drawEpipolar(undistortedL, undistortedR, F, clickedPoints));
        cv::imshow("rectified", drawEpipolar(rectifiedL, rectifiedR, Frectified, clickedPointsRectified));
        int key = cv::waitKey(30);
        if (key == 27) break;
        if (key == 'c') {
            clickedPoints.clear();
            clickedPointsRectified.clear();
        }
    }
    cv::destroyAllWindows();

    return 0;
}
