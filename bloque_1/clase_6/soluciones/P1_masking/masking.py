import cv2
import numpy as np

IMAGE = "../data/nfl_field.jpg"
TEMPLATE = "../data/plantilla.png"
SCALES = [0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2]
ROTATE_TEMPLATE = False
MAX_CANDIDATES = 2000
WINDOW = "P1 detections"
WINDOW_RESPONSE = "P1 response"
DISPLAY_HEIGHT = 520
FONT = cv2.FONT_HERSHEY_SIMPLEX


def nothing(value):
    pass


def show(window, img):
    if img.shape[0] > DISPLAY_HEIGHT:
        scale = DISPLAY_HEIGHT / img.shape[0]
        img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_AREA)
    cv2.imshow(window, img)


def scaled_size(templ, scale):
    return round(templ.shape[1] * scale), round(templ.shape[0] * scale)


def match(gray, templ, scale):
    """Mapa de respuesta de la plantilla escalada sobre la imagen.

    La plantilla se escala a scaled_size(templ, scale) y se desliza por toda la imagen.
    En cada posición se mide el parecido con la correlación normalizada (cv2.TM_CCOEFF_NORMED)
    1 es idéntica, 0 no se parecen.

    PISTA: cv2.resize cv2.matchTemplate
    Args:
        gray: imagen en gris.
        templ: plantilla en gris, a su tamaño original.
        scale: factor de escala de la plantilla.

    Returns:
        Salida de cv2.matchTemplate
    """
    ## TODO: CODE HERE
    resized = cv2.resize(templ, scaled_size(templ, scale), interpolation=cv2.INTER_AREA)
    response = cv2.matchTemplate(gray, resized, cv2.TM_CCOEFF_NORMED)
    ## END TODO
    return response


def iou(a, b):
    """Intersección sobre unión de dos BBs [x, y, w, h]: área común / área de las dos juntas.

    Returns:
        Entre 0 (no se tocan) y 1 (son la misma BB).
    """
    ## TODO: CODE HERE
    a_x_min, a_y_min = a[0], a[1]
    a_x_max, a_y_max = a[0] + a[2], a[1] + a[3]
    b_x_min, b_y_min = b[0], b[1]
    b_x_max, b_y_max = b[0] + b[2], b[1] + b[3]

    # Dimensiones de la intersección: 0 si no se solapan
    inter_width = max(0, min(a_x_max, b_x_max) - max(a_x_min, b_x_min))
    inter_height = max(0, min(a_y_max, b_y_max) - max(a_y_min, b_y_min))

    # Áreas
    inter_area = inter_width * inter_height
    union_area = a[2] * a[3] + b[2] * b[3] - inter_area

    result = inter_area / union_area
    ## END TODO
    return result


def nms(boxes, scores, iou_threshold):
    """Supresión de no máximos: índices de las BBs que se quedan.

    Se recorren las BBs de mayor a menor puntuación. Cada una se queda si no se solapa
    (IoU > iou_threshold) con ninguna de las que ya se han quedado.

    Args:
        boxes: lista de BBs [x, y, w, h].
        scores: puntuación de cada BB.
        iou_threshold: Valores [0, 1]. Cuanto más pequeño, más BBs se eliminan.

    Returns:
        Lista de índices en boxes, de mayor a menor puntuación.
    """
    keep = []
    order = np.argsort(scores)[::-1]            # índices de las BBs, de mayor a menor puntuación
    ## TODO: CODE HERE
    # for i in order:
    #     overlaps = False
    #     for j in keep:                          # las que ya se han quedado
    #         if iou(boxes[i], boxes[j]) > iou_threshold:
    #             overlaps = True
    #             break
    #     if not overlaps:
    #         keep.append(i)
    ## END TODO
    return keep


def candidates(responses, templ, threshold):
    """BBs de todas las escalas cuya respuesta pasa el umbral: como mucho, las MAX_CANDIDATES mejores."""
    boxes, scores = [], []
    for scale, response in zip(SCALES, responses):
        w, h = scaled_size(templ, scale)
        ys, xs = np.where(response > threshold)
        boxes += [[int(x), int(y), w, h] for x, y in zip(xs, ys)]
        scores += response[ys, xs].tolist()
    best = np.argsort(scores)[::-1][:MAX_CANDIDATES]
    return [boxes[i] for i in best], [scores[i] for i in best]


def draw(img, boxes, scores, keep):
    """Copia de la imagen con las detecciones de keep en verde, con su puntuación."""
    shown = img.copy()
    for i in keep:
        x, y, w, h = boxes[i]
        cv2.rectangle(shown, (x, y), (x + w, y + h), (0, 255, 0), 2)
        cv2.putText(shown, f"{scores[i]:.2f}", (x, y - 4), FONT, 0.6, (0, 255, 0), 2, cv2.LINE_AA)
    return shown


## 1. La imagen y la plantilla, las dos en gris
img = cv2.imread(IMAGE)
templ = cv2.imread(TEMPLATE)
if img is None or templ is None:
    print(f"Cannot read {IMAGE} or {TEMPLATE}")
    exit(1)
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
templ = cv2.cvtColor(templ, cv2.COLOR_BGR2GRAY)
if ROTATE_TEMPLATE:
    templ = cv2.rotate(templ, cv2.ROTATE_180)

## 2. Un mapa de respuesta por escala: se calculan una vez
responses = [match(gray, templ, scale) for scale in SCALES]

## 3. Detecciones, con el umbral de la respuesta y el de la NMS en trackbars
cv2.namedWindow(WINDOW)
cv2.createTrackbar("threshold x100", WINDOW, 65, 100, nothing)
cv2.createTrackbar("IoU x100", WINDOW, 30, 100, nothing)

# El mapa de respuesta de la escala 1,0. Va de -1 a 1: se pasa a 0..255 para verlo
response = responses[SCALES.index(1.0)]
show(WINDOW_RESPONSE, cv2.applyColorMap(np.uint8((response + 1) * 127.5), cv2.COLORMAP_JET))
cv2.setWindowTitle(WINDOW_RESPONSE, f"scale 1.0: max {response.max():.2f}")

# La NMS escrita a mano puede tardar con muchos candidatos: solo se recalcula si cambia un trackbar
last = None
while True:
    threshold = max(50, cv2.getTrackbarPos("threshold x100", WINDOW)) / 100   # por debajo de 0,50, demasiados candidatos
    iou_threshold = cv2.getTrackbarPos("IoU x100", WINDOW) / 100
    if (threshold, iou_threshold) != last:
        last = (threshold, iou_threshold)
        boxes, scores = candidates(responses, templ, threshold)
        keep = nms(boxes, scores, iou_threshold)
        show(WINDOW, draw(img, boxes, scores, keep))
        cv2.setWindowTitle(WINDOW, f"threshold {threshold:.2f}, IoU {iou_threshold:.2f}: "
                                   f"{len(keep)} of {len(boxes)} boxes")
    if cv2.waitKey(50) == 27:
        break

cv2.destroyAllWindows()
