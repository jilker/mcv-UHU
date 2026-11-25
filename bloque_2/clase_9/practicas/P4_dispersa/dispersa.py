import os
import sys

import cv2
import numpy as np

from viewer3d import Viewer3D


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


# Matriz antisimétrica de t: [t]x · v = t x v (producto vectorial)
def skew(t):
    t = np.asarray(t, np.float64).ravel()
    return np.array([[0, -t[2], t[1]],
                     [t[2], 0, -t[0]],
                     [-t[1], t[0], 0]])


# Dibuja los emparejamientos: verde los que cumplen la restricción epipolar y rojo los que no
def draw_matches(img_l, kp_l, img_r, kp_r, matches, good_matches):
    canvas = np.hstack([img_l, img_r])
    width = img_l.shape[1]
    good = set((m.queryIdx, m.trainIdx) for m in good_matches)
    for m in matches:
        color = (0, 255, 0) if (m.queryIdx, m.trainIdx) in good else (0, 0, 255)
        x1, y1 = kp_l[m.queryIdx].pt
        x2, y2 = kp_r[m.trainIdx].pt
        cv2.line(canvas, (int(x1), int(y1)), (int(x2) + width, int(y2)), color, 1, cv2.LINE_AA)
    return canvas


# Guarda los puntos en formato OBJ (una línea "v x y z r g b" por punto, color entre 0 y 1), para verlos con MeshLab
def write_obj(path, points, colors):
    with open(path, "w") as file:
        for (x, y, z), (b, g, r) in zip(points, colors):
            file.write(f"v {x:.4f} {y:.4f} {z:.4f} {r / 255:.3f} {g / 255:.3f} {b / 255:.3f}\n")



# TODO 1: calcular la matriz fundamental F a partir de la calibración (igual que el TODO 1 de la práctica P2):
# F = Kr^-T · [t]x · R · Kl^-1
# Devuelve F
def compute_fundamental(Kl, Kr, R, t):
    # --- TO DO ---
    # F = ...
    # Nota: np.linalg.inv(...) es la inversa, .T la traspuesta y @ el producto de matrices
    # Nota: [t]x se calcula con la función skew(t), definida más arriba
    # Nota: opcional, dividir F entre su norma, np.linalg.norm(F), para que tenga norma 1
    return None


# TODO 2: detectar keypoints y calcular sus descriptores SIFT (igual que detect_features de la clase 4).
# Devuelve (keypoints, descriptors).
def detect_features(img):
    # --- TO DO ---
    keypoints, descriptors = [], None
    # sift = cv2.SIFT.create()
    # keypoints, descriptors = sift.detectAndCompute(...)
    # Nota: SIFT trabaja con la imagen en gris
    return keypoints, descriptors


# TODO 3: emparejar los descriptores de desc1 con los de desc2, con el ratio test (igual que match_features de la
# clase 4): se acepta el vecino más cercano solo si distancia_1 < ratio * distancia_2.
# Devuelve la lista de cv2.DMatch (queryIdx = keypoint de desc1, trainIdx = keypoint de desc2).
def match_features(desc1, desc2, ratio):
    matcher = cv2.BFMatcher(cv2.NORM_L2)
    # --- TO DO ---
    matches = []
    # knn = matcher.knnMatch(...)
    # Nota: cada knn[i] tiene los 2 vecinos más cercanos (cv2.DMatch), ordenados, y la distancia está en .distance
    return matches


# TODO 4: quedarse solo con los emparejamientos que cumplen la restricción epipolar: el punto derecho tiene que
# estar a menos de MAX_EPIPOLAR_DIST pixels de la línea epipolar (a, b, c) del punto izquierdo.
# Devuelve la lista de cv2.DMatch que la cumplen.
def filter_epipolar(kp_l, kp_r, matches, F):
    MAX_EPIPOLAR_DIST = 2.0     # distancia máxima (en pixels) de un punto a su línea epipolar
    good_matches = []
    if not matches:
        return good_matches
    points_l = np.float32([kp_l[m.queryIdx].pt for m in matches]).reshape(-1, 1, 2)
    # --- TO DO ---
    # lines = cv2.computeCorrespondEpilines(...).reshape(-1, 3)
    # Nota: como en P2. Con reshape(-1, 3) queda una fila (a, b, c) por emparejamiento
    # Pista: recorrer los emparejamientos y quedarse con los que estén cerca de su línea:
    # for (a, b, c), m in zip(lines, matches):
    #     x, y = kp_r[m.trainIdx].pt                     # punto derecho del emparejamiento
    #     dist = ...                                     # |a·x + b·y + c| / sqrt(a * a + b * b)
    #     if dist < MAX_EPIPOLAR_DIST:
    #         good_matches.append(m)
    return good_matches


