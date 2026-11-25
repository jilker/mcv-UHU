import os
import sys

import cv2
import numpy as np

# Esquinas interiores del tablero y lado de una casilla (se leen por línea de comandos)
board_size = None
square_size = None


# Busca los pares de imágenes de la carpeta: img0_l / img0_r, img1_l / img1_r, ... (.jpg o .png).
# Devuelve (images_left, images_right).
def find_stereo_pairs(folder):
    images_left, images_right = [], []
    i = 0
    while True:
        found = False
        for ext in (".jpg", ".png"):
            left = f"{folder}/img{i}_l{ext}"
            right = f"{folder}/img{i}_r{ext}"
            if os.path.exists(left) and os.path.exists(right):
                images_left.append(left)
                images_right.append(right)
                found = True
                break
        if not found:
            break
        i += 1
    return images_left, images_right


# Coordenadas 3D de las esquinas del tablero (Z = 0)
def get_board_points_3d():
    corners_3d = []
    for i in range(board_size[1]):
        for j in range(board_size[0]):
            corners_3d.append((j * square_size, i * square_size, 0))
    return np.array(corners_3d, np.float32)


# Detecta y refina las esquinas del tablero
# Devuelve (found, corners).
def detect_chessboard(gray):
    found, corners = cv2.findChessboardCorners(gray, board_size,
                                               flags=cv2.CALIB_CB_ADAPTIVE_THRESH | cv2.CALIB_CB_NORMALIZE_IMAGE)
    if found:
        criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
        corners = cv2.cornerSubPix(gray, corners, (7, 7), (-1, -1), criteria)
    return found, corners


# TODO 1: calibrar una cámara con los puntos 3D y 2D de todas las imágenes (como en la clase 7).
# Devuelve (error de reproyección, camera_matrix, dist_coeffs).
def calibrate(object_points, image_points, image_size):
    # --- TO DO ---
    # error, camera_matrix, dist_coeffs, R, t = cv2.calibrateCamera(...)
    return -1, None, None


# TODO 2: calibrar extrínsecos del par stereo: rotación R y traslación t de la cámara derecha respecto a la izquierda.
# Las intrínsecas de cada cámara ya están calculadas y se mantienen.
# Devuelve (error de reproyección, R, t).
def calibrate_stereo(object_points, image_points_l, image_points_r, camera_matrix_l, dist_coeffs_l,
                     camera_matrix_r, dist_coeffs_r, image_size):
    # --- TO DO ---
    criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_MAX_ITER, 100, 1e-6)
    # stereoCalibrate también devuelve E y F
    # error, _, _, _, _, R, t, _, _ = cv2.stereoCalibrate(...)
    # Nota: Usar flags = cv2.CALIB_FIX_INTRINSIC (no se tocan las intrínsecas de cada cámara) y el criteria
    #       de arriba como criterio de parada
    return -1, None, None


# Guarda la calibración stereo en un .yml.
def save_stereo_calibration_to_file(path, camera_matrix_l, dist_coeffs_l, camera_matrix_r, dist_coeffs_r,
                                    R, t):
    fs = cv2.FileStorage(path, cv2.FILE_STORAGE_WRITE)
    if not fs.isOpened():
        return False
    fs.write("cameraMatrixL", camera_matrix_l)
    fs.write("distCoeffsL", dist_coeffs_l)
    fs.write("cameraMatrixR", camera_matrix_r)
    fs.write("distCoeffsR", dist_coeffs_r)
    fs.write("R", R)
    fs.write("T", t)
    fs.release()
    return True


#### 1. LEEMOS PARAMETROS
if len(sys.argv) != 5:
    print("Usage: folder corners_cols corners_rows square_size", file=sys.stderr)
    sys.exit(-1)
folder = sys.argv[1]
board_size = (int(sys.argv[2]), int(sys.argv[3]))
square_size = float(sys.argv[4])


#### 2. LEEMOS RUTAS IMAGENES
images_left, images_right = find_stereo_pairs(folder)
if not images_left:
    print(f"No image pairs in {folder} (img0_l.jpg + img0_r.jpg, img1_l.jpg + img1_r.jpg, ...)", file=sys.stderr)
    sys.exit(-1)

object_points = []
image_points_l = []
image_points_r = []
image_size = None


#### 3. DETECTAMOS PATRÓN EN CADA PAR DE IMÁGENES
for file_l, file_r in zip(images_left, images_right):

    # 3.1 LEER IMAGENES
    frame_l = cv2.imread(file_l)
    frame_r = cv2.imread(file_r)
    print(f"Reading..{file_l} | {file_r} ({frame_l.shape[1]}x{frame_l.shape[0]})")
    gray_l = cv2.cvtColor(frame_l, cv2.COLOR_BGR2GRAY)
    gray_r = cv2.cvtColor(frame_r, cv2.COLOR_BGR2GRAY)
    image_size = (gray_l.shape[1], gray_l.shape[0])

    # 3.2 DETECTAMOS IMAGE POINTS EN LAS DOS IMAGENES
    success_l, corners_l = detect_chessboard(gray_l)
    success_r, corners_r = detect_chessboard(gray_r)

    # 3.3 SOLO SE USA EL PAR SI EL TABLERO APARECE EN LAS DOS
    success = success_l and success_r
    if success:
        object_points.append(get_board_points_3d())
        image_points_l.append(corners_l)
        image_points_r.append(corners_r)

    # 3.4 MOSTRAMOS RESULTADO: izquierda | derecha
    if corners_l is not None:
        cv2.drawChessboardCorners(frame_l, board_size, corners_l, success_l)
    if corners_r is not None:
        cv2.drawChessboardCorners(frame_r, board_size, corners_r, success_r)
    cv2.imshow("left | right", np.hstack([frame_l, frame_r]))
    cv2.waitKey(0)
cv2.destroyAllWindows()

if len(object_points) < 3:
    print(f"Chessboard found in {len(object_points)} pairs, at least 3 are needed", file=sys.stderr)
    sys.exit(-1)


#### 4. CALIBRAMOS CADA CAMARA POR SEPARADO
err_l, camera_matrix_l, dist_coeffs_l = calibrate(object_points, image_points_l, image_size)
err_r, camera_matrix_r, dist_coeffs_r = calibrate(object_points, image_points_r, image_size)
if camera_matrix_l is None or camera_matrix_r is None:
    print("Cameras not calibrated (is TODO 1 done?)", file=sys.stderr)
    sys.exit(-1)


#### 5. CALIBRAMOS EL PAR STEREO
err, R, t = calibrate_stereo(object_points, image_points_l, image_points_r, camera_matrix_l, dist_coeffs_l,
                             camera_matrix_r, dist_coeffs_r, image_size)
if R is None:
    print("Stereo calibration not done (is TODO 2 done?)", file=sys.stderr)
    sys.exit(-1)


#### 6. MOSTRAMOS RESULTADO Y GUARDAMOS A FICHERO
print(f"Pairs used: {len(object_points)}/{len(images_left)}")
print(f"Reprojection error: left {err_l}, right {err_r}, stereo {err}")
print(f"R : {R}")
print(f"t : {t}")
print(f"Baseline |t| : {np.linalg.norm(t)}")

if not save_stereo_calibration_to_file("calibration.yml", camera_matrix_l, dist_coeffs_l,
                                       camera_matrix_r, dist_coeffs_r, R, t):
    print("Cannot write calibration.yml", file=sys.stderr)
    sys.exit(-1)
print("Saved calibration.yml")
