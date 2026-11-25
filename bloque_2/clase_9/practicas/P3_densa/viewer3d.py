import cv2
import numpy as np


# Visor 3D sencillo hecho con OpenCV (sin módulos extra): una cámara virtual gira alrededor del centro de la nube
# y la proyecta sobre una imagen. [arrastrar con el botón izquierdo] girar, [rueda o arrastrar con el derecho] zoom
class Viewer3D:
    def __init__(self, window, point_size):
        self.window = window
        self.point_size = point_size
        self.points = np.zeros((0, 3), np.float32)
        self.colors = np.zeros((0, 3), np.uint8)
        self.center = np.zeros(3, np.float32)
        self.distance, self.yaw, self.pitch = 1.0, 0.0, 0.0
        self.dragging = 0           # 0 nada, 1 girando, 2 zoom
        self.last_mouse = (0, 0)
        self.changed = True
        self.view_set = False        # la vista se coloca con la primera nube que tiene puntos
        cv2.namedWindow(window)
        cv2.setMouseCallback(window, self.mouse)

    def mouse(self, event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONDOWN:
            self.dragging = 1
        if event == cv2.EVENT_RBUTTONDOWN:
            self.dragging = 2
        if event in (cv2.EVENT_LBUTTONUP, cv2.EVENT_RBUTTONUP):
            self.dragging = 0
        if event == cv2.EVENT_MOUSEMOVE and self.dragging == 1:
            self.yaw += 0.01 * (x - self.last_mouse[0])
            self.pitch = max(-1.5, min(1.5, self.pitch - 0.01 * (y - self.last_mouse[1])))
            self.changed = True
        if event == cv2.EVENT_MOUSEMOVE and self.dragging == 2:
            self.distance *= np.exp(0.01 * (y - self.last_mouse[1]))
            self.changed = True
        if event == cv2.EVENT_MOUSEWHEEL:
            self.distance *= 0.9 if flags > 0 else 1.1        # el signo de flags es el sentido de la rueda
            self.changed = True
        self.last_mouse = (x, y)

    # Cambia la nube que se muestra. La primera vez que recibe puntos coloca la vista como la de la cámara izquierda.
    # Después mantiene el giro y el zoom que se hayan hecho con el ratón.
    def set_points(self, points, colors):
        self.points = np.asarray(points, np.float32).reshape(-1, 3)
        self.colors = np.asarray(colors, np.uint8).reshape(-1, 3)
        self.changed = True
        if self.view_set or len(self.points) == 0:
            return
        self.center = np.median(self.points, axis=0)       # mediana: no le afectan los puntos sueltos
        self.distance = max(1e-6, float(np.linalg.norm(self.center)))
        self.yaw, self.pitch = 0.0, 0.0
        self.view_set = True

    # Dibuja la nube si ha cambiado algo (se llama en cada vuelta del bucle, antes de cv2.waitKey)
    def render(self):
        if not self.changed:
            return
        self.changed = False
        w, h = 640, 480
        f = 0.8 * w
        img = np.full((h, w, 3), 40, np.uint8)
        cy, sy, cp, sp = np.cos(self.yaw), np.sin(self.yaw), np.cos(self.pitch), np.sin(self.pitch)
        d = self.points - self.center
        x = cy * d[:, 0] + sy * d[:, 2]                     # giro alrededor del eje Y
        z = -sy * d[:, 0] + cy * d[:, 2]
        y = cp * d[:, 1] - sp * z                           # giro alrededor del eje X
        z = sp * d[:, 1] + cp * z + self.distance
        valid = z > 0.01 * self.distance
        u = np.round(f * x[valid] / z[valid] + w / 2).astype(int)
        v = np.round(f * y[valid] / z[valid] + h / 2).astype(int)
        colors, z = self.colors[valid], z[valid]
        order = np.argsort(-z, kind="stable")              # se pinta de lejos a cerca
        u, v, colors = u[order], v[order], colors[order]
        inside = (u >= 0) & (v >= 0) & (u < w) & (v < h)
        u, v, colors = u[inside], v[inside], colors[inside]
        if self.point_size <= 1:
            img[v, u] = colors
        else:
            for px, py, c in zip(u, v, colors):
                cv2.circle(img, (int(px), int(py)), self.point_size - 1, tuple(int(k) for k in c), cv2.FILLED)
        cv2.putText(img, f"{len(self.points)} points   [drag] rotate   [wheel / right drag] zoom", (10, 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
        cv2.imshow(self.window, img)
