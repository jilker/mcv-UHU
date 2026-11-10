#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

const std::string MODEL = "../data/yolov8n.onnx";
const std::string NAMES = "../data/coco.names";
const std::string IMAGE = "../data/messi5.jpg";
const std::vector<std::string> IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".bmp"};
const std::string WINDOW = "P2 YOLO";
const int INPUT_SIZE = 640;
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

/**
 * Imagen cuadrada de INPUT_SIZE x INPUT_SIZE para la red, sin deformar la original.
 *
 * La imagen se escala para que su lado mayor mida INPUT_SIZE
 * y se pega en la esquina superior izquierda de un lienzo gris.
 * El resto del lienzo queda gris.
 * @param img    imagen BGR de cualquier tamaño
 * @param scale  (salida) el factor por el que se ha multiplicado la imagen.
 * @return el lienzo BGR de INPUT_SIZE x INPUT_SIZE
 */
cv::Mat prepareInput(const cv::Mat& img, double& scale) {
    cv::Mat canvas;
    canvas = cv::Mat(INPUT_SIZE, INPUT_SIZE, CV_8UC3, cv::Scalar::all(114));
    //// TODO: CODE HERE
    scale = double(INPUT_SIZE) / std::max(img.cols, img.rows);
    int width = cvRound(img.cols * scale);
    int height = cvRound(img.rows * scale);
    cv::Size size(width, height);
    cv::Rect roi(0, 0, width, height);
    cv::resize(img, canvas(roi), size, 0, 0, cv::INTER_LINEAR);
    //// END TODO
    return canvas;
}

/**
 * La red YOLOv8, leída del fichero ONNX.
 *
 * PISTA: cv::dnn::readNet. Deduce el formato por la extensión del fichero.
 *
 * @param path ruta del fichero .onnx
 * @return la red, lista para infer
 */
cv::dnn::Net loadNet(const std::string& path) {
    cv::dnn::Net net;
    //// TODO: CODE HERE
    net = cv::dnn::readNet(path);
    //// END TODO
    return net;
}

/**
 * Salida de la red para el lienzo de prepareInput.
 *
 * La red no recibe la imagen tal cual, sino un blob: un tensor float de 1 x 3 x INPUT_SIZE x INPUT_SIZE
 * (imagen, canal, fila, columna), con valores entre 0 y 1 y los canales en orden RGB.
 * OpenCV lee las imágenes en BGR y entre 0 y 255. Algo hay que hacer para convertirlas a RGB y a 0..1.
 * Con el blob como entrada, la red se ejecuta con forward.
 *
 * PISTA: cv::dnn::blobFromImage, net.setInput y net.forward.
 *
 * @param net     la red de loadNet
 * @param canvas  el lienzo BGR de INPUT_SIZE x INPUT_SIZE
 * @return la salida de la red
 */
cv::Mat infer(cv::dnn::Net& net, const cv::Mat& canvas) {
    cv::Mat output;
    double scaleFactor = 1.0 / 255;
    cv::Size size(INPUT_SIZE, INPUT_SIZE);
    //// TODO: CODE HERE
    cv::Mat blob = cv::dnn::blobFromImage(canvas, scaleFactor, size, cv::Scalar(), true);
    net.setInput(blob);
    output = net.forward();
    //// END TODO
    return output;
}

/**
 * Detecciones de la salida de YOLOv8 con confianza mayor que confThreshold.
 *
 * La salida tiene forma 1 x 84 x 8400: 8400 BBs candidatas, y para cada una 84 números. Los 4
 * primeros son la BB (cx, cy, w, h) en píxeles del lienzo de 640 x 640: centro, ancho y alto.
 * Los 80 siguientes, la puntuación de cada clase de COCO, entre 0 y 1.
 * La confianza de la BB es la mayor de las 80 clases y su clase es la de esa puntuación.
 *
 * PISTA: para cada BB (una fila de 84 números), cv::minMaxLoc sobre las 80 columnas de clase,
 * row.colRange(4, row.cols), da la mayor puntuación y su posición. maxVal es score y maxLoc.x es la clase.
 *
 * @param output          salida de net.forward(), cv::Mat CV_32F de 3 dimensiones, 1 x 84 x 8400
 * @param scale           el factor que devolvió prepareInput
 * @param confThreshold   entre 0 y 1
 * @param boxes, scores, classIds  (salida) una entrada por detección. Cada BB es la esquina superior
 *                        izquierda, el ancho y el alto en píxeles de la imagen original
 */
void decode(const cv::Mat& output, double scale, float confThreshold,
            std::vector<cv::Rect2d>& boxes, std::vector<float>& scores, std::vector<int>& classIds) {
    boxes.clear();
    scores.clear();
    classIds.clear();
    // De 1 x 84 x 8400 a 84 filas (los mismos datos, en 2D) y traspuesta a 8400 x 84: una fila por BB candidata
    cv::Mat predictions = output.reshape(1, output.size[1]).t();
    //// TODO: CODE HERE
    for (int i = 0; i < predictions.rows; i++) {
        const cv::Mat row = predictions.row(i);
        double score;
        cv::Point best;
        cv::minMaxLoc(row.colRange(4, row.cols), nullptr, &score, nullptr, &best);
        if (score > confThreshold) {
            float cx = row.at<float>(0) / scale;
            float cy = row.at<float>(1) / scale;
            float w = row.at<float>(2) / scale;
            float h = row.at<float>(3) / scale;
            cv::Rect2d box(cx - w / 2, cy - h / 2, w, h);
            boxes.push_back(box);
            scores.push_back(float(score));
            classIds.push_back(best.x);
        }
    }
    //// END TODO
}

