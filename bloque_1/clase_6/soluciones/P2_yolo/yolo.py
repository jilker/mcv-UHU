import sys
from pathlib import Path

import cv2
import numpy as np

MODEL = "../data/yolov8n.onnx"
NAMES = "../data/coco.names"
IMAGE = "../data/messi5.jpg"
IMAGE_EXTENSIONS = (".jpg", ".jpeg", ".png", ".bmp")
WINDOW = "P2 YOLO"
INPUT_SIZE = 640
DISPLAY_HEIGHT = 520
FONT = cv2.FONT_HERSHEY_SIMPLEX


def nothing(value):
    pass


def show(window, img):
    if img.shape[0] > DISPLAY_HEIGHT:
        scale = DISPLAY_HEIGHT / img.shape[0]
        img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_AREA)
    cv2.imshow(window, img)


def prepare_input(img):
    """Imagen cuadrada de INPUT_SIZE x INPUT_SIZE para la red, sin deformar la original.

    La imagen se escala para que su lado mayor mida INPUT_SIZE
    y se pega en la esquina superior izquierda de un lienzo gris.
    El resto del lienzo queda gris.

    Args:
        img: imagen BGR de cualquier tamaño.

    Returns:
        (canvas, scale): el lienzo BGR de INPUT_SIZE x INPUT_SIZE y el factor por el que se ha
        multiplicado la imagen.
    """
    canvas = np.full((INPUT_SIZE, INPUT_SIZE, 3), 114, np.uint8)
    ## TODO: CODE HERE
    scale = INPUT_SIZE / max(img.shape[:2])
    width = round(img.shape[1] * scale)
    height = round(img.shape[0] * scale)
    canvas[:height, :width] = cv2.resize(img, (width, height), interpolation=cv2.INTER_LINEAR)
    ## END TODO
    return canvas, scale


def load_net(path):
    """La red YOLOv8, leída del fichero ONNX.

    PISTA: cv2.dnn.readNet. Deduce el formato por la extensión del fichero.

    Args:
        path: ruta del fichero .onnx.

    Returns:
        La red, lista para infer.
    """
    ## TODO: CODE HERE
    net = cv2.dnn.readNet(path)
    ## END TODO
    return net


def infer(net, canvas):
    """Salida de la red para el lienzo de prepare_input.

    La red no recibe la imagen tal cual, sino un blob: un tensor float de 1 x 3 x INPUT_SIZE x INPUT_SIZE
    (imagen, canal, fila, columna), con valores entre 0 y 1 y los canales en orden RGB.
    OpenCV lee las imágenes en BGR y entre 0 y 255. Algo hay que hacer para convertirlas a RGB y a 0..1.
    Con el blob como entrada, la red se ejecuta con forward.

    PISTA: cv2.dnn.blobFromImage, net.setInput y net.forward.

    Args:
        net: la red de load_net.
        canvas: el lienzo BGR de INPUT_SIZE x INPUT_SIZE.

    Returns:
        La salida de la red.
    """
    scale_factor = 1 / 255
    size = (INPUT_SIZE, INPUT_SIZE)
    ## TODO: CODE HERE
    blob = cv2.dnn.blobFromImage(canvas, scale_factor, size, swapRB=True)
    net.setInput(blob)
    output = net.forward()
    ## END TODO
    return output


def decode(output, scale, conf_threshold):
    """Detecciones de la salida de YOLOv8 con confianza mayor que conf_threshold.

    La salida tiene forma (1, 84, 8400): 8400 BBs candidatas, y para cada una 84 números. Los 4
    primeros son la BB (cx, cy, w, h) en píxeles del lienzo de 640 x 640: centro, ancho y alto.
    Los 80 siguientes, la puntuación de cada clase de COCO, entre 0 y 1.
    La confianza de la BB es la mayor de las 80 clases y su clase es la de esa puntuación.

    PISTA: para cada BB (una fila de 84 números), max() y argmax() sobre las 80 columnas de clase,
    row[4:], dan la mayor puntuación y su posición. max es score y argmax es la clase.

    Args:
        output: salida de net.forward(), array float32 de (1, 84, 8400).
        scale: el factor que devolvió prepare_input.
        conf_threshold: entre 0 y 1.

    Returns:
        (boxes, scores, class_ids): listas con una entrada por detección. Cada BB es
        [x, y, w, h], la esquina superior izquierda, el ancho y el alto en píxeles de la imagen original.
    """
    # De (1, 84, 8400) a (84, 8400) y traspuesta a (8400, 84): una fila por BB candidata
    predictions = output[0].T
    boxes, scores, class_ids = [], [], []
    ## TODO: CODE HERE
    for row in predictions:
        class_scores = row[4:]
        score = float(class_scores.max())
        best = int(class_scores.argmax())
        if score > conf_threshold:
            cx = float(row[0]) / scale
            cy = float(row[1]) / scale
            w = float(row[2]) / scale
            h = float(row[3]) / scale
            box = [cx - w / 2, cy - h / 2, w, h]
            boxes.append(box)
            scores.append(score)
            class_ids.append(best)
    ## END TODO
    return boxes, scores, class_ids


