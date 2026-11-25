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


# Disparidad en color (rojo cerca, azul lejos) y negro donde no es válida
def colorize_disparity(disparity, num_disparities):
    gray = np.clip(np.round(disparity * 255.0 / num_disparities), 0, 255).astype(np.uint8)
    color = cv2.applyColorMap(gray, cv2.COLORMAP_JET)
    color[disparity <= 0] = 0
    return color


# Aplica la rectificación a una imagen: como undistort en la clase 7, pero además gira la cámara (R_rect) y usa su
# nueva matriz de proyección (P)
def rectify_image(img, K, D, R_rect, P):
    image_size = (img.shape[1], img.shape[0])
    map_x, map_y = cv2.initUndistortRectifyMap(K, D, R_rect, P, image_size, cv2.CV_32FC1)
    return cv2.remap(img, map_x, map_y, cv2.INTER_LINEAR)


# Guarda los puntos en formato OBJ (una línea "v x y z r g b" por punto, color entre 0 y 1), para verlos con MeshLab
def write_obj(path, points, colors):
    with open(path, "w") as file:
        for (x, y, z), (b, g, r) in zip(points, colors):
            file.write(f"v {x:.4f} {y:.4f} {z:.4f} {r / 255:.3f} {g / 255:.3f} {b / 255:.3f}\n")


def nothing(value):
    pass



# TODO 1: rectificación stereo (como en el TODO 3 de la práctica P2): calcular las rotaciones R1 y R2 que alinean las
# dos cámaras y sus nuevas matrices de proyección P1 y P2
# Devuelve (R1, R2, P1, P2).
def compute_rectification(Kl, Dl, Kr, Dr, R, t, image_size):
    # --- TO DO ---
    # R1, R2, P1, P2, Q, roi1, roi2 = cv2.stereoRectify(...)
    # Nota: Usar flags = cv2.CALIB_ZERO_DISPARITY y alpha = -1 (escala por defecto)
    return None, None, None, None


# TODO 2: calcular el mapa de disparidad con block matching (cv2.StereoBM).
# Devuelve la disparidad en pixels (float32). Los pixels sin disparidad válida quedan negativos.
def compute_disparity_bm(gray_l, gray_r, num_disparities, block_size):
    # --- TO DO ---
    disparity16 = np.full(gray_l.shape, -16, np.int16)
    # matcher = cv2.StereoBM.create(...)
    # disparity16 = matcher.compute(...)

    # compute da la disparidad x16 en entero (int16) para no usar decimales: se pasa a float y se divide entre 16
    disparity = disparity16.astype(np.float32) / 16.0
    return disparity


# TODO 3: calcular el mapa de disparidad con semi-global block matching (cv2.StereoSGBM).
# Devuelve la disparidad en pixels (float32). Los pixels sin disparidad válida quedan negativos.
def compute_disparity_sgbm(gray_l, gray_r, num_disparities, block_size):
    # --- TO DO ---
    disparity16 = np.full(gray_l.shape, -16, np.int16)
    # matcher = cv2.StereoSGBM.create(...)
    # Nota: Usar minDisparity = 0, P1 = 8 * block_size * block_size, P2 = 32 * block_size * block_size,
    #       disp12MaxDiff = 0, preFilterCap = 0, uniquenessRatio = 10, speckleWindowSize = 100 y speckleRange = 2
    # disparity16 = matcher.compute(...)

    # compute da la disparidad x16 en entero (int16) para no usar decimales: se pasa a float y se divide entre 16
    disparity = disparity16.astype(np.float32) / 16.0
    return disparity