/**
 * Supresión de no máximos: índices de las detecciones que se quedan.
 *
 * De cada grupo de BBs de la misma clase que se solapan (IoU mayor que nmsThreshold) solo se queda
 * la de mayor confianza. BBs de clases distintas no se suprimen entre sí.
 *
 * PISTA: cv::dnn::NMSBoxesBatched.
 *
 * @param boxes, scores, classIds  lo que devuelve decode
 * @param nmsThreshold             entre 0 y 1; cuanto más pequeño, más BBs se eliminan
 * @return los índices en boxes
 */
std::vector<int> nms(const std::vector<cv::Rect2d>& boxes, const std::vector<float>& scores,
                     const std::vector<int>& classIds, float nmsThreshold) {
    std::vector<int> keep;
    //// TODO: CODE HERE
    cv::dnn::NMSBoxesBatched(boxes, scores, classIds, 0.0f, nmsThreshold, keep);
    //// END TODO
    return keep;
}

/** Copia de la imagen con las detecciones de keep: la BB y una etiqueta con la clase y la confianza. */
cv::Mat draw(const cv::Mat& img, const std::vector<cv::Rect2d>& boxes, const std::vector<float>& scores,
             const std::vector<int>& classIds, const std::vector<int>& keep,
             const std::vector<std::string>& names, const std::vector<cv::Scalar>& colors) {
    cv::Mat shown = img.clone();
    for (int i : keep) {
        cv::Rect box = boxes[i];
        cv::Scalar color = colors[classIds[i]];
        cv::rectangle(shown, box, color, 2);
        std::string label = names[classIds[i]] + cv::format(" %.2f", scores[i]);
        int baseline;
        cv::Size text = cv::getTextSize(label, FONT, 0.5, 1, &baseline);
        int y = std::max(box.y, text.height + baseline);
        cv::rectangle(shown, cv::Point(box.x, y - text.height - baseline), cv::Point(box.x + text.width, y),
                      color, cv::FILLED);
        cv::putText(shown, label, cv::Point(box.x, y - baseline), FONT, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    return shown;
}

bool isImage(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    for (char& c : ext) {
        c = std::tolower(c);
    }
    return std::find(IMAGE_EXTENSIONS.begin(), IMAGE_EXTENSIONS.end(), ext) != IMAGE_EXTENSIONS.end();
}

bool isNumber(const std::string& text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), ::isdigit);
}

int main(int argc, char** argv) {
    //// 1. La red y los nombres de las 80 clases de COCO
    if (!std::filesystem::is_regular_file(MODEL)) {
        std::cout << "Cannot read " << MODEL << ": run python3 ../data/descargar_modelo.py" << std::endl;
        return 1;
    }
    cv::dnn::Net net = loadNet(MODEL);
    std::vector<std::string> names;
    std::ifstream namesFile(NAMES);
    for (std::string line; std::getline(namesFile, line);) {
        names.push_back(line);
    }
    cv::RNG rng(0);
    std::vector<cv::Scalar> colors;
    for (size_t i = 0; i < names.size(); i++) {
        colors.emplace_back(rng.uniform(64, 256), rng.uniform(64, 256), rng.uniform(64, 256));
    }

    //// 2. La entrada: una imagen, un vídeo o una cámara (./build/yolo ../data/vtest.avi, ./build/yolo cam)
    std::string source = argc > 1 ? argv[1] : IMAGE;
    if (source == "cam") {
        source = "0";
    }
    bool isVideo = !isImage(source);
    cv::Mat frame;
    cv::VideoCapture capture;
    if (!isVideo) {
        frame = cv::imread(source);
        if (frame.empty()) {
            std::cout << "Cannot read " << source << std::endl;
            return 1;
        }
    } else {
        if (isNumber(source)) {
            capture.open(std::stoi(source));
        } else {
            capture.open(source);
        }
        if (!capture.isOpened()) {
            std::cout << "Cannot open " << source << std::endl;
            return 1;
        }
    }

    //// 3. Detección, con los dos umbrales en trackbars
    cv::namedWindow(WINDOW);
    cv::createTrackbar("conf x100", WINDOW, nullptr, 100);
    cv::setTrackbarPos("conf x100", WINDOW, 25);
    cv::createTrackbar("NMS x100", WINDOW, nullptr, 100);
    cv::setTrackbarPos("NMS x100", WINDOW, 45);

    // Con una imagen la red se ejecuta una sola vez: al mover los trackbars solo se repiten decode y nms
    cv::Mat output;
    double scale = 1;
    while (true) {
        if (isVideo) {
            bool ok = capture.read(frame);
            if (!ok && !isNumber(source)) {     // fin del vídeo: vuelta a empezar
                capture.set(cv::CAP_PROP_POS_FRAMES, 0);
                ok = capture.read(frame);
            }
            if (!ok) {
                break;
            }
        }
        if (output.empty() || isVideo) {
            cv::Mat canvas = prepareInput(frame, scale);
            output = infer(net, canvas);
        }

        float confThreshold = std::max(1, cv::getTrackbarPos("conf x100", WINDOW)) / 100.0f;
        float nmsThreshold = cv::getTrackbarPos("NMS x100", WINDOW) / 100.0f;
        std::vector<cv::Rect2d> boxes;
        std::vector<float> scores;
        std::vector<int> classIds;
        decode(output, scale, confThreshold, boxes, scores, classIds);
        std::vector<int> keep = nms(boxes, scores, classIds, nmsThreshold);

        show(WINDOW, draw(frame, boxes, scores, classIds, keep, names, colors));
        cv::setWindowTitle(WINDOW, cv::format("conf %.2f, NMS %.2f: %d of %d boxes", confThreshold,
                                              nmsThreshold, (int)keep.size(), (int)boxes.size()));
        if (cv::waitKey(isVideo ? 1 : 50) == 27) {
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
