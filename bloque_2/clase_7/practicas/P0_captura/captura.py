import sys

import cv2

PROPERTIES = [
    ("brightness", cv2.CAP_PROP_BRIGHTNESS, 0, 255),
    ("contrast", cv2.CAP_PROP_CONTRAST, 0, 255),
    ("saturation", cv2.CAP_PROP_SATURATION, 0, 255),
    ("hue", cv2.CAP_PROP_HUE, -180, 180),
    ("gamma", cv2.CAP_PROP_GAMMA, 0, 500),
    ("gain", cv2.CAP_PROP_GAIN, 0, 255),
    ("sharpness", cv2.CAP_PROP_SHARPNESS, 0, 255),
    ("backlight", cv2.CAP_PROP_BACKLIGHT, 0, 10),
    ("auto white balance", cv2.CAP_PROP_AUTO_WB, 0, 1),
    ("wb temperature", cv2.CAP_PROP_WB_TEMPERATURE, 2000, 10000),
    ("auto exposure", cv2.CAP_PROP_AUTO_EXPOSURE, 0, 3),
    ("exposure", cv2.CAP_PROP_EXPOSURE, 0, 5000),
    ("autofocus", cv2.CAP_PROP_AUTOFOCUS, 0, 1),
    ("focus", cv2.CAP_PROP_FOCUS, 0, 255),
    ("zoom", cv2.CAP_PROP_ZOOM, 0, 500),
]


def nothing(value):
    pass


cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("Cannot open camera 0", file=sys.stderr)
    sys.exit(-1)

cv2.namedWindow("Controls")
values = []
for name, prop, min_value, max_value in PROPERTIES:
    value = int(cap.get(prop))
    cv2.createTrackbar(name, "Controls", 0, max_value, nothing)
    cv2.setTrackbarMin(name, "Controls", min_value)
    cv2.setTrackbarPos(name, "Controls", value)
    values.append(value)

captures = 0
while True:
    for i, (name, prop, min_value, max_value) in enumerate(PROPERTIES):
        value = cv2.getTrackbarPos(name, "Controls")
        if value != values[i]:
            cap.set(prop, value)
            values[i] = value

    ok, frame = cap.read()
    if not ok:
        print("Cannot read from camera 0", file=sys.stderr)
        sys.exit(-1)
    vis = frame.copy()
    cv2.putText(vis, f"[c] capture ({captures})   [ESC] exit", (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    cv2.imshow("Camera", vis)

    key = cv2.waitKey(1)
    if key == 27:
        break
    if key == ord("c"):
        file = f"capture_{captures:03d}.png"
        captures += 1
        cv2.imwrite(file, frame)
        print(f"Saved {file} ({frame.shape[1]}x{frame.shape[0]})")
cv2.destroyAllWindows()
