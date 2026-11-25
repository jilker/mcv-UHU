import os
import sys

import cv2
import numpy as np

image_width = 0
COLORS = [(0, 0, 255), (0, 255, 0), (255, 0, 0), (0, 255, 255), (255, 0, 255), (255, 255, 0)]


# Lee la calibración stereo
# Devuelve (Kl, Dl, Kr, Dr, R, t), o None si no se ha podido leer.
def load_stereo_calibration(path):
    fs = cv2.FileStorage(path, cv2.FILE_STORAGE_READ)
    if not fs.isOpened():
        return None
    names = ["cameraMatrixL", "distCoeffsL", "cameraMatrixR", "distCoeffsR", "R", "T"]
    calib = [fs.getNode(name).mat() for name in names]
    fs.release()
    if any(m is None or m.size == 0 for m in calib):
        return None
    return calib


# Lee el par de imágenes de la carpeta: img0_l e img0_r (.jpg o .png). Devuelve (img_l, img_r), o None si no están.
def read_stereo_pair(folder):
    for ext in (".jpg", ".png"):
        left = f"{folder}/img0_l{ext}"
        right = f"{folder}/img0_r{ext}"
        if os.path.exists(left) and os.path.exists(right):
            img_l, img_r = cv2.imread(left), cv2.imread(right)
            if img_l is None or img_r is None:
                return None
            return img_l, img_r
    return None


# Dibuja la recta a·x + b·y + c = 0 de lado a lado de la imagen
def draw_line(img, line, color):
    a, b, c = line
    if abs(b) < 1e-9:
        return
    w = img.shape[1]
    p1 = (0, int(round(-c / b)))
    p2 = (w - 1, int(round(-(c + a * (w - 1)) / b)))
    cv2.line(img, p1, p2, color, 1, cv2.LINE_AA)


# Matriz antisimétrica de t: [t]x · v = t x v (producto vectorial)
def skew(t):
    t = np.asarray(t, np.float64).ravel()
    return np.array([[0, -t[2], t[1]],
                     [t[2], 0, -t[0]],
                     [-t[1], t[0], 0]])



# Aplica la rectificación a una imagen: como undistort en la clase 7, pero además gira la cámara (R_rect) y usa su
# nueva matriz de proyección (P)
def rectify_image(img, K, D, R_rect, P):
    image_size = (img.shape[1], img.shape[0])
    map_x, map_y = cv2.initUndistortRectifyMap(K, D, R_rect, P, image_size, cv2.CV_32FC1)
    return cv2.remap(img, map_x, map_y, cv2.INTER_LINEAR)


# TODO 1: calcular la matriz fundamental F a partir de la calibración: F = Kr^-T · [t]x · R · Kl^-1
# Devuelve F
def compute_fundamental(Kl, Kr, R, t):
    # --- TO DO ---
    # F = ...
    # Nota: np.linalg.inv(...) es la inversa, .T la traspuesta y @ el producto de matrices
    # Nota: [t]x se calcula con la función skew(t), definida más arriba
    # Nota: opcional, dividir F entre su norma, np.linalg.norm(F), para que tenga norma 1
    return None


# TODO 2: calcular la línea epipolar (a, b, c) en la imagen derecha que corresponde a un punto de la izquierda.
def epipolar_line(F, point):
    # --- TO DO ---
    points = np.array([point], np.float32).reshape(-1, 1, 2)
    # lines = cv2.computeCorrespondEpilines(...)
    # Nota: whichImage = 1 porque los puntos son de la imagen izquierda. Devuelve un array N x 1 x 3 y
    #       lines.reshape(3) deja solo (a, b, c)
    return np.zeros(3)


# TODO 3: rectificación stereo: calcular las rotaciones R1 y R2 que alinean las dos cámaras y sus nuevas matrices de
# proyección P1 y P2
# Devuelve (R1, R2, P1, P2).
def compute_rectification(Kl, Dl, Kr, Dr, R, t, image_size):
    # --- TO DO ---
    # R1, R2, P1, P2, Q, roi1, roi2 = cv2.stereoRectify(...)
    # Nota: Usar flags = cv2.CALIB_ZERO_DISPARITY y alpha = -1 (escala por defecto)
    return None, None, None, None


