#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

const std::string IMAGE = "../data/chessboard.png";
const std::string WINDOW = "P2 grid";

/**
 * Rectas de la imagen de bordes con cv::HoughLines.
 *
 * @param edges     imagen de bordes (CV_8U, 0 o 255)
 * @param rhoRes    tamaño de las celdas del acumulador en rho, en píxeles
 * @param thetaRes  tamaño de las celdas del acumulador en theta, en radianes
 * @param votes     píxeles de borde que necesita una recta para aceptarla
 * @param minTheta, maxTheta  rango de theta de las rectas que se buscan, en radianes
 * @return las rectas (rho, theta), ordenadas por votos: primero la más votada; vacío si no
 *         encuentra ninguna
 */
std::vector<cv::Vec2f> findLines(const cv::Mat& edges, double rhoRes, double thetaRes, int votes,
                                 double minTheta, double maxTheta) {
    //// TODO: CODE HERE
    std::vector<cv::Vec2f> lines;
    cv::HoughLines(edges, lines, rhoRes, thetaRes, votes, 0, 0, minTheta, maxTheta);
    //// END TODO
    return lines;
}

/**
 * Dibuja sobre img la recta x cos(theta) + y sin(theta) = rho, de borde a borde.
 *
 * @param img    imagen sobre la que se dibuja, en color (BGR)
 * @param rho    distancia de la recta al origen (0, 0), en píxeles
 * @param theta  ángulo de la normal de la recta, en radianes
 * @param color  color de la recta, en BGR
 */
void drawLine(cv::Mat& img, double rho, double theta, const cv::Scalar& color) {
    //// TODO: CODE HERE
    int length = img.cols + img.rows;      // más que la diagonal de la imagen
    double x0 = rho * std::cos(theta); 
    double y0 = rho * std::sin(theta);
    // El vector normal a la recta es (cos(theta), sin(theta)), queremos el vector perpendicular -> + 90 grados
    // cos(theta + 90) = -sin(theta)  || sin(theta + 90) = cos(theta) 
    double dx = -std::sin(theta);
    double dy = std::cos(theta);
    cv::Point p1((int)(x0 - length * dx), (int)(y0 - length * dy));
    cv::Point p2((int)(x0 + length * dx), (int)(y0 + length * dy));
    cv::line(img, p1, p2, color, 2);
    //// END TODO
}

/**
 * Punto de cruce de dos rectas, con cv::solve.
 *
 * @param line1, line2  rectas (rho, theta), como las de findLines
 * @param point         salida: el cruce (x, y), si lo hay
 * @return true si hay cruce, false si las rectas son paralelas
 */
bool crossing(const cv::Vec2f& line1, const cv::Vec2f& line2, cv::Point2d& point) {
    //// TODO: CODE HERE
    bool found = false;
    // Dos rectas: dos ecuaciones, dos incógnitas (x, y)
    double rho1 = line1[0], theta1 = line1[1];
    double rho2 = line2[0], theta2 = line2[1];
    cv::Mat_<double> A({2, 2}, {std::cos(theta1), std::sin(theta1),
                                std::cos(theta2), std::sin(theta2)});
    cv::Mat_<double> b({2, 1}, {rho1, rho2});
    cv::Mat p;
    bool ok = cv::solve(A, b, p);
    if (ok) {
        point = cv::Point2d(p.at<double>(0), p.at<double>(1));
        found = true;
    }
    //// END TODO
    return found;
}

