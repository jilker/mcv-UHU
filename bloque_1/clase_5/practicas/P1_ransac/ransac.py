import math

import cv2
import numpy as np

WINDOW = "P1 ransac"
WIDTH, HEIGHT = 800, 600
P = 0.99


def line_fit(points):
    """Ajusta una recta a points con cv2.fitLine."""
    pts = np.array(points, np.float32)
    vx, vy, x0, y0 = cv2.fitLine(pts, cv2.DIST_L2, 0, 0.01, 0.01).ravel()
    p1 = (int(x0 - 1000 * vx), int(y0 - 1000 * vy))
    p2 = (int(x0 + 1000 * vx), int(y0 + 1000 * vy))
    return p1, p2


def distance_to_line(points, p, n):
    """Distancia perpendicular de cada punto a la recta que pasa por p con normal unitaria n: |(x - p) · n|."""
    return np.abs((points - p) @ n)


def ransac_line(points, t, iterations, rng):
    """Busca la recta que tiene más inliers usando RANSAC.

    Args:
        points: array de N x 2 puntos (x, y).
        t: umbral de inlier, en píxeles.
        iterations: número de intentos de RANSAC.
        rng: generador de números aleatorios (np.random.default_rng).

    Returns:
        (inliers, sample): inliers es un array de N bool, True en los inliers, y sample son los 2 puntos
        que definen la recta con más inliers, como array de 2 x 2; None si no se hace ningún intento. Los
        puntos son todos distintos (main no deja añadir uno repetido), así que cualquier par define una recta.
    """
    inliers = np.zeros(len(points), bool)
    sample = None
    n = len(points)
    best = 0
    ## TODO: CODE HERE
    ## END TODO
    return inliers, sample


def num_iterations(p, e, s):
    """Intentos necesarios para que, con probabilidad p, al menos una muestra tenga solo inliers.

    N = log(1 - p) / log(1 - (1 - e)^s), redondeado hacia arriba.

    Args:
        p: probabilidad de éxito que se quiere, por ejemplo 0.99.
        e: fracción de outliers, entre 0 y 1 (excluido).
        s: puntos de cada muestra (2 para una recta).

    Returns:
        N como int, al menos 1.
    """
    n = 1
    if e <= 0:
        return 1
    ## TODO: CODE HERE
    ## END TODO
    return max(1, n)


def draw_band(canvas, sample, t, color):
    """Dibuja la recta que pasa por los 2 puntos de sample y su banda de ±t píxeles."""
    p, q = sample
    d = (q - p) / np.linalg.norm(q - p)
    n = np.array([-d[1], d[0]])
    for offset, thickness in ((0, 1), (t, 1), (-t, 1)):
        a = p + offset * n - 1000 * d
        b = p + offset * n + 1000 * d
        cv2.line(canvas, (int(a[0]), int(a[1])), (int(b[0]), int(b[1])), color, thickness, cv2.LINE_AA)
    for x, y in sample:
        cv2.circle(canvas, (int(x), int(y)), 10, color, 2)


def add_point(x, y):
    if (x, y) in points:
        return False
    points.append((x, y))
    return True


def mouse_clicked(event, x, y, flags, param):
    """OpenCV la llama cada vez que se hace algo con el ratón sobre la ventana."""
    if event == cv2.EVENT_LBUTTONDOWN:
        if add_point(x, y):
            print(f"Point added at ({x}, {y}): {len(points)} points")
        else:
            print(f"Point ({x}, {y}) already added")
    elif event == cv2.EVENT_RBUTTONDOWN and len(points) > 0:
        points.pop()
        print(f"Last point removed: {len(points)} points")


def nothing(value):
    pass


points = []
cv2.namedWindow(WINDOW)
cv2.setMouseCallback(WINDOW, mouse_clicked)
cv2.createTrackbar("t (px)", WINDOW, 15, 100, nothing)
cv2.createTrackbar("iterations", WINDOW, 20, 200, nothing)

seed = 0
noise = np.random.default_rng()

while True:
    t = max(1, cv2.getTrackbarPos("t (px)", WINDOW))
    iterations = max(1, cv2.getTrackbarPos("iterations", WINDOW))
    canvas = np.zeros((HEIGHT, WIDTH, 3), np.uint8)
    title = "not enough points"

    pts = np.array(points, np.float32).reshape(-1, 2)
    color = [(255, 255, 255)] * len(pts)        # blanco: outlier

    if len(pts) >= 2:
        ## 1. Mínimos cuadrados con todos los puntos, en naranja
        p1, p2 = line_fit(pts)
        cv2.line(canvas, p1, p2, (0, 140, 255), 2, cv2.LINE_AA)

        ## 2. RANSAC: mejor hipótesis con su banda (gris) y ajuste final con sus inliers (verde)
        rng = np.random.default_rng(seed)
        inliers, sample = ransac_line(pts, t, iterations, rng)
        title = "no line found"
        if sample is not None:
            draw_band(canvas, sample, t, (128, 128, 128))
            p1, p2 = line_fit(pts[inliers])
            cv2.line(canvas, p1, p2, (0, 255, 0), 2, cv2.LINE_AA)
            for i in np.flatnonzero(inliers):
                color[i] = (0, 255, 0)
            e = (~inliers).sum() / len(pts)
            title = (f"inliers {inliers.sum()}/{len(pts)}   N needed (p = {P}, e = {e:.2f}) = "
                     f"{num_iterations(P, e, 2)}")

    for (x, y), c in zip(points, color):
        cv2.circle(canvas, (x, y), 5, c, -1)

    cv2.imshow(WINDOW, canvas)
    cv2.setWindowTitle(WINDOW, f"{title}   (seed {seed})")
    key = cv2.waitKey(30)
    if key == 27:
        break
    elif key == ord("c"):
        points.clear()
        print("All points cleared")
    elif key == ord("o"):
        added = 0
        while added < 5:
            added += add_point(int(noise.integers(WIDTH)), int(noise.integers(HEIGHT)))
        print(f"5 random outliers added: {len(points)} points")
    elif key == ord("r"):
        seed = seed + 1

cv2.destroyAllWindows()
