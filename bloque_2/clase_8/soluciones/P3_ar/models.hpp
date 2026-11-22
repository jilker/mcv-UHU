/**
 * Modelos 3D en alambre para práctica de realidad aumentada.
 *
 *     Model model = getModel(modelId, size);   // size: lado de la base, en metros
 *     model.vertices   // vértices, con el origen en el centro de la base y el eje Z hacia arriba
 *     model.edges      // pares (a, b) con los índices de los dos vértices de cada arista
 *     model.colors     // color BGR de cada arista: model.colors[i] es el color de model.edges[i]
 *
 * NO HACE FALTA MODIFICAR NADA EN ESTE FICHERO
 */
#pragma once

#include <opencv2/core.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

struct Model {
    std::vector<cv::Point3f> vertices;
    std::vector<std::pair<int, int>> edges;
    std::vector<cv::Scalar> colors;

    bool empty() const { return vertices.empty(); }
};

const std::vector<std::string> MODEL_NAMES = {"castillo", "arbol", "esfera", "ADN", "nudo", "grua", "robot"};

Model getModelAt(int modelId, float size, double t);

inline Model getModel(int modelId, float size) {
    static const auto t0 = std::chrono::steady_clock::now();
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return getModelAt(modelId, size, t);
}

namespace models_detail {

using V = cv::Vec3d;
const double PI = 3.14159265358979323846;

const cv::Scalar STONE(140, 180, 210);
const cv::Scalar ROOF(50, 50, 200);
const cv::Scalar WOOD(40, 90, 150);
const cv::Scalar DARK(70, 70, 70);
const cv::Scalar GRAY(170, 170, 170);
const cv::Scalar WHITE(230, 230, 230);
const cv::Scalar YELLOW(0, 210, 255);
const cv::Scalar ORANGE(0, 150, 255);
const cv::Scalar GREEN(60, 200, 60);
const cv::Scalar BLUE(255, 140, 30);
const cv::Scalar RED(60, 60, 230);
const cv::Scalar CYAN(255, 200, 40);
const cv::Scalar POT(60, 110, 200);
const cv::Scalar BEAD(0, 90, 230);
const cv::Scalar BASE(230, 60, 150);

inline V norm(const V& a) { return a / std::sqrt(a.dot(a)); }

inline std::pair<V, V> perp(const V& d) {
    V ref = std::abs(d[2]) < 0.9 ? V(0, 0, 1) : V(1, 0, 0);
    V u = norm(d.cross(ref));
    return {u, d.cross(u)};
}

inline V rotZ(const V& p, double a) {
    double c = std::cos(a), s = std::sin(a);
    return V(c * p[0] - s * p[1], s * p[0] + c * p[1], p[2]);
}

struct Builder {
    std::vector<V> vertices;
    std::vector<std::pair<int, int>> edges;
    std::vector<cv::Scalar> edgeColors;
    cv::Scalar color = WHITE;

    int v(const V& p) {
        vertices.push_back(p);
        return (int)vertices.size() - 1;
    }

    void e(int a, int b) {
        edges.emplace_back(a, b);
        edgeColors.push_back(color);
    }

    void line(const V& p0, const V& p1) {
        int a = v(p0);
        int b = v(p1);
        e(a, b);
    }

    std::vector<int> polyline(const std::vector<V>& points, bool closed = false) {
        std::vector<int> idx;
        for (const V& p : points) {
            idx.push_back(v(p));
        }
        for (size_t i = 0; i + 1 < idx.size(); i++) {
            e(idx[i], idx[i + 1]);
        }
        if (closed) {
            e(idx.back(), idx.front());
        }
        return idx;
    }

    std::vector<int> circle(const V& c, const V& u, const V& w, double r, int n) {
        std::vector<V> points;
        for (int i = 0; i < n; i++) {
            double a = 2 * PI * i / n;
            points.push_back(c + u * (r * std::cos(a)) + w * (r * std::sin(a)));
        }
        return polyline(points, true);
    }

