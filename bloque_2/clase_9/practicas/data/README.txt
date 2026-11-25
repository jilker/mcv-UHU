Cada carpeta contiene al menos una pareja img0_l, img0_r y calibration.yml

  calibration        : 13 pares de un tablero 9x6 (datos de OpenCV).
  middlebury         : 5 escenas ya rectificadas (Middlebury Stereo 2014, Scharstein et al.).
  middlebury_mod     : otras 5 escenas con las camaras giradas artificialmente (Middlebury Stereo 2014).
  oxford_vgg         : camaras muy convergentes o que avanzan, casi todos se rectifican mal (Oxford VGG Multi-view).
  strecha            : (castle-P30, Strecha et al. 2008, EPFL).

Datasets por practica:
  P1_calibracion_stereo : calibration
  P2_epipolar           : todos
  P3_densa              : todos (oxford rectifica mal)
  P4_dispersa           : todos

Ejemplos (python):
  python3 calibracion_stereo.py ../data/calibration 9 6 1
  python3 epipolar.py ../data/strecha/castle-p30_23_24
  python3 densa.py ../data/middlebury/adirondack
  python3 dispersa.py ../data/oxford_vgg/merton_c_03_01
