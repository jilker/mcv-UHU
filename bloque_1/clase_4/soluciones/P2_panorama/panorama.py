import cv2
import numpy as np

IMAGES = ["../data/1.jpg", "../data/2.jpg", "../data/3.jpg"]
REFERENCE = 1
WINDOW_PANORAMA = "P2 panorama"
WINDOW_STITCHER = "P2 Stitcher"
MAX_WIDTH = 1000
DISPLAY_HEIGHT = 500
RATIO = 0.60
RANSAC_THRESHOLD = 3.0
MAX_CANVAS = 5


def detect_features(img):
    """Keypoints y descriptores SIFT de la imagen. Copiar la solución de la P1."""
    ## TODO: CODE HERE
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    sift = cv2.SIFT.create()
    keypoints, descriptors = sift.detectAndCompute(gray, None)
    ## END TODO
    return keypoints, descriptors


def match_features(desc1, desc2, ratio):
    """Emparejamientos que pasan el test. Copiar la solución de la P1."""
    matcher = cv2.BFMatcher(cv2.NORM_L2)
    ## TODO: CODE HERE
    knn = matcher.knnMatch(desc1, desc2, k=2) 
    matches = []
    for neighbours in knn:
        best = neighbours[0]
        second = neighbours[1]
        if best.distance < ratio * second.distance:
            matches.append(best)
    ## END TODO
    return matches


def estimate_homography(kp1, kp2, matches):
    """Homografía H que lleva los puntos de la imagen 2 a la imagen 1: x1 ~ H x2.

    PISTA: los puntos de la imagen 2 son kp2[m.trainIdx].pt y los de la 1, kp1[m.queryIdx].pt.

    Args:
        kp1, kp2: keypoints de la imagen 1 y de la imagen 2.
        matches: emparejamientos (queryIdx en la imagen 1, trainIdx en la 2).

    Returns:
        (H, inliers): H es un array 3 x 3 de float64, o None si no se puede estimar; inliers es un 0 o
        un 1 por emparejamiento (sale del mask de cv2.findHomography).
    """
    threshold = RANSAC_THRESHOLD
    ## TODO: CODE HERE
    inliers = [0] * len(matches)
    pts1 = np.float32([kp1[m.queryIdx].pt for m in matches])
    pts2 = np.float32([kp2[m.trainIdx].pt for m in matches])
    H, mask = cv2.findHomography(pts2, pts1, cv2.RANSAC, threshold)
    if H is not None:
        inliers = mask.ravel().tolist()
    ## END TODO
    return H, inliers


def stitch(img1, img2, H):
    """
    Args:
        img1, img2: imágenes BGR.
        H: homografía 3 x 3 (float64) que lleva la imagen 2 a la 1.

    Returns:
        La panorámica (BGR), o None si el lienzo sale demasiado grande (H mal estimada).
    """
    h1, w1 = img1.shape[:2]
    h2, w2 = img2.shape[:2]

    # 1. Calcular el rectángulo que contiene las esquinas de ambas imágenes, considerando la homografía H.
    #    perspectiveTransform quiere los puntos con forma (N, 1, 2)
    corners1 = np.float32([[0, 0], [w1, 0], [w1, h1], [0, h1]]).reshape(-1, 1, 2)
    corners2 = np.float32([[0, 0], [w2, 0], [w2, h2], [0, h2]]).reshape(-1, 1, 2)
    corners = np.vstack([corners1, cv2.perspectiveTransform(corners2, H)]).reshape(-1, 2)  # Solo deformamos las esquinas de la imagen 2
    x_min, y_min = np.floor(corners.min(axis=0)).astype(int)
    x_max, y_max = np.ceil(corners.max(axis=0)).astype(int)
    width, height = x_max - x_min, y_max - y_min
    # Sanity check: si el lienzo es demasiado grande, H está mal estimada
    if width * height > MAX_CANVAS * w1 * h1:
        return None

    # 2. Calcular la traslación T que lleva (x_min, y_min) a (0, 0) para que warpPerspective no pierda las coordenadas negativas.
    T = np.array([[1, 0, -x_min], [0, 1, -y_min], [0, 0, 1]], np.float64)

    # 3. Deformar la imagen 2 con TH y copiar la imagen 1 en la posición correspondiente.
    TH = T @ H
    panorama = cv2.warpPerspective(img2, TH, (width, height))  # Deformamos la imagen 2 con la homografía compuesta TH
    panorama[-y_min:-y_min + h1, -x_min:-x_min + w1] = img1
    return panorama


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


## 1. Las imágenes
images = [load(path) for path in IMAGES]
panorama = images[REFERENCE]
for i, img in enumerate(images):
    if i == REFERENCE:
        continue
    kp1, desc1 = detect_features(panorama)
    kp2, desc2 = detect_features(img)
    matches = match_features(desc1, desc2, RATIO) if desc1 is not None and desc2 is not None else []
    H, inliers = None, [0] * len(matches)
    if len(matches) >= 4:   # findHomography necesita al menos 4 emparejamientos
        H, inliers = estimate_homography(kp1, kp2, matches)
    print(f"Image {i + 1}: {len(matches)} matches, {sum(inliers)} inliers")
    print(f"H =\n{H}")
    shown = cv2.drawMatches(panorama, kp1, img, kp2, matches, None, matchColor=(0, 255, 0),
                            matchesMask=inliers, flags=cv2.DRAW_MATCHES_FLAGS_NOT_DRAW_SINGLE_POINTS)
    window = f"P2 inliers, image {i + 1}"
    show(window, shown)
    cv2.setWindowTitle(window, f"image {i + 1}: {sum(inliers)} inliers of {len(matches)} matches")

    if H is None:
        print(f"Not enough matches to estimate H for image {i + 1}")
        continue
    stitched = stitch(panorama, img, H)
    if stitched is None:
        print(f"The panorama is too big: H of image {i + 1} is wrong")
        continue
    panorama = stitched

## 2. Panorámica
show(WINDOW_PANORAMA, panorama)
cv2.imwrite("panorama.jpg", panorama)

## 3. Lo mismo con cv2.Stitcher, para comparar: además ajusta la exposición y difumina la costura
stitcher = cv2.Stitcher_create(cv2.Stitcher_PANORAMA)
status, reference = stitcher.stitch(images)
if status == cv2.Stitcher_OK:
    show(WINDOW_STITCHER, reference)
else:
    print(f"cv2.Stitcher failed with status {status}")

cv2.waitKey(0)
cv2.destroyAllWindows()
