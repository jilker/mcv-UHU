#include <algorithm>
#include <iostream>
#include <numeric>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>

const std::string IMAGE = "../data/nfl_field.jpg";
const std::string TEMPLATE = "../data/plantilla.png";
const std::vector<double> SCALES = {0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2};
const bool ROTATE_TEMPLATE = false;
const size_t MAX_CANDIDATES = 2000;
const std::string WINDOW = "P1 detections";
const std::string WINDOW_RESPONSE = "P1 response";
const int DISPLAY_HEIGHT = 520;
const int FONT = cv::FONT_HERSHEY_SIMPLEX;

void show(const std::string& window, const cv::Mat& img) {
    cv::Mat shown = img;
    if (img.rows > DISPLAY_HEIGHT) {
        double scale = double(DISPLAY_HEIGHT) / img.rows;
        cv::resize(img, shown, cv::Size(), scale, scale, cv::INTER_AREA);
    }
    cv::imshow(window, shown);
}

cv::Size scaledSize(const cv::Mat& templ, double scale) {
    return cv::Size(cvRound(templ.cols * scale), cvRound(templ.rows * scale));
}

/**
 * Mapa de respuesta de la plantilla escalada sobre la imagen.
 *
 * La plantilla se escala a scaledSize(templ, scale) y se desliza por toda la imagen.
 * En cada posición se mide el parecido con la correlación normalizada (cv::TM_CCOEFF_NORMED)
 * 1 es idéntica, 0 no se parecen.
 *
 * PISTA: cv::resize cv::matchTemplate
 * @param gray   imagen en gris
 * @param templ  plantilla en gris, a su tamaño original
 * @param scale  factor de escala de la plantilla
 * @return salida de cv::matchTemplate
 */
cv::Mat match(const cv::Mat& gray, const cv::Mat& templ, double scale) {
    cv::Mat response;
    //// TODO: CODE HERE
    cv::Mat resized;
    cv::resize(templ, resized, scaledSize(templ, scale), 0, 0, cv::INTER_AREA);
    cv::matchTemplate(gray, resized, response, cv::TM_CCOEFF_NORMED);
    //// END TODO
    return response;
}

/**
 * Intersección sobre unión de dos BBs: área común / área de las dos juntas.
 *
 * @return entre 0 (no se tocan) y 1 (son la misma BB)
 */
double iou(const cv::Rect& a, const cv::Rect& b) {
    double result = 0;
    //// TODO: CODE HERE
    int aXMin = a.x, aYMin = a.y;
    int aXMax = a.x + a.width, aYMax = a.y + a.height;
    int bXMin = b.x, bYMin = b.y;
    int bXMax = b.x + b.width, bYMax = b.y + b.height;

    // Dimensiones de la intersección: 0 si no se solapan
    int interWidth = std::max(0, std::min(aXMax, bXMax) - std::max(aXMin, bXMin));
    int interHeight = std::max(0, std::min(aYMax, bYMax) - std::max(aYMin, bYMin));

    // Áreas (en double: con int, la división de abajo sería entera)
    double interArea = double(interWidth) * interHeight;
    double unionArea = double(a.area()) + b.area() - interArea;

    result = interArea / unionArea;
    //// END TODO
    return result;
}

/**
 * Supresión de no máximos: índices de las BBs que se quedan.
 *
 * Se recorren las BBs de mayor a menor puntuación. Cada una se queda si no se solapa
 * (IoU > iouThreshold) con ninguna de las que ya se han quedado.
 *
 * @param boxes         las BBs
 * @param scores        puntuación de cada BB
 * @param iouThreshold  valores [0, 1]. Cuanto más pequeño, más BBs se eliminan
 * @return los índices en boxes, de mayor a menor puntuación
 */
std::vector<int> nms(const std::vector<cv::Rect>& boxes, const std::vector<float>& scores, double iouThreshold) {
    std::vector<int> keep;
    // Índices de las BBs, de mayor a menor puntuación
    std::vector<int> order(boxes.size());
    std::iota(order.begin(), order.end(), 0);  // 0, 1, 2, ...
    std::sort(order.begin(), order.end(), [&](int i, int j) { return scores[i] > scores[j]; });
    //// TODO: CODE HERE
    for (int i : order) {
        bool overlaps = false;
        for (int j : keep) {  // las que ya se han quedado
            if (iou(boxes[i], boxes[j]) > iouThreshold) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            keep.push_back(i);
        }
    }
    //// END TODO
    return keep;
}