# Matriz fundamental del par rectificado
def rectified_fundamental(P1, P2):
    K1 = P1[:, :3]
    K2 = P2[:, :3]
    t = np.linalg.inv(K2) @ P2[:, 3]
    return compute_fundamental(K1, K2, np.eye(3), t)


# Dibuja izquierda | derecha: los puntos pinchados en la izquierda y sus líneas epipolares en la derecha
def draw_epipolar(img_l, img_r, F, points):
    canvas_l = img_l.copy()
    canvas_r = img_r.copy()
    for i, (x, y) in enumerate(points):
        color = COLORS[i % len(COLORS)]
        cv2.circle(canvas_l, (x, y), 5, color, 2)
        draw_line(canvas_r, epipolar_line(F, (x, y)), color)
    canvas = np.hstack([canvas_l, canvas_r])
    cv2.putText(canvas, "[click left] point   [c] clear   [ESC] exit", (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    return canvas


# Clic izquierdo en la imagen izquierda: añade el punto a la lista de la ventana (param)
def on_mouse(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN and x < image_width:
        param.append((x, y))


#### 1. LEEMOS PARAMETROS
if len(sys.argv) != 2:
    print("Usage: path_to_data_folder", file=sys.stderr)
    print("Folder must contain: img0_l, img0_r (.jpg or .png) and calibration.yml", file=sys.stderr)
    sys.exit(-1)
folder = sys.argv[1]
calibration_file = f"{folder}/calibration.yml"


#### 2. LEEMOS CALIBRACION E IMAGENES
calib = load_stereo_calibration(calibration_file)
if calib is None:
    print(f"Cannot read {calibration_file}", file=sys.stderr)
    sys.exit(-1)
Kl, Dl, Kr, Dr, R, t = calib
pair = read_stereo_pair(folder)
if pair is None:
    print(f"Cannot read {folder}/img0_l.jpg + img0_r.jpg (or .png)", file=sys.stderr)
    sys.exit(-1)
frame_l, frame_r = pair
print(f"Image {frame_l.shape[1]}x{frame_l.shape[0]}")
image_width = frame_l.shape[1]


#### 3. QUITAMOS LA DISTORSIÓN (trabajamos sin distorsión)
undistorted_l = cv2.undistort(frame_l, Kl, Dl)
undistorted_r = cv2.undistort(frame_r, Kr, Dr)


#### 4. MATRIZ FUNDAMENTAL (a partir de K, R y t)
F = compute_fundamental(Kl, Kr, R, t)
if F is None:
    print("F is empty (is TODO 1 done?)", file=sys.stderr)
    sys.exit(-1)
print(f"F:\n{F}")


#### 5. RECTIFICAMOS EL PAR Y CALCULAMOS SU MATRIZ FUNDAMENTAL
R1, R2, P1, P2 = compute_rectification(Kl, Dl, Kr, Dr, R, t, (frame_l.shape[1], frame_l.shape[0]))
if R1 is None or R2 is None or P1 is None or P2 is None:
    print("Pair not rectified (is TODO 3 done?)", file=sys.stderr)
    rectified_l, rectified_r, F_rectified = undistorted_l, undistorted_r, F
else:
    # aplicar la rectificación (homografía + corrección de distorsión)
    rectified_l = rectify_image(frame_l, Kl, Dl, R1, P1)
    rectified_r = rectify_image(frame_r, Kr, Dr, R2, P2)
    F_rectified = rectified_fundamental(P1, P2)


#### 6. DIBUJAR LÍNEAS EPIPOLARES DE LOS PUNTOS CLICKADOS
# Una ventana sin rectificar y otra rectificada, cada una con sus puntos
clicked_points, clicked_points_rectified = [], []
cv2.namedWindow("epipolar")
cv2.setMouseCallback("epipolar", on_mouse, clicked_points)
cv2.namedWindow("rectified")
cv2.setMouseCallback("rectified", on_mouse, clicked_points_rectified)
while True:
    cv2.imshow("epipolar", draw_epipolar(undistorted_l, undistorted_r, F, clicked_points))
    cv2.imshow("rectified", draw_epipolar(rectified_l, rectified_r, F_rectified, clicked_points_rectified))
    key = cv2.waitKey(30)
    if key == 27:
        break
    if key == ord("c"):
        clicked_points.clear()
        clicked_points_rectified.clear()
cv2.destroyAllWindows()
