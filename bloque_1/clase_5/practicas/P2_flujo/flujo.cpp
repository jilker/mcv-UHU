#include <opencv2/core.hpp>
#include <opencv2/features.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

const std::string VIDEO = "../data/vtest.avi";
const std::string WINDOW = "P2 optical flow";
const int MAX_CORNERS = 300;            // esquinas nuevas que se buscan en cada fotograma, como mucho
const float FB_MAX_ERROR = 1.0f;        // píxeles: un punto que no vuelve a menos de esta distancia se descarta
const size_t TRAIL = 15;                // fotogramas que se recuerda la trayectoria de cada punto
const float MOVING = 2.0f;              // píxeles: un punto que se ha movido más que esto en su trayectoria se pinta en verde

/**
 * Busqueda de esquinas en la imagen con cv::goodFeaturesToTrack.
 *
 * Parametros:
 *  Calidad = 0.01 (umbral de calidad de las esquinas)
 *  Separación = 10 (distancia mínima entre esquinas)
 *  Bloque = 7 (tamaño del bloque para el cálculo de la esquina)
 * 
 * @param gray        imagen en gris
 * @param mask        imagen CV_8U del mismo tamaño (solo se buscan esquinas donde vale 255)
 * @param maxCorners  número máximo de esquinas
 * @return las esquinas (x, y)
 */
std::vector<cv::Point2f> detectCorners(const cv::Mat& gray, const cv::Mat& mask, int maxCorners) {
    std::vector<cv::Point2f> corners;
    //// TODO: CODE HERE
    //// END TODO
    return corners;
}

/**
 * Sigue los puntos pts de la imagen prev a la imagen curr con cv::calcOpticalFlowPyrLK.
 *
 * @param prev, curr  las dos imágenes, en gris
 * @param pts         los puntos en prev
 * @param maxLevel    último nivel de la pirámide; 0 es Lucas-Kanade sin pirámide
 * @param winSize     lado de la ventana W de Lucas-Kanade, en píxeles
 * @param newPts      (salida) los puntos en curr, en el mismo orden
 * @param ok          (salida) un uchar por punto: 1 si Lucas-Kanade lo ha podido seguir, 0 si no (es el status de
 *                    calcOpticalFlowPyrLK, que se le puede pasar directamente)
 */
void track(const cv::Mat& prev, const cv::Mat& curr, const std::vector<cv::Point2f>& pts, int maxLevel,
           int winSize, std::vector<cv::Point2f>& newPts, std::vector<uchar>& ok) {
    newPts = pts;
    ok.assign(pts.size(), 0);
    cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.01); 
    //// TODO: CODE HERE
    //// END TODO
}

/**
 * Comprobación hacia atrás: sigue newPts de curr a prev y mira si vuelven a pts.
 *
 * Si Lucas-Kanade se ha equivocado hacia delante, lo normal es que hacia atrás no vuelva al punto de
 * partida: es una forma barata de detectar los seguimientos erróneos (los outliers).
 *
 * PISTA: es la misma función track, con las imágenes cambiadas de orden.
 *
 * @param prev, curr  las dos imágenes, en gris
 * @param pts         los puntos en prev
 * @param newPts      los mismos puntos seguidos en curr
 * @param maxLevel    último nivel de la pirámide
 * @param winSize     lado de la ventana W de Lucas-Kanade, en píxeles
 * @return un uchar por punto: 1 en los que vuelven a menos de FB_MAX_ERROR píxeles de pts, 0 en el resto
 */
std::vector<uchar> forwardBackward(const cv::Mat& prev, const cv::Mat& curr, const std::vector<cv::Point2f>& pts,
                                   const std::vector<cv::Point2f>& newPts, int maxLevel, int winSize) {
    std::vector<uchar> ok(pts.size(), 0);
    //// TODO: CODE HERE
    //// END TODO
    return ok;
}

