import sys

import cv2
import numpy as np

VIDEO = "../data/vtest.avi"
WINDOW = "P2 optical flow"
MAX_CORNERS = 300       # esquinas nuevas que se buscan en cada fotograma, como mucho
FB_MAX_ERROR = 1.0      # píxeles: un punto que no vuelve a menos de esta distancia se descarta
TRAIL = 15              # fotogramas que se recuerda la trayectoria de cada punto
MOVING = 2.0            # píxeles: un punto que se ha movido más que esto en su trayectoria se pinta en verde


def detect_corners(gray, mask, max_corners):
    """Busqueda de esquinas en la imagen con cv2.goodFeaturesToTrack.

    Parametros:
        Calidad = 0.01 (umbral de calidad de las esquinas)
        Separación = 10 (distancia mínima entre esquinas)
        Bloque = 7 (tamaño del bloque para el cálculo de la esquina)

    Args:
        gray: imagen en gris.
        mask: imagen uint8 del mismo tamaño (solo se buscan esquinas donde vale 255).
        max_corners: número máximo de esquinas.

    Returns:
        Array de N x 2 float32 con las esquinas (x, y).
    """
    ## TODO: CODE HERE
    corners = np.empty((0, 2), np.float32)
    ## END TODO
    return corners


def track(prev, curr, pts, max_level, win_size):
    """Sigue los puntos pts de la imagen prev a la imagen curr con cv2.calcOpticalFlowPyrLK.

    Args:
        prev, curr: las dos imágenes, en gris.
        pts: array de N x 2 float32 con los puntos en prev.
        max_level: último nivel de la pirámide; 0 es Lucas-Kanade sin pirámide.
        win_size: lado de la ventana W de Lucas-Kanade, en píxeles.

    Returns:
        (new_pts, ok): new_pts es un array de N x 2 float32 con los puntos en curr, y ok un array de N
        bool, False en los puntos que Lucas-Kanade no ha podido seguir (status 0).
    """
    criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 30, 0.01)
    ## TODO: CODE HERE
    new_pts, ok = pts.copy(), np.zeros(len(pts), bool)
    ## END TODO
    return new_pts.reshape(-1, 2), ok


def forward_backward(prev, curr, pts, new_pts, max_level, win_size):
    """Comprobación hacia atrás: sigue new_pts de curr a prev y mira si vuelven a pts.

    Si Lucas-Kanade se ha equivocado hacia delante, lo normal es que hacia atrás no vuelva al punto de
    partida: es una forma barata de detectar los seguimientos erróneos (los outliers).

    PISTA: es la misma función track, con las imágenes cambiadas de orden.

    Args:
        prev, curr: las dos imágenes, en gris.
        pts: array de N x 2 float32 con los puntos en prev.
        new_pts: array de N x 2 float32 con los mismos puntos seguidos en curr.
        max_level: último nivel de la pirámide.
        win_size: lado de la ventana W de Lucas-Kanade, en píxeles.

    Returns:
        Array de N bool, True en los puntos que vuelven a menos de FB_MAX_ERROR píxeles de pts.
    """
    ## TODO: CODE HERE
    ok = np.zeros(len(pts), bool)
    ## END TODO
    return ok


def nothing(value):
    pass


## Línea de comandos: el vídeo de data/ o la webcam
use_camera = len(sys.argv) > 1 and sys.argv[1] == "cam"
cap = cv2.VideoCapture(0 if use_camera else VIDEO)
ok, frame = cap.read()
if not ok:
    print(f"Cannot read {'the camera' if use_camera else VIDEO}")
    exit(1)
prev = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

cv2.namedWindow(WINDOW)
cv2.createTrackbar("maxLevel", WINDOW, 3, 5, nothing)
cv2.createTrackbar("step", WINDOW, 4, 10, nothing)
cv2.createTrackbar("winSize", WINDOW, 21, 51, nothing)
cv2.setTrackbarMin("winSize", WINDOW, 3)

trails = []
check = True
paused = False
lost_lk = lost_fb = 0
rejected = np.empty((0, 2), np.float32)

while True:
    max_level = cv2.getTrackbarPos("maxLevel", WINDOW)
    step = max(1, cv2.getTrackbarPos("step", WINDOW))
    win_size = cv2.getTrackbarPos("winSize", WINDOW)

    if not paused:
        for _ in range(step):
            ok, frame = cap.read()
            if not ok:
                break
        if not ok:
            if use_camera:
                break
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            ok, frame = cap.read()
            prev = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            trails = []
            continue
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        ## 2. Seguir los puntos con Lucas-Kanade y descartar los que fallan
        lost_lk = lost_fb = 0
        rejected = np.empty((0, 2), np.float32)
        if trails:
            pts = np.array([trail[-1] for trail in trails], np.float32)
            new_pts, ok_lk = track(prev, gray, pts, max_level, win_size)
            ok_all = ok_lk.copy()
            if check:
                ok_all = ok_all & forward_backward(prev, gray, pts, new_pts, max_level, win_size)
            lost_lk = int((~ok_lk).sum())
            lost_fb = int((ok_lk & ~ok_all).sum())
            rejected = pts[~ok_all]
            trails = [trail + [(float(x), float(y))] for trail, (x, y), good in zip(trails, new_pts, ok_all) if good]
            trails = [trail[-TRAIL:] for trail in trails]

        ## 3. Esquinas nuevas lejos de los puntos que ya hay.
        mask = np.full(gray.shape, 255, np.uint8)
        for trail in trails:
            x, y = trail[-1]
            cv2.circle(mask, (int(x), int(y)), 10, 0, -1)
        for x, y in detect_corners(gray, mask, MAX_CORNERS):
            trails.append([(float(x), float(y))])

        prev = gray

    ## 4. Dibujo: trayectorias en verde si el punto se mueve, en gris si está quieto, descartados en rojo
    vis = frame.copy()
    moving = 0
    for trail in trails:
        line = np.array(trail, np.int32)
        if np.linalg.norm(np.subtract(trail[-1], trail[0])) > MOVING:
            moving += 1
            cv2.polylines(vis, [line], False, (0, 255, 0), 1, cv2.LINE_AA)
            cv2.circle(vis, tuple(line[-1]), 3, (0, 255, 0), -1)
        else:
            cv2.circle(vis, tuple(line[-1]), 2, (128, 128, 128), -1)
    for x, y in rejected:
        cv2.circle(vis, (int(x), int(y)), 5, (0, 0, 255), 2)

    cv2.imshow(WINDOW, vis)
    cv2.setWindowTitle(WINDOW, f"{len(trails)} points ({moving} moving)   lost: LK {lost_lk}, "
                               f"forward-backward {lost_fb}   [check {'on' if check else 'off'}]")
    key = cv2.waitKey(30)
    if key == 27:
        break
    elif key == ord(" "):
        paused = not paused
    elif key == ord("f"):
        check = not check
        print(f"Forward-backward check: {'on' if check else 'off'}")
    elif key == ord("r"):
        trails = []                                 # en el siguiente fotograma se buscan todas de nuevo

cv2.destroyAllWindows()
