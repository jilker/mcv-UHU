#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <vector>

// Visor 3D sencillo hecho con OpenCV (sin módulos extra): una cámara virtual gira alrededor del centro de la nube
// y la proyecta sobre una imagen. [arrastrar con el botón izquierdo] girar, [rueda o arrastrar con el derecho] zoom
struct Viewer3D {
    std::string window;
    std::vector<cv::Point3f> points;
    std::vector<cv::Vec3b> colors;
    cv::Point3f center;
    float distance = 1, yaw = 0, pitch = 0;
    int pointSize = 1;
    int dragging = 0;           // 0 nada, 1 girando, 2 zoom
    cv::Point lastMouse;
    bool changed = true;
    bool viewSet = false;        // la vista se coloca con la primera nube que tiene puntos
};

// Crea la ventana del visor
void viewerCreate(Viewer3D &v, const std::string &window, int pointSize);

// Cambia la nube que se muestra. La primera vez que recibe puntos coloca la vista como la de la cámara izquierda.
// Después mantiene el giro y el zoom que se hayan hecho con el ratón.
void viewerSetPoints(Viewer3D &v, const std::vector<cv::Point3f> &points, const std::vector<cv::Vec3b> &colors);

// Dibuja la nube si ha cambiado algo (se llama en cada vuelta del bucle, antes de cv::waitKey)
void viewerRender(Viewer3D &v);
