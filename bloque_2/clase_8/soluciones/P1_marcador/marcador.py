import sys

import cv2
import numpy as np

DICTIONARY = cv2.aruco.DICT_6X6_250
marker_length = None


# Devuelve (camera_matrix, dist_coeffs), o (None, None) si no se ha podido leer.
def load_calibration(path):
    fs = cv2.FileStorage(path, cv2.FILE_STORAGE_READ)
    if not fs.isOpened():
        return None, None
    camera_matrix = fs.getNode("cameraMatrix").mat()
    dist_coeffs = fs.getNode("distCoeffs").mat()
    fs.release()
    if camera_matrix is None or dist_coeffs is None:
        return None, None
    return camera_matrix, dist_coeffs


# Object Points (puntos 3D) de las esquinas del marcador
# Centrado en el origen. Plano Z = 0. Eje Z hacia fuera.
def marker_points_3d(length):
    h = length / 2
    return np.array([[-h, h, 0], [h, h, 0], [h, -h, 0], [-h, -h, 0]], np.float32)


# TODO 1: detectar los marcadores de la imagen.
# Devuelve (corners, ids): para cada marcador, corners tiene sus 4 esquinas (array 1 x 4 x 2) e ids su
# identificador (array N x 1, o None si no hay ninguno).
def detect_markers(detector, img):
    corners, ids, rejected = detector.detectMarkers(img)
    return corners, ids


# TODO 2: estimar la pose (rvec, tvec) del marcador respecto a la cámara a partir de sus 4 esquinas.
# Devuelve (ok, rvec, tvec): ok es True si se ha podido estimar.
def estimate_marker_pose(corners, camera_matrix, dist_coeffs):
    object_points = marker_points_3d(marker_length)
    return cv2.solvePnP(object_points, corners, camera_matrix, dist_coeffs)


# TODO 3: dibujar los ejes de coordenadas del marcador.
def draw_marker_axes(img, camera_matrix, dist_coeffs, rvec, tvec):
    cv2.drawFrameAxes(img, camera_matrix, dist_coeffs, rvec, tvec, marker_length / 2)


# TODO 4: distancia (en metros) de la cámara al centro del marcador.
def distance_to_camera(tvec):
    return np.linalg.norm(tvec)


#### 1. LEEMOS PARAMETROS
if len(sys.argv) != 4:
    print("Usage: video_file|camera_index calibration_file marker_length_m", file=sys.stderr)
    sys.exit(-1)
source = sys.argv[1]
calibration_file = sys.argv[2]
marker_length = float(sys.argv[3])
if marker_length <= 0:
    print(f"Invalid marker length: {sys.argv[3]}", file=sys.stderr)
    sys.exit(-1)



#### 2. LEEMOS LA CALIBRACION
camera_matrix, dist_coeffs = load_calibration(calibration_file)
if camera_matrix is None:
    print(f"Cannot read {calibration_file}", file=sys.stderr)
    sys.exit(-1)

#### 3. ABRIMOS EL VIDEO (O LA CAMARA, SI SE INDICA SU NUMERO)
cap = cv2.VideoCapture(int(source) if source.isdigit() else source)
if not cap.isOpened():
    print(f"Cannot open {source}", file=sys.stderr)
    sys.exit(-1)

#### 4. CREAMOS EL DETECTOR DE MARCADORES
detector = cv2.aruco.ArucoDetector(cv2.aruco.getPredefinedDictionary(DICTIONARY))

#### 5. PROCESAMOS CADA FRAME
while True:
    ok, frame = cap.read()
    if not ok:
        break

    # Copia de la imagen para dibujar encima
    canvas = frame.copy()

    # 5.1 DETECTAMOS LOS MARCADORES
    corners, ids = detect_markers(detector, frame)
    if ids is not None and len(ids) > 0:
        cv2.aruco.drawDetectedMarkers(canvas, corners, ids)

    # 5.2 POSE DE CADA MARCADOR
    n = 0 if ids is None else len(ids)
    distances = []
    for i in range(n):
        ok, rvec, tvec = estimate_marker_pose(corners[i], camera_matrix, dist_coeffs)
        if not ok:
            continue
        draw_marker_axes(canvas, camera_matrix, dist_coeffs, rvec, tvec)
        distances.append(f"id {int(ids.flatten()[i])}: {distance_to_camera(tvec):.2f} m")
    if distances:
        print("Distances:   " + "   ".join(distances))

    # 5.3 MOSTRAR IMAGEN
    cv2.putText(canvas, "[space] pause   [ESC] exit", (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    cv2.imshow("P1 marcador", canvas)
    key = cv2.waitKey(30)
    if key == 27:
        break
    if key == ord(" "):
        cv2.waitKey(0)
cv2.destroyAllWindows()