int main(int argc, char** argv) {
    //// Línea de comandos: el vídeo de data/ o la webcam
    bool useCamera = argc > 1 && std::string(argv[1]) == "cam";
    cv::VideoCapture cap;
    if (useCamera) {
        cap.open(0);
    } else {
        cap.open(VIDEO);
    }
    cv::Mat frame, gray, prev;
    if (!cap.read(frame)) {
        std::cout << "Cannot read " << (useCamera ? "the camera" : VIDEO) << std::endl;
        return 1;
    }
    cv::cvtColor(frame, prev, cv::COLOR_BGR2GRAY);

    cv::namedWindow(WINDOW);
    cv::createTrackbar("maxLevel", WINDOW, nullptr, 5);
    cv::setTrackbarPos("maxLevel", WINDOW, 3);
    cv::createTrackbar("step", WINDOW, nullptr, 10);
    cv::setTrackbarPos("step", WINDOW, 4);
    cv::createTrackbar("winSize", WINDOW, nullptr, 51);
    cv::setTrackbarMin("winSize", WINDOW, 3);
    cv::setTrackbarPos("winSize", WINDOW, 21);

    std::vector<std::vector<cv::Point2f>> trails;  
    std::vector<cv::Point2f> rejected;
    bool check = true;
    bool paused = false;
    int lostLk = 0, lostFb = 0;

    while (true) {
        int maxLevel = cv::getTrackbarPos("maxLevel", WINDOW);
        int step = std::max(1, cv::getTrackbarPos("step", WINDOW));
        int winSize = cv::getTrackbarPos("winSize", WINDOW);

        if (!paused) {
            bool ok = true;
            for (int k = 0; k < step && ok; k++) {
                ok = cap.read(frame);
            }
            if (!ok) {
                if (useCamera) {
                    break;
                }
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                cap.read(frame);
                cv::cvtColor(frame, prev, cv::COLOR_BGR2GRAY);
                trails.clear();
                continue;
            }
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

            //// 2. Seguir los puntos con Lucas-Kanade y descartar los que fallan
            lostLk = lostFb = 0;
            rejected.clear();
            if (!trails.empty()) {
                std::vector<cv::Point2f> pts, newPts;
                for (const auto& trail : trails) {
                    pts.push_back(trail.back());
                }
                std::vector<uchar> okLk, okAll;
                track(prev, gray, pts, maxLevel, winSize, newPts, okLk);
                okAll = okLk;
                if (check) {
                    std::vector<uchar> okFb = forwardBackward(prev, gray, pts, newPts, maxLevel, winSize);
                    for (size_t i = 0; i < pts.size(); i++) {
                        okAll[i] = okAll[i] && okFb[i];
                    }
                }
                std::vector<std::vector<cv::Point2f>> kept;
                for (size_t i = 0; i < pts.size(); i++) {
                    if (!okLk[i]) {
                        lostLk++;
                    } else if (!okAll[i]) {
                        lostFb++;
                    }
                    if (!okAll[i]) {
                        rejected.push_back(pts[i]);
                        continue;
                    }
                    kept.push_back(trails[i]);
                    kept.back().push_back(newPts[i]);
                    if (kept.back().size() > TRAIL) {
                        kept.back().erase(kept.back().begin());
                    }
                }
                trails = kept;
            }

            //// 3. Esquinas nuevas lejos de los puntos que ya hay.
            cv::Mat mask(gray.size(), CV_8U, cv::Scalar(255));
            for (const auto& trail : trails) {
                cv::circle(mask, trail.back(), 10, cv::Scalar(0), -1);
            }
            for (const cv::Point2f& p : detectCorners(gray, mask, MAX_CORNERS)) {
                trails.push_back({p});
            }

            prev = gray.clone();
        }

        //// 4. Dibujo: trayectorias en verde si el punto se mueve, en gris si está quieto, descartados en rojo
        cv::Mat vis = frame.clone();
        int moving = 0;
        for (const auto& trail : trails) {
            if (cv::norm(trail.back() - trail.front()) > MOVING) {
                moving++;
                std::vector<cv::Point> line(trail.begin(), trail.end());
                cv::polylines(vis, line, false, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
                cv::circle(vis, trail.back(), 3, cv::Scalar(0, 255, 0), -1);
            } else {
                cv::circle(vis, trail.back(), 2, cv::Scalar(128, 128, 128), -1);
            }
        }
        for (const cv::Point2f& p : rejected) {
            cv::circle(vis, p, 5, cv::Scalar(0, 0, 255), 2);
        }

        cv::imshow(WINDOW, vis);
        cv::setWindowTitle(WINDOW, std::to_string(trails.size()) + " points (" + std::to_string(moving) +
                                   " moving)   lost: LK " + std::to_string(lostLk) + ", forward-backward " +
                                   std::to_string(lostFb) + "   [check " + (check ? "on" : "off") + "]");
        int key = cv::waitKey(30);
        if (key == 27) {
            break;
        } else if (key == ' ') {
            paused = !paused;
        } else if (key == 'f') {
            check = !check;
            std::cout << "Forward-backward check: " << (check ? "on" : "off") << std::endl;
        } else if (key == 'r') {
            trails.clear();                             // en el siguiente fotograma se buscan todas de nuevo
        }
    }

    cv::destroyAllWindows();
    return 0;
}
