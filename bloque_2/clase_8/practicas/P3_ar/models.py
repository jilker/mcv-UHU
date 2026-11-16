"""Modelos 3D en alambre para práctica de realidad aumentada.

    model = get_model(model_id, size)   # size: lado de la base, en metros (None si el id no existe)
    model.vertices  # array N x 3 float32, con el origen en el centro de la base y el eje Z hacia arriba
    model.edges     # array M x 2 int32: los índices de los dos vértices de cada arista
    model.colors    # lista de M colores BGR: model.colors[i] es el color de la arista model.edges[i]

NO HACE FALTA MODIFICAR NADA EN ESTE FICHERO
"""

import math
import time

import numpy as np

_T0 = time.monotonic()


class Model:
    def __init__(self, vertices, edges, colors):
        self.vertices = vertices
        self.edges = edges
        self.colors = colors


MODEL_NAMES = ["castillo", "arbol", "esfera", "ADN", "nudo", "grua", "robot"]


def get_model(model_id, size):
    return get_model_at(model_id, size, time.monotonic() - _T0)


def get_model_at(model_id, size, t):
    builders = {0: _castle, 1: _tree, 2: _geodesic, 3: _dna, 4: _knot, 5: _crane, 6: _robot}
    if model_id not in builders:
        return None
    b = _Builder()
    builders[model_id](b, t)
    vertices = np.array(b.vertices, np.float32).reshape(-1, 3) * size
    edges = np.array(b.edges, np.int32).reshape(-1, 2)
    return Model(vertices, edges, list(b.edge_colors))


_STONE = (140, 180, 210)
_ROOF = (50, 50, 200)
_WOOD = (40, 90, 150)
_DARK = (70, 70, 70)
_GRAY = (170, 170, 170)
_WHITE = (230, 230, 230)
_YELLOW = (0, 210, 255)
_ORANGE = (0, 150, 255)
_GREEN = (60, 200, 60)
_BLUE = (255, 140, 30)
_RED = (60, 60, 230)
_CYAN = (255, 200, 40)
_POT = (60, 110, 200)
_BEAD = (0, 90, 230)
_BASE = (230, 60, 150)


def _add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _mul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _norm(a):
    n = math.sqrt(_dot(a, a))
    return (a[0] / n, a[1] / n, a[2] / n)


def _perp(d):
    ref = (0.0, 0.0, 1.0) if abs(d[2]) < 0.9 else (1.0, 0.0, 0.0)
    u = _norm(_cross(d, ref))
    return u, _cross(d, u)


def _rot_z(p, a):
    c, s = math.cos(a), math.sin(a)
    return (c * p[0] - s * p[1], s * p[0] + c * p[1], p[2])


