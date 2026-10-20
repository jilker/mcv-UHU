import cv2

IMAGE_1 = "../data/1.jpg"
IMAGE_2 = "../data/2.jpg"
WINDOW_KEYPOINTS = "P1 keypoints"
WINDOW_MATCHES = "P1 matches"
MAX_WIDTH = 1000
DISPLAY_HEIGHT = 500


def nothing(value):
    pass


def load(path):
    """Lee la imagen y la reduce a MAX_WIDTH de ancho si es más grande."""
    img = cv2.imread(path)
    if img is None:
        print(f"Cannot read {path}")
        exit(1)
    if img.shape[1] > MAX_WIDTH:
        scale = MAX_WIDTH / img.shape[1]
        img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_AREA)
    return img


def show(window, img):
    """Muestra la imagen reducida a DISPLAY_HEIGHT de alto si es más alta. Solo cambia lo que se ve."""
    if img.shape[0] > DISPLAY_HEIGHT:
        scale = DISPLAY_HEIGHT / img.shape[0]
        img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_AREA)
    cv2.imshow(window, img)


def detect_features(img):
    """Keypoints y descriptores SIFT de la imagen.

    Args:
        img: imagen BGR.

    Returns:
        (keypoints, descriptors): lista de cv2.KeyPoint y array float32 de N filas x 128 columnas,
        un descriptor por keypoint, en el mismo orden.
    """
    ## TODO: CODE HERE
    keypoints, descriptors = [], None
    ## END TODO
    return keypoints, descriptors


def match_features(desc1, desc2, ratio):
    """Emparejamientos de desc1 con desc2.

    Para cada descriptor de desc1 se buscan sus 2 vecinos más cercanos en desc2. El emparejamiento con
    el más cercano se acepta solo si está claramente más cerca que el segundo:
    distancia_1 < ratio * distancia_2.

    PISTA: matcher.knnMatch -> devuelve knn.
    Cada knn[i] tiene 2 cv2.DMatch, ordenados del más cercano al más lejano.
    La distancia está en .distance.

    Args:
        desc1, desc2: descriptores SIFT de la imagen 1 y de la imagen 2.
        ratio: [0,1] -> cuanto más pequeño, más exigente.

    Returns:
        Los emparejamientos.
    """
    matcher = cv2.BFMatcher(cv2.NORM_L2)
    ## TODO: CODE HERE
    matches = []
    ## END TODO
    return matches


## 1. Las dos imágenes
img1 = load(IMAGE_1)
img2 = load(IMAGE_2)

## 2. Keypoints y descriptores: se calculan una vez
kp1, desc1 = detect_features(img1)
kp2, desc2 = detect_features(img2)
print(f"Keypoints: {len(kp1)} in image 1, {len(kp2)} in image 2")
both = cv2.drawMatches(img1, kp1, img2, kp2, [], None, singlePointColor=(0, 255, 0),
                       flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
show(WINDOW_KEYPOINTS, both)

## 3. Emparejamientos, con el cociente en un trackbar
cv2.namedWindow(WINDOW_MATCHES)
cv2.createTrackbar("ratio x100", WINDOW_MATCHES, 75, 100, nothing)

last_ratio = None
while True:
    ratio = max(1, cv2.getTrackbarPos("ratio x100", WINDOW_MATCHES)) / 100
    if ratio != last_ratio:
        last_ratio = ratio
        if desc1 is not None and desc2 is not None:
            matches = match_features(desc1, desc2, ratio)
        else:
            matches = []
        shown = cv2.drawMatches(img1, kp1, img2, kp2, matches, None,
                                flags=cv2.DRAW_MATCHES_FLAGS_NOT_DRAW_SINGLE_POINTS)
        show(WINDOW_MATCHES, shown)
        cv2.setWindowTitle(WINDOW_MATCHES, f"ratio {ratio:.2f}: {len(matches)} matches")
    if cv2.waitKey(50) == 27:
        break

cv2.destroyAllWindows()