# TODO 5: triangular los puntos emparejados (la cámara izquierda es el origen del sistema de coordenadas).
# Devuelve un array N x 3 float32 con los puntos 3D.
def triangulate(points_l, points_r, Kl, Kr, R, t):
    # matrices de proyección de las dos cámaras (la izquierda es el origen): P1 = Kl·[I | 0] y P2 = Kr·[R | t]
    P1 = Kl @ np.hstack([np.eye(3), np.zeros((3, 1))])
    P2 = Kr @ np.hstack([R, t.reshape(3, 1)])
    # --- TO DO ---
    points_4d = np.zeros((4, 0), np.float32)
    # points_4d = cv2.triangulatePoints(...)
    # Nota: recibe los puntos como 2 x N (points_l.T)

    # triangulatePoints da los puntos en coordenadas homogéneas (X, Y, Z, W), 4 x N: se pasan a 3D (X / W, Y / W, Z / W)
    points_3d = (points_4d[:3] / points_4d[3]).T
    return points_3d.astype(np.float32)


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
F = compute_fundamental(Kl, Kr, R, t)
if F is None:
    print("F is empty (is TODO 1 done?)", file=sys.stderr)
    sys.exit(-1)


#### 3. QUITAMOS LA DISTORSIÓN
undistorted_l = cv2.undistort(frame_l, Kl, Dl)
undistorted_r = cv2.undistort(frame_r, Kr, Dr)


#### 4. KEYPOINTS Y DESCRIPTORES
kp_l, desc_l = detect_features(undistorted_l)
kp_r, desc_r = detect_features(undistorted_r)


#### 5. MATCHING
matches = []
if desc_l is not None and desc_r is not None:
    matches = match_features(desc_l, desc_r, 0.8)      # ratio = 0.8


#### 6. FILTRO EPIPOLAR
good_matches = filter_epipolar(kp_l, kp_r, matches, F)
print(f"Keypoints: {len(kp_l)} | {len(kp_r)}, matches: {len(matches)}, epipolar: {len(good_matches)}")


#### 7. TRIANGULACIÓN (solo los puntos que quedan delante de la cámara)
points = np.zeros((0, 3), np.float32)
colors = np.zeros((0, 3), np.uint8)
if good_matches:
    points_l = np.float32([kp_l[m.queryIdx].pt for m in good_matches])
    points_r = np.float32([kp_r[m.trainIdx].pt for m in good_matches])
    points = triangulate(points_l, points_r, Kl, Kr, R, t)
    if len(points) > 0:
        colors = undistorted_l[points_l[:, 1].astype(int), points_l[:, 0].astype(int)]
        in_front = points[:, 2] > 0
        points, colors = points[in_front], colors[in_front]
        if len(points) > 0:
            print(f"Points 3D: {len(points)}, median Z: {np.median(points[:, 2])}")


#### 8. MOSTRAMOS RESULTADO: emparejamientos y nube 3D
canvas = draw_matches(undistorted_l, kp_l, undistorted_r, kp_r, matches, good_matches)
cv2.putText(canvas, "[s] save reconstruction.obj   [ESC] exit", (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
cv2.putText(canvas, "green: good (close to the epipolar line)", (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
cv2.putText(canvas, "red: filtered (far from the epipolar line)", (10, 75), cv2.FONT_HERSHEY_SIMPLEX, 0.6,
            (0, 0, 255), 2)
cv2.imshow("matches", canvas)
viewer = Viewer3D("3D", 3)
viewer.set_points(points, colors)
key = 0
while key != 27:
    viewer.render()
    key = cv2.waitKey(30)
    if key == ord("s"):
        write_obj("reconstruction.obj", points, colors)
        print(f"Saved reconstruction.obj ({len(points)} points)")
cv2.destroyAllWindows()