# TODO 4: pasar a 3D los pixels con disparidad válida (d > 0), con las ecuaciones del par rectificado.
# Devuelve (points, colors)
def disparity_to_points(disparity, img_l, camera_matrix, baseline):
    f = camera_matrix[0, 0]
    cx = camera_matrix[0, 2]
    cy = camera_matrix[1, 2]
    # --- TO DO ---
    points = np.zeros((0, 3), np.float32)
    colors = np.zeros((0, 3), np.uint8)
    # Pista: ys, xs = np.nonzero(disparity > 0) da las filas y las columnas de los pixels válidos, y
    #        disparity[ys, xs] / img_l[ys, xs] sus disparidades / colores. np.stack([X, Y, Z], axis=1) junta las columnas
    # Nota: Z = baseline * f / d     X = (x - cx) * Z / f     Y = (y - cy) * Z / f
    return points, colors


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
    print(f"Cannot read {folder}/img0_l.png + img0_r.png (or .jpg)", file=sys.stderr)
    sys.exit(-1)
img_l, img_r = pair
print(f"Image {img_l.shape[1]}x{img_l.shape[0]}")


#### 3. RECTIFICAMOS EL PAR
R1, R2, P1, P2 = compute_rectification(Kl, Dl, Kr, Dr, R, t, (img_l.shape[1], img_l.shape[0]))
if R1 is None or R2 is None or P1 is None or P2 is None:
    print("Pair not rectified (is TODO 1 done?)", file=sys.stderr)
    sys.exit(-1)
rect_l = rectify_image(img_l, Kl, Dl, R1, P1)
rect_r = rectify_image(img_r, Kr, Dr, R2, P2)
K = P1[:, :3]

# baseline: distancia entre los centros de las dos cámaras (rectificar las gira, pero no las mueve)
baseline = np.linalg.norm(t)
print(f"Baseline: {baseline}")


#### 4. CREAMOS LAS VENTANAS: par rectificado, disparidad y 3D
pair = np.hstack([rect_l, rect_r])
cv2.putText(pair, "[s] save reconstruction.obj   [ESC] exit", (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
cv2.imshow("stereo", pair)
cv2.namedWindow("disparity")
cv2.createTrackbar("SGBM", "disparity", 0, 1, nothing)
cv2.createTrackbar("numDisp x16", "disparity", 9, 16, nothing)
cv2.createTrackbar("blockSize", "disparity", 9, 31, nothing)
viewer = Viewer3D("3D", 1)


#### 5. CALCULAMOS LA DISPARIDAD Y LOS PUNTOS 3D CADA VEZ QUE CAMBIA UN PARAMETRO
# BM y SGBM trabajan con las imágenes rectificadas en gris
gray_l = cv2.cvtColor(rect_l, cv2.COLOR_BGR2GRAY)
gray_r = cv2.cvtColor(rect_r, cv2.COLOR_BGR2GRAY)
last_params = None
points, colors = np.zeros((0, 3), np.float32), np.zeros((0, 3), np.uint8)
key = 0
while key != 27:
    use_sgbm = cv2.getTrackbarPos("SGBM", "disparity")
    num_disparities = 16 * max(1, cv2.getTrackbarPos("numDisp x16", "disparity"))
    block_size = max(5, cv2.getTrackbarPos("blockSize", "disparity")) | 1      # impar y >= 5
    params = (use_sgbm, num_disparities, block_size)

    if params != last_params:
        last_params = params

        # disparidad
        if use_sgbm:
            disparity = compute_disparity_sgbm(gray_l, gray_r, num_disparities, block_size)
        else:
            disparity = compute_disparity_bm(gray_l, gray_r, num_disparities, block_size)

        # puntos 3D
        points, colors = disparity_to_points(disparity, rect_l, K, baseline)
        viewer.set_points(points, colors)

        # mostramos la disparidad
        name = "SGBM" if use_sgbm else "BM"
        print(f"{name} numDisparities={num_disparities} blockSize={block_size}: {len(points)} points")
        disparity_color = colorize_disparity(disparity, num_disparities)
        cv2.putText(disparity_color, name, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        cv2.imshow("disparity", disparity_color)

    viewer.render()
    key = cv2.waitKey(30)
    if key == ord("s"):
        write_obj("reconstruction.obj", points, colors)
        print(f"Saved reconstruction.obj ({len(points)} points)")
cv2.destroyAllWindows()
