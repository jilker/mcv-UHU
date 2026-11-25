#include "viewer3d.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

// Ratón: [arrastrar con el botón izquierdo] girar, [rueda o arrastrar con el derecho] zoom
static void viewerMouse(int event, int x, int y, int flags, void *userdata)
{
    Viewer3D *v = (Viewer3D *)userdata;
    if (event == cv::EVENT_LBUTTONDOWN) v->dragging = 1;
    if (event == cv::EVENT_RBUTTONDOWN) v->dragging = 2;
    if (event == cv::EVENT_LBUTTONUP || event == cv::EVENT_RBUTTONUP) v->dragging = 0;
    if (event == cv::EVENT_MOUSEMOVE && v->dragging == 1) {
        v->yaw += 0.01f * (x - v->lastMouse.x);
        v->pitch = std::max(-1.5f, std::min(1.5f, v->pitch - 0.01f * (y - v->lastMouse.y)));
        v->changed = true;
    }
    if (event == cv::EVENT_MOUSEMOVE && v->dragging == 2) {
        v->distance *= std::exp(0.01f * (y - v->lastMouse.y));
        v->changed = true;
    }
    if (event == cv::EVENT_MOUSEWHEEL) {
        v->distance *= cv::getMouseWheelDelta(flags) > 0 ? 0.9f : 1.1f;
        v->changed = true;
    }
    v->lastMouse = cv::Point(x, y);
}

// Crea la ventana del visor
void viewerCreate(Viewer3D &v, const std::string &window, int pointSize)
{
    v.window = window;
    v.pointSize = pointSize;
    cv::namedWindow(window);
    cv::setMouseCallback(window, viewerMouse, &v);
}

// Cambia la nube que se muestra. La primera vez que recibe puntos coloca la vista como la de la cámara izquierda.
void viewerSetPoints(Viewer3D &v, const std::vector<cv::Point3f> &points, const std::vector<cv::Vec3b> &colors)
{
    v.points = points;
    v.colors = colors;
    v.changed = true;
    if (v.viewSet || points.empty()) return;
    std::vector<float> xs, ys, zs;              // centro = mediana de cada coordenada (no le afectan los puntos sueltos)
    for (const cv::Point3f &p : points) {
        xs.push_back(p.x);
        ys.push_back(p.y);
        zs.push_back(p.z);
    }
    for (std::vector<float> *c : {&xs, &ys, &zs})
        std::nth_element(c->begin(), c->begin() + c->size() / 2, c->end());
    v.center = cv::Point3f(xs[xs.size() / 2], ys[ys.size() / 2], zs[zs.size() / 2]);
    v.distance = std::max(1e-6, cv::norm(v.center));
    v.yaw = 0;
    v.pitch = 0;
    v.viewSet = true;
}

// Dibuja la nube si ha cambiado algo (se llama en cada vuelta del bucle, antes de cv::waitKey)
void viewerRender(Viewer3D &v)
{
    if (!v.changed) return;
    v.changed = false;
    const int w = 640, h = 480;
    const float f = 0.8f * w;
    cv::Mat img(h, w, CV_8UC3, cv::Scalar(40, 40, 40));
    float cy = std::cos(v.yaw), sy = std::sin(v.yaw), cp = std::cos(v.pitch), sp = std::sin(v.pitch);
    std::vector<std::pair<float, int>> order;   // (profundidad, índice): se pinta de lejos a cerca
    std::vector<cv::Point2f> projected(v.points.size());
    for (size_t i = 0; i < v.points.size(); i++) {
        cv::Point3f d = v.points[i] - v.center;
        float x = cy * d.x + sy * d.z, z = -sy * d.x + cy * d.z;          // giro alrededor del eje Y
        float y = cp * d.y - sp * z;
        z = sp * d.y + cp * z + v.distance;                                 // giro alrededor del eje X
        if (z <= 0.01f * v.distance) continue;
        projected[i] = cv::Point2f(f * x / z + w / 2, f * y / z + h / 2);
        order.push_back({z, (int)i});
    }
    std::stable_sort(order.begin(), order.end(), [](const std::pair<float, int> &a, const std::pair<float, int> &b) {
        return a.first > b.first;
    });
    for (const std::pair<float, int> &o : order) {
        cv::Point p(cvRound(projected[o.second].x), cvRound(projected[o.second].y));
        if (p.x < 0 || p.y < 0 || p.x >= w || p.y >= h) continue;
        const cv::Vec3b &c = v.colors[o.second];
        if (v.pointSize <= 1)
            img.at<cv::Vec3b>(p) = c;
        else
            cv::circle(img, p, v.pointSize - 1, cv::Scalar(c[0], c[1], c[2]), cv::FILLED);
    }
    cv::putText(img, cv::format("%d points   [drag] rotate   [wheel / right drag] zoom", (int)v.points.size()),
                cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
    cv::imshow(v.window, img);
}