    void boxCorners(const std::vector<V>& p) {
        std::vector<int> idx;
        for (const V& q : p) {
            idx.push_back(v(q));
        }
        for (int i = 0; i < 4; i++) {
            e(idx[i], idx[(i + 1) % 4]);
            e(idx[4 + i], idx[4 + (i + 1) % 4]);
            e(idx[i], idx[4 + i]);
        }
    }

    void box(const V& c, const V& size) {
        V h = size * 0.5;
        const int sxy[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
        std::vector<V> p;
        for (int sz : {-1, 1}) {
            for (auto& s : sxy) {
                p.push_back(c + V(s[0] * h[0], s[1] * h[1], sz * h[2]));
            }
        }
        boxCorners(p);
    }

    void beam(const V& p0, const V& p1, double width, const V& up = V(0, 0, 1)) {
        V d = norm(p1 - p0);
        V u = d.cross(up), w;
        if (u.dot(u) < 1e-8) {
            std::tie(u, w) = perp(d);
        } else {
            u = norm(u);
            w = u.cross(d);
        }
        double h = width / 2;
        const int sxy[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
        std::vector<V> p;
        for (const V& base : {p0, p1}) {
            for (auto& s : sxy) {
                p.push_back(base + u * (s[0] * h) + w * (s[1] * h));
            }
        }
        boxCorners(p);
    }

    void cylinder(const V& p0, const V& p1, double r0, double r1, int n, int sides = 0) {
        auto [u, w] = perp(norm(p1 - p0));
        std::vector<int> a = circle(p0, u, w, r0, n);
        std::vector<int> b = circle(p1, u, w, r1, n);
        if (sides == 0) {
            sides = n;
        }
        for (int i = 0; i < n; i += n / sides) {
            e(a[i], b[i]);
        }
    }

    void cone(const V& base, const V& apex, double r, int n) {
        auto [u, w] = perp(norm(apex - base));
        std::vector<int> ring = circle(base, u, w, r, n);
        int top = v(apex);
        for (int i : ring) {
            e(i, top);
        }
    }

    void lattice(const std::vector<V>& bottom, const std::vector<V>& top, int levels) {
        int n = (int)bottom.size();
        std::vector<int> prev;
        for (int k = 0; k <= levels; k++) {
            double s = (double)k / levels;
            std::vector<int> ring;
            for (int i = 0; i < n; i++) {
                ring.push_back(v(bottom[i] + (top[i] - bottom[i]) * s));
            }
            for (int i = 0; i < n; i++) {
                e(ring[i], ring[(i + 1) % n]);
            }
            if (!prev.empty()) {
                for (int i = 0; i < n; i++) {
                    e(prev[i], ring[i]);
                    if (k % 2) {
                        e(prev[i], ring[(i + 1) % n]);
                    } else {
                        e(prev[(i + 1) % n], ring[i]);
                    }
                }
            }
            prev = ring;
        }
    }

    void transform(size_t start, const std::function<V(const V&)>& f) {
        for (size_t i = start; i < vertices.size(); i++) {
            vertices[i] = f(vertices[i]);
        }
    }
};

struct Random {
    uint32_t state;
    explicit Random(uint32_t seed) : state(seed) {}
    double next() {
        state = state * 1664525u + 1013904223u;
        return state / 4294967296.0;
    }
};

// id 0: castillo

inline void castle(Builder& b, double t) {
    b.color = STONE;
    const double H = 0.42;
    const double wallH = 0.22, wallW = 0.06;
    const double corners[4][2] = {{-H, -H}, {H, -H}, {H, H}, {-H, H}};

    for (int i = 0; i < 4; i++) {
        double x0 = corners[i][0], y0 = corners[i][1];
        double x1 = corners[(i + 1) % 4][0], y1 = corners[(i + 1) % 4][1];
        V d = norm(V(x1 - x0, y1 - y0, 0));
        V n(d[1], -d[0], 0);
        double rTower = 0.11;
        V p0 = V(x0, y0, 0) + d * rTower;
        V p1 = V(x1, y1, 0) - d * rTower;
        std::vector<V> q = {p0 + n * (wallW / 2), p1 + n * (wallW / 2), p1 - n * (wallW / 2), p0 - n * (wallW / 2)};
        std::vector<V> qq = q;
        for (const V& p : q) {
            qq.push_back(p + V(0, 0, wallH));
        }
        b.boxCorners(qq);
        double length = std::hypot(p1[0] - p0[0], p1[1] - p0[1]);
        int merlons = 5;
        double step = length / merlons;
        for (int k = 0; k < merlons; k++) {
            V c = p0 + d * (step * (k + 0.5)) + n * (wallW / 4);
            c[2] = wallH + 0.025;
            double sx = std::abs(d[0]) * step * 0.55 + std::abs(n[0]) * wallW / 2;
            double sy = std::abs(d[1]) * step * 0.55 + std::abs(n[1]) * wallW / 2;
            b.box(c, V(sx, sy, 0.05));
        }
    }

    b.color = WOOD;
    double yg = -H - wallW / 2;
    std::vector<V> arch = {V(-0.07, yg, 0.0), V(-0.07, yg, 0.1)};
    for (int k = 1; k < 8; k++) {
        arch.push_back(V(-0.07 * std::cos(PI * k / 8), yg, 0.1 + 0.07 * std::sin(PI * k / 8)));
    }
    arch.push_back(V(0.07, yg, 0.1));
    arch.push_back(V(0.07, yg, 0.0));
    b.polyline(arch);
    b.color = DARK;
    for (int k = -2; k <= 2; k++) {
        b.line(V(0.025 * k, yg - 0.001, 0.0),
               V(0.025 * k, yg - 0.001, 0.1 + 0.07 * std::cos(std::asin(0.025 * k / 0.07))));
    }

    for (auto& c : corners) {
        double x = c[0], y = c[1];
        b.color = STONE;
        b.cylinder(V(x, y, 0), V(x, y, 0.36), 0.11, 0.11, 16, 8);
        b.cylinder(V(x, y, 0.36), V(x, y, 0.42), 0.135, 0.135, 16, 16);
        b.color = ROOF;
        b.cone(V(x, y, 0.42), V(x, y, 0.68), 0.14, 16);
        b.color = DARK;
        for (int k = 0; k < 4; k++) {
            double a = PI / 4 + k * PI / 2;
            V p(x + 0.111 * std::cos(a), y + 0.111 * std::sin(a), 0.2);
            b.line(p, p + V(0, 0, 0.08));
        }
    }

    b.color = STONE;
    const double K = 0.15, kh = 0.58;
    b.box(V(0, 0, kh / 2), V(2 * K, 2 * K, kh));
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 3; k++) {
            double s = -K + 2 * K * (k + 0.5) / 3;
            const double pos[4][2] = {{s, -K}, {K, s}, {-s, K}, {-K, -s}};
            b.box(V(pos[i][0], pos[i][1], kh + 0.025), V(0.05, 0.05, 0.05));
        }
    }
    b.color = DARK;
    for (double z : {0.18, 0.38}) {
        b.polyline({V(-0.03, -K, z), V(0.03, -K, z), V(0.03, -K, z + 0.08), V(-0.03, -K, z + 0.08)}, true);
        b.polyline({V(K, -0.03, z), V(K, 0.03, z), V(K, 0.03, z + 0.08), V(K, -0.03, z + 0.08)}, true);
    }