def nms(boxes, scores, class_ids, nms_threshold):
    """Supresión de no máximos: índices de las detecciones que se quedan.

    De cada grupo de BBs de la misma clase que se solapan (IoU mayor que nms_threshold) solo se queda
    la de mayor confianza. BBs de clases distintas no se suprimen entre sí.

    PISTA: cv2.dnn.NMSBoxesBatched.

    Args:
        boxes, scores, class_ids: lo que devuelve decode.
        nms_threshold: entre 0 y 1; cuanto más pequeño, más BBs se eliminan.

    Returns:
        Lista de índices en boxes.
    """
    ## TODO: CODE HERE
    keep = list(cv2.dnn.NMSBoxesBatched(boxes, scores, class_ids, 0.0, nms_threshold))
    ## END TODO
    return keep


def draw(img, boxes, scores, class_ids, keep, names, colors):
    """Copia de la imagen con las detecciones de keep: la BB y una etiqueta con la clase y la confianza."""
    shown = img.copy()
    for i in keep:
        x, y, w, h = (round(v) for v in boxes[i])
        color = colors[class_ids[i]]
        cv2.rectangle(shown, (x, y), (x + w, y + h), color, 2)
        label = f"{names[class_ids[i]]} {scores[i]:.2f}"
        (text_w, text_h), baseline = cv2.getTextSize(label, FONT, 0.5, 1)
        y = max(y, text_h + baseline)
        cv2.rectangle(shown, (x, y - text_h - baseline), (x + text_w, y), color, cv2.FILLED)
        cv2.putText(shown, label, (x, y - baseline), FONT, 0.5, (0, 0, 0), 1, cv2.LINE_AA)
    return shown


## 1. La red y los nombres de las 80 clases de COCO
if not Path(MODEL).is_file():
    print(f"Cannot read {MODEL}: run python3 ../data/descargar_modelo.py")
    exit(1)
net = load_net(MODEL)
names = Path(NAMES).read_text().splitlines()
colors = [tuple(int(c) for c in color) for color in np.random.default_rng(0).integers(64, 256, (len(names), 3))]

## 2. La entrada: una imagen, un vídeo o una cámara (python3 yolo.py ../data/vtest.avi, python3 yolo.py cam)
source = sys.argv[1] if len(sys.argv) > 1 else IMAGE
if source == "cam":
    source = "0"
capture = None
if source.lower().endswith(IMAGE_EXTENSIONS):
    frame = cv2.imread(source)
    if frame is None:
        print(f"Cannot read {source}")
        exit(1)
else:
    capture = cv2.VideoCapture(int(source) if source.isdigit() else source)
    if not capture.isOpened():
        print(f"Cannot open {source}")
        exit(1)

## 3. Detección, con los dos umbrales en trackbars
cv2.namedWindow(WINDOW)
cv2.createTrackbar("conf x100", WINDOW, 25, 100, nothing)
cv2.createTrackbar("NMS x100", WINDOW, 45, 100, nothing)

# Con una imagen la red se ejecuta una sola vez: al mover los trackbars solo se repiten decode y nms
output = None
while True:
    if capture is not None:
        ok, frame = capture.read()
        if not ok and not source.isdigit():     # fin del vídeo: vuelta a empezar
            capture.set(cv2.CAP_PROP_POS_FRAMES, 0)
            ok, frame = capture.read()
        if not ok:
            break
    if output is None or capture is not None:
        canvas, scale = prepare_input(frame)
        output = infer(net, canvas)

    conf_threshold = max(1, cv2.getTrackbarPos("conf x100", WINDOW)) / 100
    nms_threshold = cv2.getTrackbarPos("NMS x100", WINDOW) / 100
    boxes, scores, class_ids = decode(output, scale, conf_threshold)
    keep = nms(boxes, scores, class_ids, nms_threshold)

    show(WINDOW, draw(frame, boxes, scores, class_ids, keep, names, colors))
    cv2.setWindowTitle(WINDOW, f"conf {conf_threshold:.2f}, NMS {nms_threshold:.2f}: "
                               f"{len(keep)} of {len(boxes)} boxes")
    if cv2.waitKey(1 if capture is not None else 50) == 27:
        break

cv2.destroyAllWindows()
