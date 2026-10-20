#include <algorithm>
#include <cmath>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <utility>
#include <vector>

const std::string WINDOW = "P1 fit";
const int WIDTH = 800, HEIGHT = 600;

/**
 * Ajusta una recta a points con cv::fitLine.
 *
 * @param points  puntos (x, y), en píxeles; al menos 2
 * @return dos puntos (p1, p2) de la recta, uno a cada lado para que cv::line la dibuje de borde a borde
 */
std::pair<cv::Point, cv::Point> lineFit(const std::vector<cv::Point>& points) {
    //// TODO: CODE HERE
    cv::Vec4f line;
    cv::fitLine(points, line, cv::DIST_L2, 0, 0.01, 0.01);
    float vx = line[0], vy = line[1], x0 = line[2], y0 = line[3];
    cv::Point p1((int)(x0 - 1000 * vx), (int)(y0 - 1000 * vy));  
    cv::Point p2((int)(x0 + 1000 * vx), (int)(y0 + 1000 * vy));
    //// END TODO
    return {p1, p2};
}

/**
 * Ajusta a points el polinomio de grado 4 y = c0 + c1 x + c2 x^2 + c3 x^3 + c4 x^4 con cv::solve.
 *
 * Con 5 puntos la curva pasa exactamente por todos, con más, es el resultado de mínimos cuadrados.
 *
 * PISTA: usar cv::solve con cv::DECOMP_SVD.
 *
 * @param points  puntos (x, y), en píxeles; al menos 5
 * @return los coeficientes c0 ... c4, en una matriz CV_64F de 5 filas x 1 columna
 */
cv::Mat polyFit(const std::vector<cv::Point>& points) {
    //// TODO: CODE HERE
    int n = static_cast<int>(points.size());
    cv::Mat A = cv::Mat::zeros(n, 5, CV_64F);
    cv::Mat b = cv::Mat::zeros(n, 1, CV_64F);
    for (int i = 0; i < n; i++) {
        double x = points.at(i).x;
        double y = points.at(i).y;
        for (int power = 0; power < 5; power++) {
            A.at<double>(i, power) = std::pow(x, power);
        }
        b.at<double>(i, 0) = y;
    }
    cv::Mat c;
    cv::solve(A, b, c, cv::DECOMP_SVD);
    //// END TODO
    return c;
}

/**
 * Un punto del polinomio de coeficientes c por cada columna del lienzo.
 *
 * @param c  los coeficientes c0 ... c4, en una matriz CV_64F de 5 filas x 1 columna (ver polyFit)
 * @return un punto (px, y) por cada columna px de 0 a WIDTH - 1, con y limitada entre -HEIGHT y
 *         2 * HEIGHT: fuera del lienzo no se ve
 */
std::vector<cv::Point> polyCurve(const cv::Mat& c) {
    std::vector<cv::Point> curve;
    for (int px = 0; px < WIDTH; px++) {
        double y = 0;
        for (int power = 0; power < 5; power++) {  // y = c0 + c1 x + c2 x^2 + c3 x^3 + c4 x^4
            y = y + c.at<double>(power) * std::pow(px, power);
        }
        y = std::max((double)-HEIGHT, std::min(2.0 * HEIGHT, y));
        curve.push_back(cv::Point(px, (int)y));
    }
    return curve;
}

//// Ratón: los puntos clicados se guardan en points
std::vector<cv::Point> points;

// OpenCV la llama cada vez que se hace algo con el ratón sobre la ventana
void mouseClicked(int event, int x, int y, int, void*) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        points.push_back(cv::Point(x, y));
        std::cout << "Point added at (" << x << ", " << y << "): " << points.size() << " points" << std::endl;
    } else if (event == cv::EVENT_RBUTTONDOWN && !points.empty()) {
        points.pop_back();
        std::cout << "Last point removed: " << points.size() << " points" << std::endl;
    }
}

int main(int argc, char** argv) {
    //// Línea de comandos: line o poly
    std::string mode = "";
    if (argc > 1) {
        mode = argv[1];
    }
    if (mode != "line" && mode != "poly") {
        std::cerr << "Usage: ./build/ajuste line | poly" << std::endl;
        return 1;
    }

    cv::namedWindow(WINDOW);
    cv::setMouseCallback(WINDOW, mouseClicked);

    while (true) {
        cv::Mat canvas = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC3);
        for (const cv::Point& p : points) {
            cv::circle(canvas, p, 5, cv::Scalar(255, 255, 255), -1);
        }
        std::string title = mode + ": not enough points";

        if (mode == "line" && points.size() >= 2) {
            //// 1. Recta con cv::fitLine
            auto [p1, p2] = lineFit(points);
            cv::line(canvas, p1, p2, cv::Scalar(0, 0, 255), 2);
            title = "line";

        } else if (mode == "poly" && points.size() >= 5) {
            //// 2. Polinomio de grado 4 con cv::solve
            std::vector<cv::Point> curve = polyCurve(polyFit(points));
            cv::polylines(canvas, curve, false, cv::Scalar(0, 0, 255), 2);
            title = "poly degree 4";
        }

        cv::imshow(WINDOW, canvas);
        cv::setWindowTitle(WINDOW, title + "   (" + std::to_string(points.size()) + " points)");
        int key = cv::waitKey(30);
        if (key == 27) {
            break;
        } else if (key == 'c') {
            points.clear();
            std::cout << "All points cleared" << std::endl;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