int main() {
    //// Cargar la imagen: en color para dibujar encima, en escala de grises para buscar los bordes
    cv::Mat img = cv::imread(IMAGE);
    if (img.empty()) {
        std::cerr << "Error: could not read the image" << std::endl;
        return 1;
    }
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    int height = gray.rows, width = gray.cols;

    //// Trackbars (nullptr: la posición se lee con getTrackbarPos)
    // WINDOW_NORMAL + resizeWindow: las ventanas empiezan a la mitad del tamaño de la foto y se pueden agrandar
    // con el ratón (con GTK, nunca por debajo del tamaño que tienen al mostrar la imagen por primera vez)
    cv::namedWindow(WINDOW, cv::WINDOW_NORMAL);
    cv::namedWindow("edges", cv::WINDOW_NORMAL);
    cv::resizeWindow(WINDOW, width / 2, height / 2 + 280);     // + 280: las 5 trackbars van dentro de la ventana
    cv::resizeWindow("edges", width / 2, height / 2);
    cv::createTrackbar("canny low", WINDOW, nullptr, 500);    // Canny: umbral bajo, los bordes débiles pegados a uno fuerte se quedan
    cv::createTrackbar("canny high", WINDOW, nullptr, 500);   // Canny: umbral alto, por encima es borde seguro
    cv::createTrackbar("rho res", WINDOW, nullptr, 200);    // tamaño de las celdas del acumulador en rho, en décimas de px
    cv::createTrackbar("theta res", WINDOW, nullptr, 50);   // tamaño de las celdas del acumulador en theta, en décimas de grado
    cv::createTrackbar("votes", WINDOW, nullptr, 3000);     // píxeles de borde que necesita una recta para aceptarla
    cv::setTrackbarPos("canny low", WINDOW, 50);
    cv::setTrackbarPos("canny high", WINDOW, 150);
    cv::setTrackbarPos("rho res", WINDOW, 10);
    cv::setTrackbarPos("theta res", WINDOW, 10);
    cv::setTrackbarPos("votes", WINDOW, 350);
    cv::setTrackbarMin("rho res", WINDOW, 1);
    cv::setTrackbarMin("theta res", WINDOW, 1);
    cv::setTrackbarMin("votes", WINDOW, 1);

    while (true) {
        double rhoRes = cv::getTrackbarPos("rho res", WINDOW) / 10.0;                          // de décimas de px a px
        double thetaRes = (cv::getTrackbarPos("theta res", WINDOW) / 10.0) * (CV_PI / 180.0);  // de décimas de grado a radianes
        int votes = cv::getTrackbarPos("votes", WINDOW);
        int cannyLow = cv::getTrackbarPos("canny low", WINDOW);
        int cannyHigh = cv::getTrackbarPos("canny high", WINDOW);
        cv::Mat result = img.clone();

        //// 1. Bordes
        cv::Mat blurred, edges;
        cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
        cv::Canny(blurred, edges, cannyLow, cannyHigh);
        cv::dilate(edges, edges, cv::Mat());    // kernel vacío: 3x3

        //// 2. Rectas. theta es el ángulo de la normal de la recta: 0 para una recta vertical, 90 grados para una horizontal
        std::vector<cv::Vec2f> vertical = findLines(edges, rhoRes, thetaRes, votes, -CV_PI / 4, CV_PI / 4);        // theta de -45 a 45 grados
        std::vector<cv::Vec2f> horizontal = findLines(edges, rhoRes, thetaRes, votes, CV_PI / 4, 3 * CV_PI / 4);   // theta de 45 a 135 grados
        for (const cv::Vec2f& line : vertical) {
            drawLine(result, line[0], line[1], cv::Scalar(0, 0, 255));
        }
        for (const cv::Vec2f& line : horizontal) {
            drawLine(result, line[0], line[1], cv::Scalar(0, 0, 255));
        }

        //// 3. Cruces: cada recta vertical con cada horizontal, solo los que caen dentro de la imagen
        std::vector<cv::Point2d> crossings;
        for (const cv::Vec2f& v : vertical) {
            for (const cv::Vec2f& h : horizontal) {
                cv::Point2d point;
                if (crossing(v, h, point) && 0 <= point.x && point.x < width && 0 <= point.y && point.y < height) {
                    crossings.push_back(point);
                }
            }
        }
        for (const cv::Point2d& c : crossings) {
            cv::circle(result, cv::Point((int)c.x, (int)c.y), 6, cv::Scalar(255, 255, 0), -1);
        }

        cv::imshow("edges", edges);
        cv::imshow(WINDOW, result);
        std::string title = "vertical: " + std::to_string(vertical.size()) + "   horizontal: " + std::to_string(horizontal.size())
                          + "   crossings: " + std::to_string(crossings.size());
        cv::setWindowTitle(WINDOW, title);
        if (cv::waitKey(30) == 27) {
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