    b.color = WHITE;
    V poleTop(0, 0, kh + 0.35);
    b.line(V(0, 0, kh), poleTop);
    b.color = YELLOW;
    const int nu = 6, nv = 3;
    int idx[nu + 1][nv + 1];
    for (int u = 0; u <= nu; u++) {
        for (int v = 0; v <= nv; v++) {
            idx[u][v] = b.v(V(0.2 * u / nu, 0.035 * ((double)u / nu) * std::sin(5 * t - 3.0 * u / nu),
                              poleTop[2] - 0.12 * v / nv));
        }
    }
    for (int u = 0; u <= nu; u++) {
        for (int v = 0; v <= nv; v++) {
            if (u < nu) {
                b.e(idx[u][v], idx[u + 1][v]);
            }
            if (v < nv) {
                b.e(idx[u][v], idx[u][v + 1]);
            }
        }
    }
}

// id 1: árbol

inline void treeBranch(Builder& b, double t, V p, V d, V u, double length, int depth, int idxP) {
    const double golden = PI * (3 - std::sqrt(5.0));
    double sway = 0.06 * (6 - depth) * std::sin(1.7 * t + 0.9 * depth);
    d = norm(d + V(sway, 0.5 * sway, 0));
    V q = p + d * length;
    int idxQ = b.v(q);
    b.color = depth <= 1 ? GREEN : WOOD;
    b.e(idxP, idxQ);
    if (depth == 0) {
        return;
    }
    u = norm(u - d * u.dot(d));
    V w = d.cross(u);
    double spread = 32 * PI / 180;
    for (int k = 0; k < 3; k++) {
        double phi = 2 * PI * k / 3 + golden * depth;
        V side = u * std::cos(phi) + w * std::sin(phi);
        V child = norm(d * std::cos(spread) + side * std::sin(spread));
        treeBranch(b, t, q, child, u, length * 0.7, depth - 1, idxQ);
    }
    if (depth >= 5) {
        treeBranch(b, t, q, d, u, length * 0.55, depth - 1, idxQ);
    }
}

inline void tree(Builder& b, double t) {
    b.color = POT;
    b.cylinder(V(0, 0, 0), V(0, 0, 0.12), 0.16, 0.22, 16, 8);
    int root = b.v(V(0, 0, 0));
    treeBranch(b, t, V(0, 0, 0), V(0, 0, 1), V(1, 0, 0), 0.42, 5, root);
}

// id 2: esfera

inline void geodesic(Builder& b, double t) {
    const double phi = (1 + std::sqrt(5.0)) / 2;
    std::vector<V> verts = {V(-1, phi, 0), V(1, phi, 0), V(-1, -phi, 0), V(1, -phi, 0),
                            V(0, -1, phi), V(0, 1, phi), V(0, -1, -phi), V(0, 1, -phi),
                            V(phi, 0, -1), V(phi, 0, 1), V(-phi, 0, -1), V(-phi, 0, 1)};
    for (V& p : verts) {
        p = norm(p);
    }
    std::vector<cv::Vec3i> faces = {{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
                                    {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                                    {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
                                    {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};
    for (int it = 0; it < 2; it++) {
        std::map<std::pair<int, int>, int> cache;
        auto mid = [&](int i, int j) {
            std::pair<int, int> key(std::min(i, j), std::max(i, j));
            auto found = cache.find(key);
            if (found != cache.end()) {
                return found->second;
            }
            verts.push_back(norm((verts[i] + verts[j]) * 0.5));
            cache[key] = (int)verts.size() - 1;
            return (int)verts.size() - 1;
        };
        std::vector<cv::Vec3i> newFaces;
        for (const cv::Vec3i& f : faces) {
            int a = f[0], c = f[1], d = f[2];
            int ab = mid(a, c), cd = mid(c, d), da = mid(d, a);
            newFaces.push_back({a, ab, da});
            newFaces.push_back({c, cd, ab});
            newFaces.push_back({d, da, cd});
            newFaces.push_back({ab, cd, da});
        }
        faces = newFaces;
    }

    b.color = BASE;
    b.cylinder(V(0, 0, 0), V(0, 0, 0.1), 0.25, 0.15, 16, 8);
    const double r = 0.42, zc = 0.5;
    double ang = 0.4 * t;
    int start = (int)b.vertices.size();
    for (const V& p : verts) {
        V q = rotZ(p, ang);
        b.v(V(r * q[0], r * q[1], zc + r * q[2]));
    }
    b.color = CYAN;
    std::set<std::pair<int, int>> edges;
    for (const cv::Vec3i& f : faces) {
        for (int i = 0; i < 3; i++) {
            int a = f[i], c = f[(i + 1) % 3];
            edges.insert({std::min(a, c), std::max(a, c)});
        }
    }
    for (auto [a, c] : edges) {
        b.e(start + a, start + c);
    }
}

// id 3: ADN

inline void dna(Builder& b, double t) {
    const int n = 120;
    const double turns = 2.5, height = 1.3, r = 0.22;
    const double groove = 2 * PI * 0.38;
    double ang = 0.8 * t;
    std::vector<V> s1, s2;
    for (int i = 0; i <= n; i++) {
        double a = ang + 2 * PI * turns * i / n;
        double z = 0.08 + height * i / n;
        s1.push_back(V(r * std::cos(a), r * std::sin(a), z));
        s2.push_back(V(r * std::cos(a + groove), r * std::sin(a + groove), z));
    }
    b.color = BASE;
    b.cylinder(V(0, 0, 0), V(0, 0, 0.06), 0.3, 0.3, 24, 8);
    b.color = BLUE;
    std::vector<int> i1 = b.polyline(s1);
    b.color = RED;
    std::vector<int> i2 = b.polyline(s2);
    for (int i = 2; i < n; i += 4) {
        int m = b.v((s1[i] + s2[i]) * 0.5);
        b.color = GREEN;
        b.e(i1[i], m);
        b.color = YELLOW;
        b.e(m, i2[i]);
    }
}

// id 4: nudo

inline void knot(Builder& b, double t) {
    const int p = 2, q = 3;
    const double R = 0.3, a = 0.13, tube = 0.055;
    const int n = 120, sides = 6;
    double ang = 0.5 * t;

    auto center = [&](double s) {
        return V((R + a * std::cos(q * s)) * std::cos(p * s), (R + a * std::cos(q * s)) * std::sin(p * s),
                 -a * std::sin(q * s));
    };
    auto toWorld = [&](V v) {
        v = V(v[0], -v[2], v[1]);
        v = rotZ(v, ang);
        return V(v[0], v[1], v[2] + 0.62);
    };

    b.color = BASE;
    b.cylinder(V(0, 0, 0), V(0, 0, 0.05), 0.3, 0.3, 24, 8);
    b.line(V(0, 0, 0.05), V(0, 0, 0.2));
    b.color = ORANGE;
    std::vector<std::vector<int>> rings;
    for (int i = 0; i < n; i++) {
        double s = 2 * PI * i / n;
        V c = center(s);
        V tan = norm(center(s + 1e-3) - center(s - 1e-3));
        V core(R * std::cos(p * s), R * std::sin(p * s), 0);
        V nrm = c - core;
        nrm = norm(nrm - tan * nrm.dot(tan));
        V bin = tan.cross(nrm);
        std::vector<int> ring;
        for (int k = 0; k < sides; k++) {
            double th = 2 * PI * k / sides;
            ring.push_back(b.v(toWorld(c + (nrm * std::cos(th) + bin * std::sin(th)) * tube)));
        }
        for (int k = 0; k < sides; k++) {
            b.e(ring[k], ring[(k + 1) % sides]);
        }
        rings.push_back(ring);
    }
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < sides; k++) {
            b.e(rings[i][k], rings[(i + 1) % n][k]);
        }
    }
}

// id 5: grúa

inline void crane(Builder& b, double t) {
    const double s = 0.07;
    const double baseH = 0.08, mastH = 1.15;
    b.color = BASE;
    b.box(V(0, 0, baseH / 2), V(0.5, 0.5, baseH));
    b.color = YELLOW;
    const double sq[4][2] = {{-s, -s}, {s, -s}, {s, s}, {-s, s}};
    std::vector<V> bottom, top;
    for (auto& c : sq) {
        bottom.push_back(V(c[0], c[1], baseH));
        top.push_back(V(c[0], c[1], mastH));
    }
    b.lattice(bottom, top, 8);

    size_t start = b.vertices.size();
    double z0 = mastH + 0.08;
    const double jib = 1.1, cj = 0.42;
    double x = 0.6 + 0.3 * std::sin(0.6 * t);
    double hookZ = 0.5 + 0.2 * std::sin(0.45 * t + 1);
    V hookEye(x, 0, hookZ);
    double loadTop = hookZ - 0.12;
    b.color = WOOD;
    b.box(V(x, 0, loadTop - 0.07), V(0.16, 0.16, 0.14));
    b.color = WHITE;
    const double sling[4][2] = {{-0.08, -0.08}, {0.08, -0.08}, {0.08, 0.08}, {-0.08, 0.08}};
    for (auto& c : sling) {
        b.line(hookEye, V(x + c[0], c[1], loadTop));
    }
    std::vector<V> hook = {V(x, 0, hookZ)};
    for (int k = 0; k < 10; k++) {
        hook.push_back(V(x + 0.03 * std::sin(PI * k / 6), 0, hookZ - 0.04 + 0.03 * std::cos(PI * k / 6)));
    }
    b.polyline(hook);
    b.line(V(x, -0.03, z0 - 0.03), V(x, -0.01, hookZ));
    b.line(V(x, 0.03, z0 - 0.03), V(x, 0.01, hookZ));
    b.color = DARK;
    b.box(V(0, 0, mastH + 0.04), V(2 * s + 0.02, 2 * s + 0.02, 0.08));
    b.color = BLUE;
    b.box(V(s + 0.05, -0.1, z0 - 0.06), V(0.1, 0.08, 0.1));
    b.color = GRAY;
    b.box(V(-cj + 0.07, 0, z0 - 0.06), V(0.12, 0.14, 0.12));
    b.color = DARK;
    b.box(V(x, 0, z0 - 0.015), V(0.08, 0.12, 0.03));
    b.color = YELLOW;
    b.lattice({V(s, -0.05, z0), V(s, 0.05, z0), V(s, 0, z0 + 0.09)},
              {V(jib, -0.05, z0), V(jib, 0.05, z0), V(jib, 0, z0 + 0.09)}, 12);
    b.lattice({V(-s, -0.06, z0), V(-s, 0.06, z0), V(-s, 0.06, z0 + 0.03), V(-s, -0.06, z0 + 0.03)},
              {V(-cj, -0.06, z0), V(-cj, 0.06, z0), V(-cj, 0.06, z0 + 0.03), V(-cj, -0.06, z0 + 0.03)}, 4);
    b.color = WHITE;
    V apex(0, 0, z0 + 0.35);
    for (auto& c : sq) {
        b.line(V(c[0], c[1], z0), apex);
    }
    b.line(apex, V(0.75, 0, z0 + 0.09));
    b.line(apex, V(-cj, 0.06, z0 + 0.03));
    b.line(apex, V(-cj, -0.06, z0 + 0.03));
    b.transform(start, [&](const V& v) { return rotZ(v, 0.25 * t); });
}

// id 6: robot

inline void robot(Builder& b, double t) {
    const double seamY = -0.18, plateZ = 0.04;
    b.color = BASE;
    b.box(V(0, seamY - 0.17, plateZ / 2), V(0.9, 0.32, plateZ));
    b.box(V(0, seamY + 0.17, plateZ / 2), V(0.9, 0.32, plateZ));
    double xTip = 0.35 * std::sin(0.6 * t);
    int nBead = std::max(2, (int)((xTip + 0.38) * 60));
    std::vector<V> bead;
    for (int i = 0; i < nBead; i++) {
        double xx = -0.38 + (xTip + 0.38) * i / (nBead - 1);
        bead.push_back(V(xx, seamY, plateZ + 0.008 * (1 + std::cos(40 * xx))));
    }
    b.color = BEAD;
    b.polyline(bead);

    const V base(0.0, 0.32, 0.0);
    const double shoulderH = 0.28, l1 = 0.45, l2 = 0.4, torch = 0.2;
    V tip(xTip, seamY, plateZ + 0.01);
    V toolDir = norm(V(0, 0.35, 1));
    V wrist = tip + toolDir * torch;

    double yaw = std::atan2(wrist[1] - base[1], wrist[0] - base[0]);
    V rdir(std::cos(yaw), std::sin(yaw), 0);
    V shoulder(base[0], base[1], shoulderH);
    double dr = std::hypot(wrist[0] - base[0], wrist[1] - base[1]);
    double dz = wrist[2] - shoulderH;
    double dist = std::hypot(dr, dz);
    double cosE = (dist * dist - l1 * l1 - l2 * l2) / (2 * l1 * l2);
    double elbowAng = std::acos(std::max(-1.0, std::min(1.0, cosE)));
    double shAng = std::atan2(dz, dr) + std::atan2(l2 * std::sin(elbowAng), l1 + l2 * std::cos(elbowAng));
    V elbow = shoulder + rdir * (l1 * std::cos(shAng)) + V(0, 0, l1 * std::sin(shAng));

    b.color = BASE;
    b.cylinder(base, base + V(0, 0, 0.06), 0.16, 0.16, 20, 10);
    b.color = BLUE;
    size_t start = b.vertices.size();
    b.cylinder(base + V(0, 0, 0.06), base + V(0, 0, 0.2), 0.12, 0.1, 12, 6);
    b.transform(start, [&](const V& v) { return base + rotZ(v - base, yaw); });
    b.beam(base + V(0, 0, 0.2), shoulder, 0.08);
    V axis(-rdir[1], rdir[0], 0);
    b.color = DARK;
    const std::pair<V, double> joints[3] = {{shoulder, 0.07}, {elbow, 0.06}, {wrist, 0.04}};
    for (auto& [j, rad] : joints) {
        b.cylinder(j - axis * 0.06, j + axis * 0.06, rad, rad, 12, 6);
    }
    b.color = BLUE;
    b.beam(shoulder, elbow, 0.08, axis);
    b.beam(elbow, wrist, 0.06, axis);
    V nozzle = tip + toolDir * 0.06;
    b.color = WHITE;
    b.cylinder(wrist, nozzle, 0.03, 0.025, 8, 4);
    b.cylinder(nozzle, tip, 0.022, 0.012, 8, 4);
    b.color = DARK;
    b.polyline({wrist + V(0, 0, 0.02), elbow + V(0, 0, 0.12), shoulder + V(0, 0.05, 0.12), base + V(0, 0.12, 0.12)});
    b.color = ORANGE;
    Random rnd((uint32_t)(int)(t * 25));
    for (int i = 0; i < 14; i++) {
        double a = 2 * PI * rnd.next();
        double up = 0.2 + 0.8 * rnd.next();
        double length = 0.04 + 0.12 * rnd.next();
        V d = norm(V(std::cos(a) * (1 - up), std::sin(a) * (1 - up), up));
        V p0 = tip + d * (0.02 + 0.05 * rnd.next());
        b.line(p0, p0 + d * length);
    }
    b.transform(0, [](const V& v) { return rotZ(v, PI); });
}

}

inline Model getModelAt(int modelId, float size, double t) {
    using namespace models_detail;
    Builder b;
    Model model;
    switch (modelId) {
        case 0: castle(b, t); break;
        case 1: tree(b, t); break;
        case 2: geodesic(b, t); break;
        case 3: dna(b, t); break;
        case 4: knot(b, t); break;
        case 5: crane(b, t); break;
        case 6: robot(b, t); break;
        default: return model;
    }
    for (const V& p : b.vertices) {
        model.vertices.push_back(cv::Point3f((float)p[0], (float)p[1], (float)p[2]) * size);
    }
    model.edges = b.edges;
    model.colors = b.edgeColors;
    return model;
}
