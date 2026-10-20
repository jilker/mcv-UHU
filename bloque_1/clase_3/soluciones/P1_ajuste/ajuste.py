import sys

import cv2
import numpy as np

WINDOW = "P1 fit"
WIDTH, HEIGHT = 800, 600


def line_fit(points):
    """Ajusta una recta a points con cv2.fitLine.

    Args:
        points: lista de puntos (x, y), en píxeles; al menos 2.

    Returns:
        Dos puntos (p1, p2) de la recta, como tuplas de int, uno a cada lado para que cv2.line la dibuje
        de borde a borde.
    """
    ## TODO: CODE HERE
    pts = np.array(points, np.float32)     
    vx, vy, x0, y0 = cv2.fitLine(pts, cv2.DIST_L2, 0, 0.01, 0.01).ravel()
    p1 = (int(x0 - 1000 * vx), int(y0 - 1000 * vy))    
    p2 = (int(x0 + 1000 * vx), int(y0 + 1000 * vy))
    ## END TODO
    return p1, p2


def poly_fit(points):
    """Ajusta a points el polinomio de grado 4 y = c0 + c1 x + c2 x^2 + c3 x^3 + c4 x^4 con cv2.solve.

    Con 5 puntos la curva pasa exactamente por todos, con más, es el resultado de mínimos cuadrados.

    PISTA: usar cv2.solve con cv2.DECOMP_SVD.

    Args:
        points: lista de puntos (x, y), en píxeles; al menos 5.

    Returns:
        Los coeficientes c0 ... c4, en un array de 5 filas x 1 columna.
    """
    ## TODO: CODE HERE
    n = len(points)
    A = np.zeros((n, 5))
    b = np.zeros((n, 1))
    for i in range(n):
        x, y = points[i]                
        for power in range(5):          
            A[i, power] = x ** power
        b[i, 0] = y
    _, c = cv2.solve(A, b, flags=cv2.DECOMP_SVD)
    ## END TODO
    return c


def poly_curve(c):
    """Un punto del polinomio de coeficientes c por cada columna del lienzo.

    Args:
        c: los coeficientes c0 ... c4, en un array de 5 filas x 1 columna (ver poly_fit).

    Returns:
        Lista de puntos (px, y) como int, uno por cada columna px de 0 a WIDTH - 1, con y limitada entre
        -HEIGHT y 2 * HEIGHT: fuera del lienzo no se ve.
    """
    curve = []
    for px in range(WIDTH):
        y = 0
        for power in range(5):          # y = c0 + c1 x + c2 x^2 + c3 x^3 + c4 x^4
            y = y + c[power, 0] * px ** power
        y = max(-HEIGHT, min(2 * HEIGHT, y))
        curve.append((px, int(y)))
    return curve


def mouse_clicked(event, x, y, flags, param):
    """OpenCV la llama cada vez que se hace algo con el ratón sobre la ventana."""
    if event == cv2.EVENT_LBUTTONDOWN:
        points.append((x, y))
        print(f"Point added at ({x}, {y}): {len(points)} points")
    elif event == cv2.EVENT_RBUTTONDOWN and len(points) > 0:
        points.pop()
        print(f"Last point removed: {len(points)} points")


## Línea de comandos: line o poly
if len(sys.argv) < 2 or sys.argv[1] not in ("line", "poly"):
    print("Usage: python3 ajuste.py line | poly")
    exit(1)
mode = sys.argv[1]


## Ratón: los puntos clicados se guardan en points
points = []
cv2.namedWindow(WINDOW)
cv2.setMouseCallback(WINDOW, mouse_clicked)

while True:
    canvas = np.zeros((HEIGHT, WIDTH, 3), np.uint8)
    for x, y in points:
        cv2.circle(canvas, (x, y), 5, (255, 255, 255), -1)
    title = f"{mode}: not enough points"

    if mode == "line" and len(points) >= 2:
        ## 1. Recta con cv2.fitLine
        p1, p2 = line_fit(points)
        cv2.line(canvas, p1, p2, (0, 0, 255), 2)
        title = "line"

    elif mode == "poly" and len(points) >= 5:
        ## 2. Polinomio de grado 4 con cv2.solve
        curve = poly_curve(poly_fit(points))
        cv2.polylines(canvas, [np.array(curve, np.int32)], False, (0, 0, 255), 2)
        title = "poly degree 4"

    cv2.imshow(WINDOW, canvas)
    cv2.setWindowTitle(WINDOW, f"{title}   ({len(points)} points)")
    key = cv2.waitKey(30)
    if key == 27:
        break
    elif key == ord("c"):
        points.clear()
        print("All points cleared")

cv2.destroyAllWindows()