class _Builder:
    def __init__(self):
        self.vertices = []
        self.edges = []
        self.edge_colors = []
        self.color = _WHITE

    def v(self, p):
        self.vertices.append((float(p[0]), float(p[1]), float(p[2])))
        return len(self.vertices) - 1

    def e(self, a, b):
        self.edges.append((a, b))
        self.edge_colors.append(self.color)

    def line(self, p0, p1):
        self.e(self.v(p0), self.v(p1))

    def polyline(self, points, closed=False):
        idx = [self.v(p) for p in points]
        for i in range(len(idx) - 1):
            self.e(idx[i], idx[i + 1])
        if closed:
            self.e(idx[-1], idx[0])
        return idx

    def circle(self, c, u, w, r, n):
        return self.polyline([_add(c, _add(_mul(u, r * math.cos(2 * math.pi * i / n)),
                                           _mul(w, r * math.sin(2 * math.pi * i / n)))) for i in range(n)], True)

    def box(self, c, size):
        hx, hy, hz = size[0] / 2, size[1] / 2, size[2] / 2
        self.box_corners([_add(c, (sx * hx, sy * hy, sz * hz))
                          for sz in (-1, 1) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))])

    def box_corners(self, p):
        idx = [self.v(q) for q in p]
        for i in range(4):
            self.e(idx[i], idx[(i + 1) % 4])
            self.e(idx[4 + i], idx[4 + (i + 1) % 4])
            self.e(idx[i], idx[4 + i])

    def beam(self, p0, p1, width, up=(0.0, 0.0, 1.0)):
        d = _norm(_sub(p1, p0))
        u = _cross(d, up)
        if _dot(u, u) < 1e-8:
            u, w = _perp(d)
        else:
            u = _norm(u)
            w = _cross(u, d)
        h = width / 2
        sq = [_add(_mul(u, sx * h), _mul(w, sy * h)) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        self.box_corners([_add(p0, q) for q in sq] + [_add(p1, q) for q in sq])

    def cylinder(self, p0, p1, r0, r1, n, sides=None):
        d = _norm(_sub(p1, p0))
        u, w = _perp(d)
        a = self.circle(p0, u, w, r0, n)
        b = self.circle(p1, u, w, r1, n)
        sides = n if sides is None else sides
        for i in range(0, n, n // sides):
            self.e(a[i], b[i])

    def cone(self, base, apex, r, n):
        d = _norm(_sub(apex, base))
        u, w = _perp(d)
        ring = self.circle(base, u, w, r, n)
        top = self.v(apex)
        for i in ring:
            self.e(i, top)

    def lattice(self, bottom, top, levels):
        n = len(bottom)
        prev = None
        for k in range(levels + 1):
            s = k / levels
            ring = [self.v(_add(bottom[i], _mul(_sub(top[i], bottom[i]), s))) for i in range(n)]
            for i in range(n):
                self.e(ring[i], ring[(i + 1) % n])
            if prev is not None:
                for i in range(n):
                    self.e(prev[i], ring[i])
                    if k % 2:
                        self.e(prev[i], ring[(i + 1) % n])
                    else:
                        self.e(prev[(i + 1) % n], ring[i])
            prev = ring

    def transform(self, start, f):
        for i in range(start, len(self.vertices)):
            self.vertices[i] = f(self.vertices[i])


class _Random:

    def __init__(self, seed):
        self.state = seed & 0xFFFFFFFF

    def next(self):
        self.state = (self.state * 1664525 + 1013904223) & 0xFFFFFFFF
        return self.state / 4294967296.0


# id 0: castillo

def _castle(b, t):
    b.color = _STONE
    H = 0.42
    wall_h, wall_w = 0.22, 0.06
    corners = [(-H, -H), (H, -H), (H, H), (-H, H)]

    for i in range(4):
        x0, y0 = corners[i]
        x1, y1 = corners[(i + 1) % 4]
        d = _norm((x1 - x0, y1 - y0, 0))
        n = (d[1], -d[0], 0)
        r_tower = 0.11
        p0 = _add((x0, y0, 0), _mul(d, r_tower))
        p1 = _sub((x1, y1, 0), _mul(d, r_tower))
        q = [_add(p0, _mul(n, wall_w / 2)), _add(p1, _mul(n, wall_w / 2)),
             _sub(p1, _mul(n, wall_w / 2)), _sub(p0, _mul(n, wall_w / 2))]
        b.box_corners(q + [_add(p, (0, 0, wall_h)) for p in q])
        length = math.dist(p0[:2], p1[:2])
        merlons = 5
        step = length / merlons
        for k in range(merlons):
            c = _add(_add(p0, _mul(d, step * (k + 0.5))), _mul(n, wall_w / 4))
            c = (c[0], c[1], wall_h + 0.025)
            sx = abs(d[0]) * step * 0.55 + abs(n[0]) * wall_w / 2
            sy = abs(d[1]) * step * 0.55 + abs(n[1]) * wall_w / 2
            b.box(c, (sx, sy, 0.05))

    b.color = _WOOD
    yg = -H - wall_w / 2
    arch = [(-0.07, yg, 0.0), (-0.07, yg, 0.1)]
    arch += [(-0.07 * math.cos(math.pi * k / 8), yg, 0.1 + 0.07 * math.sin(math.pi * k / 8)) for k in range(1, 8)]
    arch += [(0.07, yg, 0.1), (0.07, yg, 0.0)]
    b.polyline(arch)
    b.color = _DARK
    for k in range(-2, 3):
        b.line((0.025 * k, yg - 0.001, 0.0), (0.025 * k, yg - 0.001, 0.1 + 0.07 * math.cos(math.asin(0.025 * k / 0.07))))

    for x, y in corners:
        b.color = _STONE
        b.cylinder((x, y, 0), (x, y, 0.36), 0.11, 0.11, 16, 8)
        b.cylinder((x, y, 0.36), (x, y, 0.42), 0.135, 0.135, 16, 16)
        b.color = _ROOF
        b.cone((x, y, 0.42), (x, y, 0.68), 0.14, 16)
        b.color = _DARK
        for k in range(4):
            a = math.pi / 4 + k * math.pi / 2
            p = (x + 0.111 * math.cos(a), y + 0.111 * math.sin(a), 0.2)
            b.line(p, _add(p, (0, 0, 0.08)))

    b.color = _STONE
    K, kh = 0.15, 0.58
    b.box((0, 0, kh / 2), (2 * K, 2 * K, kh))
    for i in range(4):
        for k in range(3):
            s = -K + 2 * K * (k + 0.5) / 3
            pos = [(s, -K), (K, s), (-s, K), (-K, -s)][i]
            b.box((pos[0], pos[1], kh + 0.025), (0.05, 0.05, 0.05))
    b.color = _DARK
    for z in (0.18, 0.38):
        b.polyline([(-0.03, -K, z), (0.03, -K, z), (0.03, -K, z + 0.08), (-0.03, -K, z + 0.08)], True)
        b.polyline([(K, -0.03, z), (K, 0.03, z), (K, 0.03, z + 0.08), (K, -0.03, z + 0.08)], True)

    b.color = _WHITE
    pole_top = (0, 0, kh + 0.35)
    b.line((0, 0, kh), pole_top)
    b.color = _YELLOW
    nu, nv = 6, 3
    flag = [[(0.2 * u / nu,
              0.035 * (u / nu) * math.sin(5 * t - 3.0 * u / nu),
              pole_top[2] - 0.12 * v / nv) for v in range(nv + 1)] for u in range(nu + 1)]
    idx = [[b.v(p) for p in col] for col in flag]
    for u in range(nu + 1):
        for v in range(nv + 1):
            if u < nu:
                b.e(idx[u][v], idx[u + 1][v])
            if v < nv:
                b.e(idx[u][v], idx[u][v + 1])


# id 1: árbol

def _tree(b, t):
    golden = math.pi * (3 - math.sqrt(5))

    def branch(p, d, u, length, depth, idx_p):
        sway = 0.06 * (6 - depth) * math.sin(1.7 * t + 0.9 * depth)
        d = _norm(_add(d, (sway, 0.5 * sway, 0)))
        q = _add(p, _mul(d, length))
        idx_q = b.v(q)
        b.color = _GREEN if depth <= 1 else _WOOD
        b.e(idx_p, idx_q)
        if depth == 0:
            return
        u = _norm(_sub(u, _mul(d, _dot(u, d))))
        w = _cross(d, u)
        spread = math.radians(32)
        for k in range(3):
            phi = 2 * math.pi * k / 3 + golden * depth
            side = _add(_mul(u, math.cos(phi)), _mul(w, math.sin(phi)))
            child = _norm(_add(_mul(d, math.cos(spread)), _mul(side, math.sin(spread))))
            branch(q, child, u, length * 0.7, depth - 1, idx_q)
        if depth >= 5:
            branch(q, d, u, length * 0.55, depth - 1, idx_q)

    b.color = _POT
    b.cylinder((0, 0, 0), (0, 0, 0.12), 0.16, 0.22, 16, 8)
    root = b.v((0, 0, 0))
    branch((0, 0, 0), (0, 0, 1), (1, 0, 0), 0.42, 5, root)


# id 2: esfera

def _geodesic(b, t):
    phi = (1 + math.sqrt(5)) / 2
    verts = [_norm(p) for p in [(-1, phi, 0), (1, phi, 0), (-1, -phi, 0), (1, -phi, 0), (0, -1, phi), (0, 1, phi),
                                (0, -1, -phi), (0, 1, -phi), (phi, 0, -1), (phi, 0, 1), (-phi, 0, -1), (-phi, 0, 1)]]
    faces = [(0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11), (1, 5, 9), (5, 11, 4), (11, 10, 2),
             (10, 7, 6), (7, 1, 8), (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9), (4, 9, 5), (2, 4, 11),
             (6, 2, 10), (8, 6, 7), (9, 8, 1)]
    for _ in range(2):
        cache = {}

        def mid(i, j):
            key = (min(i, j), max(i, j))
            if key not in cache:
                verts.append(_norm(_mul(_add(verts[i], verts[j]), 0.5)))
                cache[key] = len(verts) - 1
            return cache[key]

        new_faces = []
        for a, c, d in faces:
            ab, cd, da = mid(a, c), mid(c, d), mid(d, a)
            new_faces += [(a, ab, da), (c, cd, ab), (d, da, cd), (ab, cd, da)]
        faces = new_faces

    b.color = _BASE
    b.cylinder((0, 0, 0), (0, 0, 0.1), 0.25, 0.15, 16, 8)
    r, zc = 0.42, 0.5
    ang = 0.4 * t
    start = len(b.vertices)
    for p in verts:
        q = _rot_z(p, ang)
        b.v((r * q[0], r * q[1], zc + r * q[2]))
    b.color = _CYAN
    edges = set()
    for f in faces:
        for i in range(3):
            a, c = f[i], f[(i + 1) % 3]
            edges.add((min(a, c), max(a, c)))
    for a, c in sorted(edges):
        b.e(start + a, start + c)


# id 3: ADN

def _dna(b, t):
    n, turns, height, r = 120, 2.5, 1.3, 0.22
    groove = 2 * math.pi * 0.38
    ang = 0.8 * t
    s1, s2 = [], []
    for i in range(n + 1):
        a = ang + 2 * math.pi * turns * i / n
        z = 0.08 + height * i / n
        s1.append((r * math.cos(a), r * math.sin(a), z))
        s2.append((r * math.cos(a + groove), r * math.sin(a + groove), z))
    b.color = _BASE
    b.cylinder((0, 0, 0), (0, 0, 0.06), 0.3, 0.3, 24, 8)
    b.color = _BLUE
    i1 = b.polyline(s1)
    b.color = _RED
    i2 = b.polyline(s2)
    for i in range(2, n, 4):
        m = b.v(_mul(_add(s1[i], s2[i]), 0.5))
        b.color = _GREEN
        b.e(i1[i], m)
        b.color = _YELLOW
        b.e(m, i2[i])


# id 4: nudo

def _knot(b, t):
    p, q = 2, 3
    R, a, tube = 0.3, 0.13, 0.055
    n, sides = 120, 6
    ang = 0.5 * t

    def center(s):
        return ((R + a * math.cos(q * s)) * math.cos(p * s), (R + a * math.cos(q * s)) * math.sin(p * s),
                -a * math.sin(q * s))

    def to_world(v):
        v = (v[0], -v[2], v[1])
        v = _rot_z(v, ang)
        return (v[0], v[1], v[2] + 0.62)

    b.color = _BASE
    b.cylinder((0, 0, 0), (0, 0, 0.05), 0.3, 0.3, 24, 8)
    b.line((0, 0, 0.05), (0, 0, 0.2))
    b.color = _ORANGE
    rings = []
    for i in range(n):
        s = 2 * math.pi * i / n
        c = center(s)
        tan = _norm(_sub(center(s + 1e-3), center(s - 1e-3)))
        core = (R * math.cos(p * s), R * math.sin(p * s), 0)
        nrm = _sub(c, core)
        nrm = _norm(_sub(nrm, _mul(tan, _dot(nrm, tan))))
        bin_ = _cross(tan, nrm)
        ring = []
        for k in range(sides):
            th = 2 * math.pi * k / sides
            ring.append(b.v(to_world(_add(c, _mul(_add(_mul(nrm, math.cos(th)), _mul(bin_, math.sin(th))), tube)))))
        for k in range(sides):
            b.e(ring[k], ring[(k + 1) % sides])
        rings.append(ring)
    for i in range(n):
        for k in range(sides):
            b.e(rings[i][k], rings[(i + 1) % n][k])


# id 5: grúa

def _crane(b, t):
    s = 0.07
    base_h, mast_h = 0.08, 1.15
    b.color = _BASE
    b.box((0, 0, base_h / 2), (0.5, 0.5, base_h))
    b.color = _YELLOW
    sq = [(-s, -s), (s, -s), (s, s), (-s, s)]
    b.lattice([(x, y, base_h) for x, y in sq], [(x, y, mast_h) for x, y in sq], 8)

    start = len(b.vertices)
    top = mast_h
    z0 = top + 0.08
    jib, cj = 1.1, 0.42
    x = 0.6 + 0.3 * math.sin(0.6 * t)
    hook_z = 0.5 + 0.2 * math.sin(0.45 * t + 1)
    hook_eye = (x, 0, hook_z)
    load_top = hook_z - 0.12
    b.color = _WOOD
    b.box((x, 0, load_top - 0.07), (0.16, 0.16, 0.14))
    b.color = _WHITE
    for dx, dy in ((-0.08, -0.08), (0.08, -0.08), (0.08, 0.08), (-0.08, 0.08)):
        b.line(hook_eye, (x + dx, dy, load_top))
    b.polyline([(x, 0, hook_z)] + [(x + 0.03 * math.sin(math.pi * k / 6), 0, hook_z - 0.04 + 0.03 * math.cos(
        math.pi * k / 6)) for k in range(0, 10)])
    b.line((x, -0.03, z0 - 0.03), (x, -0.01, hook_z))
    b.line((x, 0.03, z0 - 0.03), (x, 0.01, hook_z))
    b.color = _DARK
    b.box((0, 0, top + 0.04), (2 * s + 0.02, 2 * s + 0.02, 0.08))
    b.color = _BLUE
    b.box((s + 0.05, -0.1, z0 - 0.06), (0.1, 0.08, 0.1))
    b.color = _GRAY
    b.box((-cj + 0.07, 0, z0 - 0.06), (0.12, 0.14, 0.12))
    b.color = _DARK
    b.box((x, 0, z0 - 0.015), (0.08, 0.12, 0.03))
    b.color = _YELLOW
    b.lattice([(s, -0.05, z0), (s, 0.05, z0), (s, 0, z0 + 0.09)],
              [(jib, -0.05, z0), (jib, 0.05, z0), (jib, 0, z0 + 0.09)], 12)
    b.lattice([(-s, -0.06, z0), (-s, 0.06, z0), (-s, 0.06, z0 + 0.03), (-s, -0.06, z0 + 0.03)],
              [(-cj, -0.06, z0), (-cj, 0.06, z0), (-cj, 0.06, z0 + 0.03), (-cj, -0.06, z0 + 0.03)], 4)
    b.color = _WHITE
    apex = (0, 0, z0 + 0.35)
    for x_, y_ in sq:
        b.line((x_, y_, z0), apex)
    b.line(apex, (0.75, 0, z0 + 0.09))
    b.line(apex, (-cj, 0.06, z0 + 0.03))
    b.line(apex, (-cj, -0.06, z0 + 0.03))
    b.transform(start, lambda v: _rot_z(v, 0.25 * t))


# id 6: robot

def _robot(b, t):
    seam_y, plate_z = -0.18, 0.04
    b.color = _BASE
    b.box((0, seam_y - 0.17, plate_z / 2), (0.9, 0.32, plate_z))
    b.box((0, seam_y + 0.17, plate_z / 2), (0.9, 0.32, plate_z))
    x_tip = 0.35 * math.sin(0.6 * t)
    bead = [(xx, seam_y, plate_z + 0.008 * (1 + math.cos(40 * xx))) for xx in
            np.linspace(-0.38, x_tip, max(2, int((x_tip + 0.38) * 60)))]
    b.color = _BEAD
    b.polyline(bead)

    base = (0.0, 0.32, 0.0)
    shoulder_h, l1, l2, torch = 0.28, 0.45, 0.4, 0.2
    tip = (x_tip, seam_y, plate_z + 0.01)
    tool_dir = _norm((0, 0.35, 1))
    wrist = _add(tip, _mul(tool_dir, torch))

    yaw = math.atan2(wrist[1] - base[1], wrist[0] - base[0])
    rdir = (math.cos(yaw), math.sin(yaw), 0)
    shoulder = (base[0], base[1], shoulder_h)
    dr = math.hypot(wrist[0] - base[0], wrist[1] - base[1])
    dz = wrist[2] - shoulder_h
    dist = math.hypot(dr, dz)
    cos_e = (dist * dist - l1 * l1 - l2 * l2) / (2 * l1 * l2)
    elbow_ang = math.acos(max(-1.0, min(1.0, cos_e)))
    sh_ang = math.atan2(dz, dr) + math.atan2(l2 * math.sin(elbow_ang), l1 + l2 * math.cos(elbow_ang))
    elbow = _add(shoulder, _add(_mul(rdir, l1 * math.cos(sh_ang)), (0, 0, l1 * math.sin(sh_ang))))

    b.color = _BASE
    b.cylinder(base, _add(base, (0, 0, 0.06)), 0.16, 0.16, 20, 10)
    b.color = _BLUE
    start = len(b.vertices)
    b.cylinder(_add(base, (0, 0, 0.06)), _add(base, (0, 0, 0.2)), 0.12, 0.1, 12, 6)
    b.transform(start, lambda v: _add(base, _rot_z(_sub(v, base), yaw)))
    b.beam(_add(base, (0, 0, 0.2)), shoulder, 0.08)
    axis = (-rdir[1], rdir[0], 0)
    b.color = _DARK
    for j, rad in ((shoulder, 0.07), (elbow, 0.06), (wrist, 0.04)):
        b.cylinder(_sub(j, _mul(axis, 0.06)), _add(j, _mul(axis, 0.06)), rad, rad, 12, 6)
    b.color = _BLUE
    b.beam(shoulder, elbow, 0.08, axis)
    b.beam(elbow, wrist, 0.06, axis)
    nozzle = _add(tip, _mul(tool_dir, 0.06))
    b.color = _WHITE
    b.cylinder(wrist, nozzle, 0.03, 0.025, 8, 4)
    b.cylinder(nozzle, tip, 0.022, 0.012, 8, 4)
    b.color = _DARK
    b.polyline([_add(wrist, (0, 0, 0.02)), _add(elbow, (0, 0, 0.12)), _add(shoulder, (0, 0.05, 0.12)),
                _add(base, (0, 0.12, 0.12))])
    b.color = _ORANGE
    rnd = _Random(int(t * 25))
    for _ in range(14):
        a = 2 * math.pi * rnd.next()
        up = 0.2 + 0.8 * rnd.next()
        length = 0.04 + 0.12 * rnd.next()
        d = _norm((math.cos(a) * (1 - up), math.sin(a) * (1 - up), up))
        p0 = _add(tip, _mul(d, 0.02 + 0.05 * rnd.next()))
        b.line(p0, _add(p0, _mul(d, length)))
    b.transform(0, lambda v: _rot_z(v, math.pi))
