import sys

import cv2
import numpy as np

from models import MODEL_NAMES, get_model

# Lado de la base del modelo 3D, en metros
MODEL_SIZE = 0.12


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


# Lee un board (mapa de marcadores) de fichero: diccionario y, para cada marcador, su id y sus 4 esquinas
# Devuelve el cv2.aruco.Board, o None si no se ha podido leer.
def load_board(path):
    fs = cv2.FileStorage(path, cv2.FILE_STORAGE_READ)
    if not fs.isOpened():
        return None
    dictionary = cv2.aruco.getPredefinedDictionary(int(fs.getNode("dictionary").real()))
    markers = fs.getNode("markers")
    ids, obj_points = [], []
    for i in range(markers.size()):
        corners = markers.at(i).getNode("corners").mat()
        if corners is None or corners.shape != (4, 3):
            return None
        ids.append(int(markers.at(i).getNode("id").real()))
        obj_points.append(corners.astype(np.float32))
    fs.release()
    if not ids:
        return None
    return cv2.aruco.Board(obj_points, dictionary, np.array(ids, np.int32))


#### 1. LEEMOS PARAMETROS
if len(sys.argv) != 5:
    print("Usage: video_file|camera_index calibration_file board_file model_id", file=sys.stderr)
    for i, name in enumerate(MODEL_NAMES):
        print(f"    model_id {i}: {name}", file=sys.stderr)
    sys.exit(-1)
source = sys.argv[1]
calibration_file = sys.argv[2]
board_file = sys.argv[3]
model_id = int(sys.argv[4])
if not 0 <= model_id < len(MODEL_NAMES):
    print(f"Invalid model_id: {sys.argv[4]}", file=sys.stderr)
    sys.exit(-1)



#### 2. LEEMOS LA CALIBRACION
camera_matrix, dist_coeffs = load_calibration(calibration_file)
if camera_matrix is None:
    print(f"Cannot read {calibration_file}", file=sys.stderr)
    sys.exit(-1)

#### 3. LEEMOS EL BOARD
board = load_board(board_file)
if board is None:
    print(f"Cannot read {board_file}", file=sys.stderr)
    sys.exit(-1)

#### 4. ABRIMOS EL VIDEO (O LA CAMARA, SI SE INDICA SU NUMERO)
cap = cv2.VideoCapture(int(source) if source.isdigit() else source)
if not cap.isOpened():
    print(f"Cannot open {source}", file=sys.stderr)
    sys.exit(-1)

#### 5. CREAMOS EL DETECTOR DE MARCADORES
# TODO 1: crear el detector de marcadores con el diccionario del board (como en P2).
# --- TO DO ---
# detector = cv2.aruco.ArucoDetector(...)
# Pista: board.getDictionary() devuelve el diccionario del board

#### 6. PROCESAMOS CADA FRAME
while True:
    ok, frame = cap.read()
    if not ok:
        break

    # Copia de la imagen para dibujar encima
    canvas = frame.copy()

    # 6.1 DETECTAMOS LOS MARCADORES Y ESTIMAMOS LA POSE DEL BOARD
    # TODO 2: detectar los marcadores y estimar la pose (rvec, tvec) del board (como en P2).
    pose_found = False
    # --- TO DO ---

    # 6.2 DIBUJAMOS EL MODELO 3D SOBRE EL BOARD
    if pose_found:
        # Modelo en su sistema de referencia local (cambia con cada llamada según animación)
        model = get_model(model_id, MODEL_SIZE)

        # TODO 3: proyectar los vértices del modelo en la imagen.
        # --- TO DO ---
        # points_reprojected, _ = cv2.projectPoints(...)
        # Nota: model.vertices ya está en el sistema de referencia del board, así que se proyecta con su pose

        # TODO 4: dibujar las aristas del modelo.
        # --- TO DO ---
        # Cada fila i de model.edges es un par (a, b) con los índices de los dos vértices que hay que unir:
        # unir points_reprojected[a] y points_reprojected[b] con cv2.line(...) en canvas, con el color
        # model.colors[i]
        # Pista: zip(model.edges, model.colors) recorre a la vez cada arista y su color
        # Nota: cv2.line necesita los puntos como tuplas de enteros; antes, points_reprojected.reshape(-1, 2)
        # quita la dimensión que añade cv2.projectPoints

    # 6.3 MOSTRAR IMAGEN
    cv2.putText(canvas, "[space] pause   [ESC] exit", (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    cv2.imshow("P3 ar", canvas)
    key = cv2.waitKey(30)
    if key == 27:
        break
    if key == ord(" "):
        cv2.waitKey(0)
cv2.destroyAllWindows()
