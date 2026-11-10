import glob
import sys

import cv2
import numpy as np


# Lee la calibración guardada por la práctica P1
# Devuelve (camera_matrix, dist_coeffs), o (None, None) si no se ha podido leer.
def load_calibration(path):
    fs = cv2.FileStorage(path, cv2.FILE_STORAGE_READ)
    if not fs.isOpened():
        return None, None
    camera_matrix = fs.getNode("cameraMatrix").mat()
    dist_coeffs = fs.getNode("distCoeffs").mat()
    fs.release()
    if camera_matrix is None or dist_coeffs is None or camera_matrix.size == 0 or dist_coeffs.size == 0:
        return None, None
    return camera_matrix, dist_coeffs



# TODO 1: quitar la distorsión de la imagen con la calibración de la cámara.
def undistort_image(img, camera_matrix, dist_coeffs):
    # --- TO DO ---
    undistorted = None
    # undistorted = cv2.undistort(...)
    return undistorted


#### 1. LEEMOS PARAMETROS
if len(sys.argv) != 3:
    print("Usage: calibration_file folder_with_images", file=sys.stderr)
    sys.exit(-1)
calibration_file = sys.argv[1]
images_folder = sys.argv[2]



#### 2. LEEMOS CALIBRACION Y RUTAS IMAGENES
camera_matrix, dist_coeffs = load_calibration(calibration_file)
if camera_matrix is None or dist_coeffs is None:
    print(f"Cannot read {calibration_file}", file=sys.stderr)
    sys.exit(-1)

images = []
for ext in ("/*.jpg", "/*.png"):
    images += sorted(glob.glob(images_folder + ext))

#### 3. QUITAMOS LA DISTORSIÓN DE CADA IMAGEN
for file in images:

    # 3.1 LEER IMAGEN
    frame = cv2.imread(file)
    print(f"Reading..{file} ({frame.shape[1]}x{frame.shape[0]})")

    # 3.2 QUITAMOS LA DISTORSIÓN
    undistorted = undistort_image(frame, camera_matrix, dist_coeffs)
    if undistorted is None or undistorted.size == 0:
        print("Undistorted image is empty (is TODO 1 done?)", file=sys.stderr)
        sys.exit(-1)

    # 3.3 MOSTRAMOS RESULTADO: original a la izquierda, sin distorsión a la derecha
    cv2.imshow("original | undistorted", np.hstack([frame, undistorted]))
    cv2.waitKey(0)
cv2.destroyAllWindows()
