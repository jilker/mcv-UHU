import glob
import sys

import cv2
import numpy as np

# Esquinas interiores del tablero (se lee por línea de comandos)
board_size = None


# Coordenadas 3D de las esquinas del tablero (Z = 0, cada cuadro mide 1 unidad),
# en el mismo orden que findChessboardCorners
def define_board_points_3d():
    corners_3d = []
    for i in range(board_size[1]):
        for j in range(board_size[0]):
            corners_3d.append((j, i, 0))
    return np.array(corners_3d, np.float32)



# TODO 1: detectar las esquinas del tablero en la imagen en gris.
# Devuelve (found, corners): found es True si se encuentran todas las esquinas.
def detect_chessboard(gray):
    # --- TO DO ---
    # return cv2.findChessboardCorners(...)
    # Nota: Usar flags: cv2.CALIB_CB_ADAPTIVE_THRESH | cv2.CALIB_CB_FAST_CHECK | cv2.CALIB_CB_NORMALIZE_IMAGE
    return False, None


# TODO 2: refinar las esquinas a precisión subpíxel.
# Devuelve las esquinas refinadas.
def refine_corners(gray, corners):
    # --- TO DO ---
    criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
    # return cv2.cornerSubPix(...)
    # Nota: Usar winSize = (7, 7) y zeroZone = (-1, -1)
    return corners


# TODO 3: calibrar la cámara con los puntos 3D y 2D de todas las imágenes.
# Devuelve (error de reproyección, camera_matrix, dist_coeffs).
def calibrate(object_points, image_points, image_size):
    # --- TO DO ---
    # error, camera_matrix, dist_coeffs, R, T = cv2.calibrateCamera(...)
    return -1, None, None


# Guarda la calibración en un .yml (claves que lee la práctica P2)
def save_calibration_to_file(path, camera_matrix, dist_coeffs):
    fs = cv2.FileStorage(path, cv2.FILE_STORAGE_WRITE)
    if not fs.isOpened():
        return False
    fs.write("cameraMatrix", camera_matrix)
    fs.write("distCoeffs", dist_coeffs)
    fs.release()
    return True


#### 1. LEEMOS PARAMETROS
if len(sys.argv) != 4:
    print("Usage: folder_with_images corners_cols corners_rows", file=sys.stderr)
    sys.exit(-1)
images_folder = sys.argv[1]
board_size = (int(sys.argv[2]), int(sys.argv[3]))



#### 2. LEEMOS RUTAS IMAGENES
images = []
for ext in ("/*.jpg", "/*.png"):
    images += sorted(glob.glob(images_folder + ext))

object_points = []
image_points = []
image_size = None

#### 3. DETECTAMOS PATRÓN EN CADA IMAGEN
for file in images:

    # 3.1 LEER IMAGEN
    frame = cv2.imread(file)
    print(f"Reading..{file} ({frame.shape[1]}x{frame.shape[0]})")
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    image_size = (gray.shape[1], gray.shape[0])

    # 3.2 DEFINIMOS OBJECT POINTS (Nota: puede hacerse fuera del bucle)
    board_points_3d = define_board_points_3d()

    # 3.3 DETECTAMOS IMAGE POINTS
    success, corners = detect_chessboard(gray)

    # 3.4 MOSTRAMOS RESULTADO
    if success:
        corners = refine_corners(gray, corners)
        object_points.append(board_points_3d)
        image_points.append(corners)
    if corners is not None:
        cv2.drawChessboardCorners(frame, board_size, corners, success)

    cv2.imshow("Image", frame)
    cv2.waitKey(0)
cv2.destroyAllWindows()

if not image_points:
    print("No chessboard detected (is TODO 1 done?)", file=sys.stderr)
    sys.exit(-1)

#### 4. CALIBRAMOS LA CAMARA
err, camera_matrix, dist_coeffs = calibrate(object_points, image_points, image_size)

#### 5. MOSTRAMOS RESULTADO Y GUARDAMOS A FICHERO
print(f"Reprojection error: {err}")
print(f"cameraMatrix : {camera_matrix}")
print(f"distCoeffs : {dist_coeffs}")

if not save_calibration_to_file("calibration.yml", camera_matrix, dist_coeffs):
    print("Cannot write calibration.yml", file=sys.stderr)
    sys.exit(-1)
print("Saved calibration.yml")
