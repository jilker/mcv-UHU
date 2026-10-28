#include <opencv2/core.hpp>
#include <opencv2/geometry.hpp>  
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

const std::string WINDOW = "P1 ransac";
const int WIDTH = 800, HEIGHT = 600;
const double P = 0.99;

/**
 * Ajusta una recta a points con cv::fitLine
 */
std::pair<cv::Point, cv::Point> lineFit(const std::vector<cv::Point2f>& points) {
    cv::Vec4f line;
    cv::fitLine(points, line, cv::DIST_L2, 0, 0.01, 0.01);
    float vx = line[0], vy = line[1], x0 = line[2], y0 = line[3];
    cv::Point p1((int)(x0 - 1000 * vx), (int)(y0 - 1000 * vy));
    cv::Point p2((int)(x0 + 1000 * vx), (int)(y0 + 1000 * vy));
    return {p1, p2};
}

/**
 * Distancia perpendicular del punto x a la recta que pasa por p con normal unitaria normal: |(x - p) · n|.
 */
inline float distanceToLine(const cv::Point2f& x, const cv::Point2f& p, const cv::Point2f& normal) {
    return std::abs((x - p).dot(normal));
}

/**
 * Busca la recta que tiene más inliers usando RANSAC.
 *
 * @param points      puntos (x, y)
 * @param t           umbral de inlier, en píxeles
 * @param iterations  número de intentos de RANSAC
 * @param rng         generador de números aleatorios
 * @param inliers     (salida) un bool por punto, true en los inliers
 * @param sample      (salida) los 2 puntos que definen la recta con más inliers
 * @return false si no se hace ningún intento (iterations = 0). Los puntos son todos distintos
 *         (main no deja añadir uno repetido), así que cualquier par define una recta
 */
bool ransacLine(const std::vector<cv::Point2f>& points, float t, int iterations, cv::RNG& rng,
                std::vector<bool>& inliers, std::pair<cv::Point2f, cv::Point2f>& sample) {
    inliers.assign(points.size(), false);
    bool found = false;
    int n = (int)points.size();
    int best = 0;
    //// TODO: CODE HERE
    //// END TODO
    return found;
}

/**
 * Intentos necesarios para que, con probabilidad p, al menos una muestra tenga solo inliers.
 *
 * N = log(1 - p) / log(1 - (1 - e)^s), redondeado hacia arriba.
 *
 * @param p  probabilidad de éxito que se quiere, por ejemplo 0.99
 * @param e  fracción de outliers, entre 0 y 1 (excluido)
 * @param s  puntos de cada muestra (2 para una recta)
 * @return N, al menos 1
 */
int numIterations(double p, double e, int s) {
    int n = 1;
    if (e <= 0) {
        return 1;
    }
    //// TODO: CODE HERE
    //// END TODO
    return std::max(1, n);
}

// Dibuja la recta que pasa por los 2 puntos de sample y su banda de ±t píxeles
void drawBand(cv::Mat& canvas, const std::pair<cv::Point2f, cv::Point2f>& sample, float t,
              const cv::Scalar& color) {
    cv::Point2f p = sample.first, q = sample.second;
    cv::Point2f d = (q - p) / cv::norm(q - p);
    cv::Point2f n(-d.y, d.x);
    for (float offset : {0.0f, t, -t}) {
        cv::Point2f a = p + offset * n - 1000 * d;
        cv::Point2f b = p + offset * n + 1000 * d;
        cv::line(canvas, a, b, color, 1, cv::LINE_AA);
    }
    cv::circle(canvas, p, 10, color, 2);
    cv::circle(canvas, q, 10, color, 2);
}

std::vector<cv::Point> points;
bool addPoint(int x, int y) {
    if (std::find(points.begin(), points.end(), cv::Point(x, y)) != points.end()) {
        return false;
    }
    points.push_back(cv::Point(x, y));
    return true;
}

// OpenCV la llama cada vez que se hace algo con el ratón sobre la ventana
void mouseClicked(int event, int x, int y, int, void*) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (addPoint(x, y)) {
            std::cout << "Point added at (" << x << ", " << y << "): " << points.size() << " points" << std::endl;
        } else {
            std::cout << "Point (" << x << ", " << y << ") already added" << std::endl;
        }
    } else if (event == cv::EVENT_RBUTTONDOWN && !points.empty()) {
        points.pop_back();
        std::cout << "Last point removed: " << points.size() << " points" << std::endl;
    }
}

int main() {
    cv::namedWindow(WINDOW);
    cv::setMouseCallback(WINDOW, mouseClicked);
    cv::createTrackbar("t (px)", WINDOW, nullptr, 100);
    cv::setTrackbarPos("t (px)", WINDOW, 15);
    cv::createTrackbar("iterations", WINDOW, nullptr, 200);
    cv::setTrackbarPos("iterations", WINDOW, 20);

    int seed = 0;
    cv::RNG noise(cv::getTickCount());

    while (true) {
        float t = std::max(1, cv::getTrackbarPos("t (px)", WINDOW));
        int iterations = std::max(1, cv::getTrackbarPos("iterations", WINDOW));
        cv::Mat canvas = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC3);
        std::string title = "not enough points";

        std::vector<cv::Point2f> pts(points.begin(), points.end());
        std::vector<cv::Scalar> color(pts.size(), cv::Scalar(255, 255, 255));   // blanco: outlier

        if (pts.size() >= 2) {
            //// 1. Mínimos cuadrados con todos los puntos, en naranja
            auto [p1, p2] = lineFit(pts);
            cv::line(canvas, p1, p2, cv::Scalar(0, 140, 255), 2, cv::LINE_AA);

            //// 2. RANSAC: mejor hipótesis con su banda (gris) y ajuste final con sus inliers (verde)
            cv::RNG rng(seed);
            std::vector<bool> inliers;
            std::pair<cv::Point2f, cv::Point2f> sample;
            title = "no line found";
            if (ransacLine(pts, t, iterations, rng, inliers, sample)) {
                drawBand(canvas, sample, t, cv::Scalar(128, 128, 128));
                std::vector<cv::Point2f> inliers_points, outliers_points;
                for (size_t i = 0; i < pts.size(); i++) {
                    if (inliers[i]) {
                        inliers_points.push_back(pts[i]);
                        color[i] = cv::Scalar(0, 255, 0);
                    } else {
                        outliers_points.push_back(pts[i]);
                    }
                }
                auto [q1, q2] = lineFit(inliers_points);
                cv::line(canvas, q1, q2, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                double e = double(outliers_points.size()) / pts.size();
                char text[100];
                std::snprintf(text, sizeof(text), "inliers %zu/%zu   N needed (p = %.2f, e = %.2f) = %d",
                              inliers_points.size(), pts.size(), P, e, numIterations(P, e, 2));
                title = text;
            }
        }

        for (size_t i = 0; i < points.size(); i++) {
            cv::circle(canvas, points[i], 5, color[i], -1);
        }

        cv::imshow(WINDOW, canvas);
        cv::setWindowTitle(WINDOW, title + "   (seed " + std::to_string(seed) + ")");
        int key = cv::waitKey(30);
        if (key == 27) {
            break;
        } else if (key == 'c') {
            points.clear();
            std::cout << "All points cleared" << std::endl;
        } else if (key == 'o') {
            int added = 0;
            while (added < 5) {
                added += addPoint(noise.uniform(0, WIDTH), noise.uniform(0, HEIGHT));
            }
            std::cout << "5 random outliers added: " << points.size() << " points" << std::endl;
        } else if (key == 'r') {
            seed++;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