/** BBs de todas las escalas cuya respuesta pasa el umbral: como mucho, las MAX_CANDIDATES mejores. */
void candidates(const std::vector<cv::Mat>& responses, const cv::Mat& templ, double threshold,
                std::vector<cv::Rect>& boxes, std::vector<float>& scores) {
    std::vector<cv::Rect> all;
    std::vector<float> allScores;
    for (size_t s = 0; s < SCALES.size(); s++) {
        cv::Size size = scaledSize(templ, SCALES[s]);
        const cv::Mat& response = responses[s];
        for (int y = 0; y < response.rows; y++) {
            for (int x = 0; x < response.cols; x++) {
                float score = response.at<float>(y, x);
                if (score > threshold) {
                    all.emplace_back(x, y, size.width, size.height);
                    allScores.push_back(score);
                }
            }
        }
    }
    std::vector<int> order(all.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int i, int j) { return allScores[i] > allScores[j]; });
    order.resize(std::min(order.size(), MAX_CANDIDATES));
    boxes.clear();
    scores.clear();
    for (int i : order) {
        boxes.push_back(all[i]);
        scores.push_back(allScores[i]);
    }
}

/** Copia de la imagen con las detecciones de keep en verde, con su puntuación. */
cv::Mat draw(const cv::Mat& img, const std::vector<cv::Rect>& boxes, const std::vector<float>& scores,
             const std::vector<int>& keep) {
    cv::Mat shown = img.clone();
    for (int i : keep) {
        cv::rectangle(shown, boxes[i], cv::Scalar(0, 255, 0), 2);
        cv::putText(shown, cv::format("%.2f", scores[i]), cv::Point(boxes[i].x, boxes[i].y - 4), FONT, 0.6,
                    cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
    }
    return shown;
}

int main() {
    //// 1. La imagen y la plantilla, las dos en gris
    cv::Mat img = cv::imread(IMAGE);
    cv::Mat templColor = cv::imread(TEMPLATE);
    if (img.empty() || templColor.empty()) {
        std::cout << "Cannot read " << IMAGE << " or " << TEMPLATE << std::endl;
        return 1;
    }
    cv::Mat gray, templ;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(templColor, templ, cv::COLOR_BGR2GRAY);
    if (ROTATE_TEMPLATE) {
        cv::rotate(templ, templ, cv::ROTATE_180);
    }

    //// 2. Un mapa de respuesta por escala: se calculan una vez
    std::vector<cv::Mat> responses;
    for (double scale : SCALES) {
        responses.push_back(match(gray, templ, scale));
    }

    //// 3. Detecciones, con el umbral de la respuesta y el de la NMS en trackbars
    cv::namedWindow(WINDOW);
    cv::createTrackbar("threshold x100", WINDOW, nullptr, 100);
    cv::setTrackbarPos("threshold x100", WINDOW, 65);
    cv::createTrackbar("IoU x100", WINDOW, nullptr, 100);
    cv::setTrackbarPos("IoU x100", WINDOW, 30);

    // El mapa de respuesta de la escala 1,0. Va de -1 a 1: se pasa a 0..255 para verlo
    int scaleOne = int(std::find(SCALES.begin(), SCALES.end(), 1.0) - SCALES.begin());
    const cv::Mat& response = responses[scaleOne];
    cv::Mat response8u, responseShown;
    response.convertTo(response8u, CV_8U, 127.5, 127.5);
    cv::applyColorMap(response8u, responseShown, cv::COLORMAP_JET);
    show(WINDOW_RESPONSE, responseShown);
    double maxValue;
    cv::minMaxLoc(response, nullptr, &maxValue);
    cv::setWindowTitle(WINDOW_RESPONSE, cv::format("scale 1.0: max %.2f", maxValue));

    // La NMS escrita a mano puede tardar con muchos candidatos: solo se recalcula si cambia un trackbar
    double lastThreshold = -1, lastIou = -1;
    while (true) {
        // por debajo de 0,50, demasiados candidatos
        double threshold = std::max(50, cv::getTrackbarPos("threshold x100", WINDOW)) / 100.0;
        double iouThreshold = cv::getTrackbarPos("IoU x100", WINDOW) / 100.0;
        if (threshold != lastThreshold || iouThreshold != lastIou) {
            lastThreshold = threshold;
            lastIou = iouThreshold;
            std::vector<cv::Rect> boxes;
            std::vector<float> scores;
            candidates(responses, templ, threshold, boxes, scores);
            std::vector<int> keep = nms(boxes, scores, iouThreshold);
            show(WINDOW, draw(img, boxes, scores, keep));
            cv::setWindowTitle(WINDOW, cv::format("threshold %.2f, IoU %.2f: %d of %d boxes", threshold,
                                                  iouThreshold, (int)keep.size(), (int)boxes.size()));
        }
        if (cv::waitKey(50) == 27) {
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
