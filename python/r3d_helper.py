#!/usr/bin/env python
# Created:2026.10.03

"""
（所有代码由AI生成）

N 体（半隐式欧拉）与二体近似下的近距离交会分析工具。

模块划分
--------
1. 通用工具           : format_bar, fmt_seconds
2. N 体积分与相遇检测 : nbody_accelerations, integrate_nbody,
                        integrate_nbody_until_encounter,
                        _refine_encounter_from_buffer, find_encounter
3. 二体轨道力学       : rv_to_elements, solve_kepler, elements_to_r,
                        elements_to_rv, distance_at, find_t_min_near
4. Δv 求解            : find_dv_linear, find_dv_optimal, report_dv
5. 天体文件解析       : parse_celestial_file
6. N 体对照与优化     : hill_radius_km, refine_local,
                        run_nbody_comparison, optimize_dv_nbody
7. 主流程             : main

约定
----
- 长度 km，时间 s，速度 km/s，引力常数按对应单位选择
- N 体引力常数 G_NBODY = 6.6743e-20 km^3/(kg·s^2)
- 二体 mu 由中心天体质量换算：mu = M * G_SI / 1e9  (km^3/s^2)
"""

import sys
import re
import time
from fractions import Fraction
from collections import deque

import numpy as np
from scipy.optimize import minimize, minimize_scalar


# ============================================================
# 1. 通用工具
# ============================================================

def format_bar(cur, total, width=30, prefix=""):
    """生成终端进度条字符串。"""
    if total <= 0:
        return f"{prefix}[{'-' * width}] {cur}"
    frac = min(max(cur / total, 0.0), 1.0)
    filled = int(round(width * frac))
    bar = "#" * filled + "-" * (width - filled)
    return f"{prefix}[{bar}] {cur}/{total} ({frac * 100:5.1f}%)"


def fmt_seconds(s):
    """把秒数格式化成人类可读的字符串。"""
    if s < 60:
        return f"{s:.2f}s"
    m, sec = divmod(s, 60)
    if m < 60:
        return f"{int(m)}m{sec:04.1f}s"
    h, m = divmod(m, 60)
    return f"{int(h)}h{int(m):02d}m{sec:04.1f}s"


# 全局常量：N 体模拟使用的引力常数（km^3 / (kg * s^2)）
G_NBODY = 6.6743e-20


# ============================================================
# 2. N 体积分与相遇检测
# ============================================================

def nbody_accelerations(pos, masses, G=G_NBODY):
    """
    向量化 N 体引力加速度。

    参数
    ----
    pos    : (N, 3) ndarray，各天体的惯性系位置 (km)
    masses : (N,)  ndarray，各天体的质量 (kg)
    G      : float，引力常数

    返回
    ----
    acc : (N, 3) ndarray，每个天体所受的引力加速度 (km/s^2)
    """
    # dr[i, j] = pos[j] - pos[i]，形状 (N, N, 3)
    dr = pos[None, :, :] - pos[:, None, :]
    r2 = np.einsum('ijk,ijk->ij', dr, dr)         # (N, N)
    np.fill_diagonal(r2, np.inf)                  # 排除自身作用
    inv_r3 = 1.0 / (r2 * np.sqrt(r2))             # 1 / r^3
    coef = G * masses[None, :] * inv_r3           # G * m_j / r_ij^3
    acc = np.einsum('ij,ijk->ik', coef, dr)       # (N, 3)
    return acc


def integrate_nbody(bodies, t_end, dt, G=G_NBODY, record_every=1):
    """
    半隐式欧拉 N 体积分（固定步长），全过程记录历史轨迹。

    参数
    ----
    bodies      : list[dict]
        每个 dict 含 'name', 'm'(kg), 'r'(km, ndarray(3,)),
        'v'(km/s, ndarray(3,))
    t_end       : float  总模拟时长 (s)
    dt          : float  时间步长 (s)
    G           : float  引力常数 km^3/(kg*s^2)
    record_every: int    每隔多少步记录一次

    返回
    ----
    dict : times(T,), pos(T,N,3), vel(T,N,3), masses(N,), dt, record_every
    """
    N = len(bodies)
    masses = np.array([b['m'] for b in bodies], dtype=float)
    pos = np.array([b['r'] for b in bodies], dtype=float)
    vel = np.array([b['v'] for b in bodies], dtype=float)

    n_steps = max(1, int(round(t_end / dt)))
    record_every = max(1, int(record_every))
    n_rec = n_steps // record_every + 1

    times = np.empty(n_rec)
    pos_hist = np.empty((n_rec, N, 3))
    vel_hist = np.empty((n_rec, N, 3))

    rec = 0
    for step in range(n_steps + 1):
        # 按间隔记录当前状态
        if step % record_every == 0 and rec < n_rec:
            times[rec] = step * dt
            pos_hist[rec] = pos
            vel_hist[rec] = vel
            rec += 1
        if step == n_steps:
            break
        # 半隐式欧拉：先更新速度，再用新速度更新位置
        acc = nbody_accelerations(pos, masses, G)
        vel = vel + acc * dt
        pos = pos + vel * dt

    return {
        'times': times[:rec],
        'pos': pos_hist[:rec],
        'vel': vel_hist[:rec],
        'masses': masses,
        'dt': dt,
        'record_every': record_every,
    }


def _refine_encounter_from_buffer(recent, dt):
    """
    用滚动缓冲里最后 3 步做三点抛物线细化，估计极小的时刻与距离。

    缓冲元素: (t, d, pos, vel)。相邻元素的时间间隔恒为 dt。
    若三点无法构成局部极小（denom 过小或步长越界），退回到中间点。
    """
    items = list(recent)
    if len(items) < 3:
        t, d, _, _ = items[-1]
        return t, d

    t0, d0, _, _ = items[-3]
    t1, d1, _, _ = items[-2]
    t2, d2, _, _ = items[-1]

    denom = d0 - 2.0 * d1 + d2
    if denom > 1e-30:
        offset = 0.5 * (d0 - d2) / denom
        if abs(offset) <= 0.5:
            t_ref = t1 + offset * dt
            d_ref = d1 - 0.25 * (d0 - d2) * offset
            return float(t_ref), float(d_ref)

    return float(t1), float(d1)


def integrate_nbody_until_encounter(bodies, i1, i2, dt, t_max,
                                    G=G_NBODY,
                                    history_len=3,
                                    verbose=False,
                                    print_every_steps=500000):
    """
    半隐式欧拉 N 体积分，边积分边检测 i1 与 i2 的最近时刻。

    相遇判据：径向相对速度 v_rad = (Δr·Δv)/|Δr| 由负变正。
    找到后在该步附近用三点抛物线细化，立即返回。

    参数
    ----
    bodies   : list[dict]  每个含 'name','m','r','v'
    i1, i2   : int         要检测相遇的两个天体在 bodies 中的下标
    dt       : float       步长 (s)
    t_max    : float       最大积分时间 (s)，硬上限，防死循环
    history_len : int      滚动保存最近多少步（用于抛物线细化，>=3）
    verbose  : bool
    print_every_steps : int  打印进度的步数间隔

    返回
    ----
    dict: found, t_min, d_min, t_final, n_steps
    """
    N = len(bodies)
    masses = np.array([b['m'] for b in bodies], dtype=float)
    pos = np.array([b['r'] for b in bodies], dtype=float)
    vel = np.array([b['v'] for b in bodies], dtype=float)

    def radial_state(p, v):
        """返回 i1 相对 i2 的距离与径向速度 (标量)。"""
        dr = p[i1] - p[i2]
        dv = v[i1] - v[i2]
        d = np.linalg.norm(dr)
        if d < 1e-12:
            return d, 0.0
        return d, float(np.dot(dr, dv) / d)

    # 滚动缓冲：存 (t, d, pos.copy(), vel.copy())
    # 用 maxlen 保证内存 O(N)，不随步数增长
    recent = deque(maxlen=max(history_len, 3))

    t = 0.0
    d_curr, v_rad_prev = radial_state(pos, vel)
    recent.append((t, d_curr, pos.copy(), vel.copy()))

    n_max = int(np.ceil(t_max / dt))
    n_steps = 0

    while n_steps < n_max:
        acc = nbody_accelerations(pos, masses, G)
        vel = vel + acc * dt
        pos = pos + vel * dt
        t += dt
        n_steps += 1

        d_curr, v_rad_curr = radial_state(pos, vel)
        recent.append((t, d_curr, pos.copy(), vel.copy()))

        # 径向速度由负变正 → 越过最近点
        if v_rad_prev < 0.0 and v_rad_curr >= 0.0:
            t_ref, d_ref = _refine_encounter_from_buffer(recent, dt)
            return {
                'found': True,
                't_min': t_ref,
                'd_min': d_ref,
                't_final': t,
                'n_steps': n_steps,
            }

        v_rad_prev = v_rad_curr

        if verbose and n_steps % print_every_steps == 0:
            print(f"    t = {t:.4e} s ({t / 86400:.3f} d), "
                  f"d = {d_curr:.6e} km, v_rad = {v_rad_curr:+.4e} km/s")

    return {
        'found': False,
        't_min': None,
        'd_min': None,
        't_final': t,
        'n_steps': n_steps,
    }


def find_encounter(result, i1, i2, t_lo=None, t_hi=None):
    """
    （离线）在已记录的 N 体积分结果中，找 i1 与 i2 的最近距离时刻。

    步骤：
      1. 计算径向相对速度 v_rad = (Δr·Δv)/|Δr|
      2. 找 v_rad 由负变正的索引（由靠近到远离）
      3. 在穿越点附近用三点抛物线细化
      4. 若窗口内没有符号变化，退回到全局最小距离

    返回 (t_min, d_min, k_index)
    """
    pos = result['pos']
    vel = result['vel']
    times = result['times']
    dt_rec = times[1] - times[0] if len(times) > 1 else result['dt']

    dr = pos[:, i1, :] - pos[:, i2, :]
    dv = vel[:, i1, :] - vel[:, i2, :]
    dist = np.linalg.norm(dr, axis=1)
    v_rad = np.einsum('ij,ij->i', dr, dv) / np.where(dist > 1e-12, dist, 1e-12)

    # 窗口过滤
    in_win = np.ones(len(times), dtype=bool)
    if t_lo is not None:
        in_win &= (times >= t_lo)
    if t_hi is not None:
        in_win &= (times <= t_hi)

    # 找 v_rad 由负变正且落在窗口内的穿越点
    k_cross = None
    for k in range(len(times) - 1):
        if not (in_win[k] and in_win[k + 1]):
            continue
        if v_rad[k] < 0 and v_rad[k + 1] >= 0:
            k_cross = k
            break

    # 无穿越 → 取窗口内最小距离
    if k_cross is None:
        dist_win = np.where(in_win, dist, np.inf)
        k = int(np.argmin(dist_win))
    else:
        k = k_cross

    # 在 k 附近做三点抛物线细化（仅当构成真正的局部极小时）
    t_best, d_best = float(times[k]), float(dist[k])
    if 0 < k < len(times) - 1:
        d0, d1, d2 = dist[k - 1], dist[k], dist[k + 1]
        denom = d0 - 2.0 * d1 + d2
        if denom > 1e-30:
            offset = 0.5 * (d0 - d2) / denom
            if abs(offset) <= 0.5:
                d_ref = d1 - 0.25 * (d0 - d2) * offset
                if 0.0 <= d_ref < d_best:
                    t_ref = times[k] + offset * dt_rec
                    t_best, d_best = float(t_ref), float(d_ref)

    return t_best, d_best, k


# ============================================================
# 3. 二体轨道力学
# ============================================================

def rv_to_elements(r, v, mu):
    """
    由初始位置 r、速度 v 计算开普勒轨道根数。

    注意：本函数适合 e>0 的椭圆轨道；圆轨道需特殊处理。

    返回 dict: a, e, i, Omega, omega, M0, n, T
    """
    r = np.asarray(r, dtype=float)
    v = np.asarray(v, dtype=float)

    r_norm = np.linalg.norm(r)
    v_norm = np.linalg.norm(v)

    h = np.cross(r, v)
    h_norm = np.linalg.norm(h)

    e_vec = np.cross(v, h) / mu - r / r_norm
    e = np.linalg.norm(e_vec)

    energy = v_norm ** 2 / 2 - mu / r_norm
    a = -mu / (2 * energy)

    if a <= 0 or e >= 1:
        raise ValueError("不是椭圆轨道：检查初始 r,v 或 mu。")

    i = np.arccos(np.clip(h[2] / h_norm, -1.0, 1.0))

    n_vec = np.cross([0, 0, 1], h)
    n_norm = np.linalg.norm(n_vec)

    if n_norm < 1e-12:
        Omega = 0.0
    else:
        Omega = np.arctan2(n_vec[1], n_vec[0]) % (2 * np.pi)

    if e < 1e-12:
        raise ValueError("圆轨道需要单独处理近地点定义。")

    if n_norm < 1e-12:
        # 赤道轨道简化
        omega = np.arctan2(e_vec[1], e_vec[0]) % (2 * np.pi)
    else:
        cos_omega = np.dot(n_vec, e_vec) / (n_norm * e)
        omega = np.arccos(np.clip(cos_omega, -1.0, 1.0))
        if e_vec[2] < 0:
            omega = 2 * np.pi - omega

    cos_nu = np.dot(e_vec, r) / (e * r_norm)
    nu = np.arccos(np.clip(cos_nu, -1.0, 1.0))
    if np.dot(r, v) < 0:
        nu = 2 * np.pi - nu

    E0 = 2 * np.arctan2(
        np.sqrt(1 - e) * np.sin(nu / 2),
        np.sqrt(1 + e) * np.cos(nu / 2)
    )
    M0 = E0 - e * np.sin(E0)
    M0 = M0 % (2 * np.pi)

    n_mean = np.sqrt(mu / a ** 3)
    T = 2 * np.pi / n_mean

    return {
        "a": a,
        "e": e,
        "i": i,
        "Omega": Omega,
        "omega": omega,
        "M0": M0,
        "n": n_mean,
        "T": T,
    }


def solve_kepler(M, e, tol=1e-12, max_iter=100):
    """牛顿法解开普勒方程 M = E - e sin E。"""
    M = np.mod(M, 2 * np.pi)
    E = M if e < 0.8 else np.pi
    for _ in range(max_iter):
        f = E - e * np.sin(E) - M
        fp = 1 - e * np.cos(E)
        dE = -f / fp
        E += dE
        if abs(dE) < tol:
            break
    return E


def elements_to_r(elem, t):
    """由轨道根数计算 t 时刻惯性系位置 (km)。"""
    a = elem["a"]
    e = elem["e"]
    i = elem["i"]
    Omega = elem["Omega"]
    omega = elem["omega"]
    M0 = elem["M0"]
    n = elem["n"]

    M = M0 + n * t
    E = solve_kepler(M, e)

    x_pf = a * (np.cos(E) - e)
    y_pf = a * np.sqrt(1 - e ** 2) * np.sin(E)
    z_pf = 0.0

    cosO, sinO = np.cos(Omega), np.sin(Omega)
    cosi, sini = np.cos(i), np.sin(i)
    cosw, sinw = np.cos(omega), np.sin(omega)

    # Rz(omega)
    x1 = cosw * x_pf - sinw * y_pf
    y1 = sinw * x_pf + cosw * y_pf
    z1 = z_pf

    # Rx(i)
    x2 = x1
    y2 = cosi * y1 - sini * z1
    z2 = sini * y1 + cosi * z1

    # Rz(Omega)
    x = cosO * x2 - sinO * y2
    y = sinO * x2 + cosO * y2
    z = z2

    return np.array([x, y, z])


def elements_to_rv(elem, t):
    """
    由轨道根数计算 t 时刻惯性系下的位置和速度。

    返回 (r, v)，单位分别为 km 和 km/s。
    """
    a = elem["a"]
    e = elem["e"]
    i = elem["i"]
    Omega = elem["Omega"]
    omega = elem["omega"]
    M0 = elem["M0"]
    n = elem["n"]

    M = M0 + n * t
    E = solve_kepler(M, e)

    cosE, sinE = np.cos(E), np.sin(E)

    # 近焦点坐标系位置
    x_pf = a * (cosE - e)
    y_pf = a * np.sqrt(1 - e ** 2) * sinE
    z_pf = 0.0

    # 近焦点坐标系速度：dE/dt = n / (1 - e cos E)
    Edot = n / (1 - e * cosE)
    vx_pf = -a * sinE * Edot
    vy_pf = a * np.sqrt(1 - e ** 2) * cosE * Edot
    vz_pf = 0.0

    # 旋转矩阵 R = Rz(Omega) Rx(i) Rz(omega)
    cosO, sinO = np.cos(Omega), np.sin(Omega)
    cosi, sini = np.cos(i), np.sin(i)
    cosw, sinw = np.cos(omega), np.sin(omega)

    R = np.array([
        [cosO * cosw - sinO * sinw * cosi, -cosO * sinw - sinO * cosw * cosi,  sinO * sini],
        [sinO * cosw + cosO * sinw * cosi, -sinO * sinw + cosO * cosw * cosi, -cosO * sini],
        [sinw * sini,                       cosw * sini,                       cosi],
    ])

    r = R @ np.array([x_pf, y_pf, z_pf])
    v = R @ np.array([vx_pf, vy_pf, vz_pf])
    return r, v


def distance_at(elem1, elem2, t):
    """计算 t 时刻两条椭圆轨道的瞬时距离 (km)。"""
    r1 = elements_to_r(elem1, t)
    r2 = elements_to_r(elem2, t)
    return np.linalg.norm(r1 - r2)


def find_t_min_near(elem1, elem2, t_center, half_window):
    """
    在 t_center 附近搜索最小距离。

    先粗采样定位局部极小，再用有界 Brent 法精化。

    返回 (t_min, d_min)
    """
    ts = np.linspace(t_center - half_window, t_center + half_window, 201)
    ds = np.array([distance_at(elem1, elem2, t) for t in ts])
    i0 = int(np.argmin(ds))
    t_a = ts[max(i0 - 1, 0)]
    t_b = ts[min(i0 + 1, len(ts) - 1)]

    res = minimize_scalar(
        lambda t: distance_at(elem1, elem2, t),
        bounds=(t_a, t_b), method="bounded",
        options={"xatol": 1e-3},
    )
    return res.x, res.fun


# ============================================================
# 4. Δv 求解
# ============================================================

def find_dv_linear(r1_0, v1_0, r2_0, v2_0, mu, t_min,
                   target="body1", eps=1e-6):
    """
    线性化方法：用有限差分的状态转移矩阵给出一阶 Δv 估计。

    target : 'body1' 或 'body2'，表示对谁施加脉冲。
    """
    r1_0 = np.asarray(r1_0, float)
    v1_0 = np.asarray(v1_0, float)
    r2_0 = np.asarray(r2_0, float)
    v2_0 = np.asarray(v2_0, float)

    elem1 = rv_to_elements(r1_0, v1_0, mu)
    elem2 = rv_to_elements(r2_0, v2_0, mu)

    r1_at = elements_to_r(elem1, t_min)
    r2_at = elements_to_r(elem2, t_min)
    dr = r1_at - r2_at

    Phi = np.zeros((3, 3))
    if target == "body1":
        v_base = v1_0
        r_base = r1_at
        for i in range(3):
            v_p = v_base.copy()
            v_p[i] += eps
            elem_p = rv_to_elements(r1_0, v_p, mu)
            Phi[:, i] = (elements_to_r(elem_p, t_min) - r_base) / eps
        shift = -dr
    elif target == "body2":
        v_base = v2_0
        r_base = r2_at
        for i in range(3):
            v_p = v_base.copy()
            v_p[i] += eps
            elem_p = rv_to_elements(r2_0, v_p, mu)
            Phi[:, i] = (elements_to_r(elem_p, t_min) - r_base) / eps
        shift = dr
    else:
        raise ValueError("target 必须是 'body1' 或 'body2'")

    dv = np.linalg.pinv(Phi) @ shift
    return dv


def find_dv_optimal(r1_0, v1_0, r2_0, v2_0, mu, t_min,
                    target="body1",
                    half_window=None,
                    max_dv=None,
                    verbose=False):
    """
    数值优化 Δv，使 t_min 附近的最小距离最小化（二体近似下）。

    参数
    ----
    r1_0, v1_0, r2_0, v2_0 : array-like
        初始位置(km)与速度(km/s)。
    mu : float
        中心天体引力参数 km^3/s^2。
    t_min : float
        原始最近距离时刻(s)。
    target : str
        'body1' 或 'body2'，对谁施加脉冲。
    half_window : float or None
        在 t_min 附近搜索新极小的半窗口(s)。None 则取 0.05*t_min。
    max_dv : float or None
        Δv 大小上限(km/s)。超过则加惩罚。None 不限制。
    verbose : bool
        是否打印优化过程。

    返回
    ----
    dict: dv, dv_magnitude, d_min_new, t_min_new, target
    """
    r1_0 = np.asarray(r1_0, float)
    v1_0 = np.asarray(v1_0, float)
    r2_0 = np.asarray(r2_0, float)
    v2_0 = np.asarray(v2_0, float)

    if half_window is None:
        half_window = 0.05 * t_min

    def build_elems(dv):
        """根据脉冲施加对象返回 (elem1_new, elem2_new)。"""
        if target == "body1":
            return (rv_to_elements(r1_0, v1_0 + dv, mu),
                    rv_to_elements(r2_0, v2_0, mu))
        else:
            return (rv_to_elements(r1_0, v1_0, mu),
                    rv_to_elements(r2_0, v2_0 + dv, mu))

    def eval_dmin(dv):
        """评估给定 Δv 下的最小距离。"""
        elem1_new, elem2_new = build_elems(dv)
        _, d = find_t_min_near(elem1_new, elem2_new, t_min, half_window)
        return d

    def objective(dv):
        """目标函数：最小距离，加上可选的 |Δv| 上限惩罚。"""
        d = eval_dmin(dv)
        if max_dv is not None:
            mag = np.linalg.norm(dv)
            if mag > max_dv:
                d += 1e6 * (mag - max_dv) ** 2
        return d

    dv0 = find_dv_linear(r1_0, v1_0, r2_0, v2_0, mu, t_min, target)

    if verbose:
        print(f"[init] dv0 = {dv0}, |dv0| = {np.linalg.norm(dv0):.6f} km/s")
        print(f"[init] d_min(dv0) = {eval_dmin(dv0):.3f} km")

    result = minimize(
        objective, dv0,
        method="Nelder-Mead",
        options={"xatol": 1e-8, "fatol": 1e-3, "maxiter": 5000, "disp": verbose},
    )

    dv_opt = result.x
    elem1_new, elem2_new = build_elems(dv_opt)
    t_new, d_new = find_t_min_near(elem1_new, elem2_new, t_min, half_window)

    return {
        "dv": dv_opt,
        "dv_magnitude": float(np.linalg.norm(dv_opt)),
        "d_min_new": float(d_new),
        "t_min_new": float(t_new),
        "target": target,
    }


def report_dv(result, d_min_old, label="", t_ref=None):
    """
    打印二体 Δv 优化结果。

    t_ref : float or None
        用于显示 t 偏移的参考时刻(s)。None 时只显示绝对 t_min。
    """
    print(f"=== {label} ===")
    print(f"target       = {result['target']}")
    print(f"Δv           = [{result['dv'][0]: .6f}, {result['dv'][1]: .6f}, {result['dv'][2]: .6f}] km/s")
    print(f"|Δv|         = {result['dv_magnitude']:.6f} km/s = {result['dv_magnitude'] * 1000:.3f} m/s")
    print(f"d_min 旧     = {d_min_old:.3f} km")
    print(f"d_min 新     = {result['d_min_new']:.3f} km")
    print(f"改善量       = {d_min_old - result['d_min_new']:.3f} km")
    print(f"t_min 新     = {result['t_min_new']:.3f} s")
    if t_ref is not None:
        print(f"t 偏移       = {result['t_min_new'] - t_ref:.3f} s")
    print()


# ============================================================
# 5. 天体文件解析
# ============================================================

def parse_celestial_file(path):
    """
    解析游戏导出的天体参数文件。

    返回 dict: {index: {'name', 'mass', 'radius', 'r', 'v'}}
    位置单位 km，速度单位 km/s，质量单位 kg。
    """
    pattern = re.compile(
        r'\[(\d+)\]\s+'                 # 序号
        r'(\S+)\s+'                     # 名称（不含空格）
        r'\(([^)]+)\)\s+'               # (质量kg/r=半径km)
        r'位置\(km\):\s*\{([^}]+)\}\s+'
        r'速度\(km/s\):\s*\{([^}]+)\}'
    )
    bodies = {}
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            m = pattern.search(line)
            if not m:
                continue
            idx = int(m.group(1))
            name = m.group(2)
            info = m.group(3)

            mm = re.search(r'([\d.eE+-]+)\s*kg', info)
            mass = float(mm.group(1)) if mm else 0.0
            rm = re.search(r'r\s*=\s*([\d.eE+-]+)\s*km', info)
            radius = float(rm.group(1)) if rm else 0.0

            pos = np.array([float(x) for x in m.group(4).split(',')])
            vel = np.array([float(x) for x in m.group(5).split(',')])

            bodies[idx] = {
                'name': name,
                'mass': mass,
                'radius': radius,
                'r': pos,
                'v': vel,
            }
    return bodies


# ============================================================
# 6. N 体对照与优化
# ============================================================

def hill_radius_km(a_km, m_kg, M_kg):
    """希尔球半径 (km)。a_km: 轨道半长轴; m_kg: 小天体质量; M_kg: 中心天体质量。"""
    return a_km * (m_kg / (3.0 * M_kg)) ** (1.0 / 3.0)


def run_nbody_comparison(bodies_all, idx_center, idx1, idx2,
                         t_min_2body, d_min_2body,
                         dv_fix=None, dv_target_idx=None,
                         dt=1800.0, t_max_factor=5.0,
                         scopes=('selected', 'all'),
                         verbose=True):
    """
    在 N 体（半隐式欧拉）模拟中比较二体近似结果。

    与旧版区别：不再预先设 t_end，而是交给
    integrate_nbody_until_encounter 一直积分直到找到相遇。

    参数
    ----
    bodies_all : dict {index: body_dict}
    idx_center : int   中心天体序号（仅用于 build_local_list 时保持一致）
    idx1, idx2 : int   感兴趣的两个天体序号
    t_min_2body, d_min_2body : float  二体基线
    dv_fix     : ndarray(3,) or None  修正脉冲
    dv_target_idx : int or None       脉冲施加对象
    dt         : float   步长 (s)
    t_max_factor : float 积分上限 = t_max_factor * t_min_2body
    scopes     : tuple   'selected' 表示只用三个天体，'all' 表示全体
    verbose    : bool

    返回
    ----
    dict : {场景名: 模拟结果}
    """
    if dv_fix is not None and dv_target_idx is None:
        raise ValueError("若提供 dv_fix，必须指定 dv_target_idx")

    t_max = t_max_factor * t_min_2body

    all_indices = sorted(bodies_all.keys())
    selected_indices = sorted(set([idx_center, idx1, idx2]))

    def build_local_list(indices, use_dv):
        """把 dict 形式的 bodies_all 转成 integrate_* 需要的 list[dict]。"""
        lst = []
        for i in indices:
            b = bodies_all[i]
            entry = {
                'name': b['name'],
                'm': b['mass'],
                'r': b['r'].copy(),
                'v': b['v'].copy(),
            }
            if use_dv and i == dv_target_idx:
                entry['v'] = entry['v'] + np.asarray(dv_fix, dtype=float)
            lst.append(entry)
        return lst

    def simulate(indices, use_dv, label):
        """在给定天体子集上运行 N 体积分并对比二体基线。"""
        local_bodies = build_local_list(indices, use_dv)
        li1 = indices.index(idx1)
        li2 = indices.index(idx2)

        if verbose:
            print(f"\n[{label}] N = {len(indices)} 体, dt = {dt:g} s, "
                  f"t_max = {t_max:.4e} s = {t_max / 86400:.3f} d")

        enc = integrate_nbody_until_encounter(
            local_bodies, li1, li2, dt, t_max,
            G=G_NBODY, verbose=verbose,
        )

        if not enc['found']:
            if verbose:
                print(f"  [警告] 在 t_max = {t_max:.4e} s 内未检测到相遇。"
                      f"可能相遇不存在，或 t_max 太小。")
            return {
                'found': False,
                't_min': None, 'd_min': None,
                't_final': enc['t_final'],
                'indices': indices,
            }

        if verbose:
            print(f"  t_min_Nbody = {enc['t_min']:.3f} s   "
                  f"(2-body: {t_min_2body:.3f} s, "
                  f"Δ = {enc['t_min'] - t_min_2body:+.3f} s)")
            print(f"  d_min_Nbody = {enc['d_min']:.3f} km  "
                  f"(2-body: {d_min_2body:.3f} km, "
                  f"Δ = {enc['d_min'] - d_min_2body:+.3f} km)")

        return {
            'found': True,
            't_min': enc['t_min'],
            'd_min': enc['d_min'],
            't_final': enc['t_final'],
            'indices': indices,
        }

    # 组装场景
    scenarios = {}
    if 'selected' in scopes:
        scenarios['3体_无修正'] = (selected_indices, False)
        if dv_fix is not None:
            scenarios['3体_有修正'] = (selected_indices, True)
    if 'all' in scopes:
        scenarios['全部_无修正'] = (all_indices, False)
        if dv_fix is not None:
            scenarios['全部_有修正'] = (all_indices, True)

    out = {}
    for name, (indices, use_dv) in scenarios.items():
        out[name] = simulate(indices, use_dv, name)

    if verbose:
        print("\n" + "=" * 96)
        print("二体近似 vs N 体模拟 结果对比（在线相遇检测）")
        print("=" * 96)
        hdr = (f"{'场景':<14} {'N':>4} {'found':>6} {'t_min(s)':>18} "
               f"{'d_min(km)':>18} {'Δt(s)':>16} {'Δd(km)':>18} {'rel_err':>10}")
        print(hdr)
        print("-" * len(hdr))
        print(f"{'二体基线':<14} {'-':>4} {'-':>6} {t_min_2body:>18.3f} "
              f"{d_min_2body:>18.3f} {'-':>16} {'-':>18} {'-':>10}")
        for name, r in out.items():
            if not r['found']:
                print(f"{name:<14} {len(r['indices']):>4} {'NO':>6} "
                      f"{'-':>18} {'-':>18} {'-':>16} {'-':>18} {'-':>10}")
                continue
            dt_err = r['t_min'] - t_min_2body
            dd_err = r['d_min'] - d_min_2body
            rel = 100.0 * dd_err / d_min_2body if d_min_2body > 0 else float('nan')
            print(f"{name:<14} {len(r['indices']):>4} {'YES':>6} "
                  f"{r['t_min']:>18.3f} {r['d_min']:>18.3f} "
                  f"{dt_err:>+16.3f} {dd_err:>+18.3f} {rel:>9.3f}%")
        print()

    return out


def refine_local(dv_best, eval_fn,
                 step0=0.005, min_step=1e-4,
                 max_iter=50, verbose=True):
    """
    以 dv_best 为中心的局部坐标下降。

    参数
    ----
    dv_best  : ndarray(3,)  初始 Δv
    eval_fn  : callable     dv -> 距离标量
    step0    : float        初始步长 (km/s)
    min_step : float        最小时步长，步长小于该值时停止
    max_iter : int          最大迭代次数
    verbose  : bool

    返回
    ----
    (dv, d_best)
    """
    dv = dv_best.copy()
    d_best = eval_fn(dv)
    step = step0
    t0 = time.perf_counter()

    for it in range(max_iter):
        improved = False
        for axis in range(3):
            for sign in (+1, -1):
                trial = dv.copy()
                trial[axis] += sign * step
                d = eval_fn(trial)
                if d < d_best:
                    dv, d_best = trial, d
                    improved = True
        if verbose:
            elapsed = time.perf_counter() - t0
            print(f"    local iter {it:02d}  step={step:.2e}  "
                  f"d_min={d_best:.4e}  elapsed={fmt_seconds(elapsed)}")
        if not improved:
            step *= 0.5
            if step < min_step:
                break
    return dv, d_best


def optimize_dv_nbody(bodies_all, idx_center, idx1, idx2,
                      dv_target_idx,
                      t_ref=1.6189e7,
                      dt=1800.0, t_max_factor=5.0,
                      dv_init=None,
                      dv_scales=None,
                      target_dmin=None,
                      method='Nelder-Mead',
                      maxiter=300,
                      verbose=True):
    """
    在 N 体模型下优化 Δv，使 d_min 最小化。

    流程
    ----
    1. 以 dv_init 为基础生成多起点（坐标轴方向 ± 不同幅值）
    2. 多起点扫描，找到 d_min 最小的起点
    3. 若达到 target_dmin，直接返回
    4. 否则以最佳起点做局部坐标下降精修

    参数
    ----
    t_ref        : float  参考交会时刻(s)，用于设置积分上限
    target_dmin  : float or None  目标最小距离，若达到可提前退出
    dv_scales    : list   起点幅度（相对 base_mag 的比例）

    返回
    ----
    dict: dv_opt, dv_magnitude, d_min, t_min, found,
          n_eval, history, skipped_refine, total_seconds
    """
    t_start_all = time.perf_counter()

    all_indices = sorted(bodies_all.keys())
    li1 = all_indices.index(idx1)
    li2 = all_indices.index(idx2)
    target_local_idx = all_indices.index(dv_target_idx)

    # 拷贝一份 base_bodies，避免污染原字典
    base_bodies = []
    for i in all_indices:
        b = bodies_all[i]
        base_bodies.append({
            'name': b['name'],
            'm': b['mass'],
            'r': b['r'].copy(),
            'v': b['v'].copy(),
        })

    t_max = t_max_factor * t_ref

    eval_count = [0]
    eval_times = []            # 每次评估耗时
    history = []               # (dv, d_min, t_min) 历史

    def eval_dmin(dv, tag=""):
        """
        在 N 体模型下评估给定 Δv 对应的最小距离。

        未找到相遇时返回一个很大的惩罚值 1e12。
        """
        t0 = time.perf_counter()
        eval_count[0] += 1

        # 从 base_bodies 深拷贝一份，避免修改原状态
        bs = [{'name': b['name'], 'm': b['m'],
               'r': b['r'].copy(), 'v': b['v'].copy()}
              for b in base_bodies]
        bs[target_local_idx]['v'] = bs[target_local_idx]['v'] + np.asarray(dv)

        enc = integrate_nbody_until_encounter(
            bs, li1, li2, dt, t_max, G=G_NBODY, verbose=False,
        )
        dt_eval = time.perf_counter() - t0
        eval_times.append(dt_eval)

        if not enc['found']:
            return 1e12

        history.append((np.asarray(dv).copy(), enc['d_min'], enc['t_min']))
        if verbose and tag:
            avg = sum(eval_times) / len(eval_times)
            print(f"    {tag} d_min={enc['d_min']:.4e} km  "
                  f"t={enc['t_min']:.4e} s  "
                  f"耗时={fmt_seconds(dt_eval)}  平均={fmt_seconds(avg)}")
        return enc['d_min']

    # ---------- 构造起点 ----------
    if dv_init is None:
        dv_init = np.zeros(3)
    if dv_scales is None:
        dv_scales = [0.1, 0.3, 1.0, 3.0]

    base_mag = max(np.linalg.norm(dv_init), 0.05)
    starts = [np.zeros(3)]
    if np.linalg.norm(dv_init) > 1e-9:
        starts.append(dv_init.copy())
    for s in dv_scales:
        for axis in range(3):
            for sign in (+1, -1):
                v = np.zeros(3)
                v[axis] = sign * s * base_mag
                starts.append(v)

    # 去重
    uniq = []
    for v in starts:
        if not any(np.allclose(v, u, atol=1e-12) for u in uniq):
            uniq.append(v)
    starts = uniq

    if verbose:
        print(f"[N-body opt] t_max = {t_max:.4e} s, dt = {dt:g} s")
        print(f"[N-body opt] dv_init = "
              f"[{dv_init[0]: .4e}, {dv_init[1]: .4e}, {dv_init[2]: .4e}] km/s")
        print(f"[N-body opt] 起点数 = {len(starts)}，目标 d_min = {target_dmin}")
        print(f"[N-body opt] 开始多起点扫描")
        print()

    # ---------- 多起点扫描 ----------
    n_starts = len(starts)
    best = {'d_min': np.inf, 'dv': None, 't_min': None, 'found': False}
    t_scan0 = time.perf_counter()

    for k, dv0 in enumerate(starts):
        d0 = eval_dmin(dv0, tag=f"start[{k:02d}]")
        if d0 < best['d_min']:
            best['d_min'] = d0
            best['dv'] = dv0.copy()

        if verbose:
            elapsed = time.perf_counter() - t_scan0
            avg = elapsed / (k + 1)
            eta = avg * (n_starts - k - 1)
            print(f"  {format_bar(k + 1, n_starts, prefix='scan ')}  "
                  f"已用={fmt_seconds(elapsed)}  "
                  f"ETA={fmt_seconds(eta)}  "
                  f"当前最佳={best['d_min']:.4e} km")

        if target_dmin is not None and best['d_min'] < target_dmin:
            if verbose:
                print(f"  >> 已达到目标 d_min < {target_dmin:.4e} km，"
                      f"提前结束扫描")
            break

    if verbose:
        print()

    # ---------- 若已达标，直接返回 ----------
    if target_dmin is not None and best['d_min'] < target_dmin:
        bs = [{'name': b['name'], 'm': b['m'],
               'r': b['r'].copy(), 'v': b['v'].copy()}
              for b in base_bodies]
        bs[target_local_idx]['v'] = bs[target_local_idx]['v'] + best['dv']
        enc_final = integrate_nbody_until_encounter(
            bs, li1, li2, dt, t_max, G=G_NBODY, verbose=False)
        total = time.perf_counter() - t_start_all
        if verbose:
            print(f"[N-body opt] 跳过精修，直接返回（达标）。"
                  f"总评估次数={eval_count[0]}，"
                  f"总耗时={fmt_seconds(total)}")
        return {
            'dv_opt': best['dv'],
            'dv_magnitude': float(np.linalg.norm(best['dv'])),
            'd_min': float(best['d_min']),
            't_min': float(enc_final['t_min']) if enc_final['found'] else None,
            'found': enc_final['found'],
            'n_eval': eval_count[0],
            'history': history,
            'skipped_refine': True,
            'total_seconds': total,
        }

    # ---------- 精修 ----------
    if verbose:
        print(f"[N-body opt] 最佳起点 d_min = {best['d_min']:.4e} km，"
              f"dv = [{best['dv'][0]: .4e}, {best['dv'][1]: .4e}, "
              f"{best['dv'][2]: .4e}]")
        print(f"[N-body opt] 开始局部坐标下降精修")

    dv_opt, d_final_local = refine_local(
        best['dv'], eval_dmin,
        step0=0.005, min_step=1e-4,
        max_iter=50, verbose=verbose,
    )

    # 最终再跑一遍 N 体积分，拿到精确的 t_min / d_min
    bs = [{'name': b['name'], 'm': b['m'],
           'r': b['r'].copy(), 'v': b['v'].copy()}
          for b in base_bodies]
    bs[target_local_idx]['v'] = bs[target_local_idx]['v'] + dv_opt
    enc_final = integrate_nbody_until_encounter(
        bs, li1, li2, dt, t_max, G=G_NBODY, verbose=False)

    total = time.perf_counter() - t_start_all
    d_final = enc_final['d_min'] if enc_final['found'] else np.inf

    if verbose:
        print()
        print(f"[N-body opt] 收敛: d_min = {d_final:.4e} km, "
              f"|dv| = {np.linalg.norm(dv_opt) * 1000:.3f} m/s")
        print(f"[N-body opt] 总评估次数 = {eval_count[0]}")
        print(f"[N-body opt] 总耗时 = {fmt_seconds(total)}")

    return {
        'dv_opt': dv_opt,
        'dv_magnitude': float(np.linalg.norm(dv_opt)),
        'd_min': float(d_final) if np.isfinite(d_final) else None,
        't_min': float(enc_final['t_min']) if enc_final['found'] else None,
        'found': enc_final['found'],
        'n_eval': eval_count[0],
        'history': history,
        'skipped_refine': False,
        'total_seconds': total,
    }


# ============================================================
# 7. 主流程
# ============================================================

def main():
    G = 6.6743e-11  # m^3/(kg*s^2)

    # ---------- 命令行参数 ----------
    # 用法: python ds.py <file> <center_idx> <body1_idx> <body2_idx>
    if len(sys.argv) >= 5:
        path = sys.argv[1]
        idx_center = int(sys.argv[2])
        idx1 = int(sys.argv[3])
        idx2 = int(sys.argv[4])
    else:
        print("用法: python ds.py <file> <center_idx> <body1_idx> <body2_idx>")
        sys.exit(1)

    # ---------- 读取天体 ----------
    bodies = parse_celestial_file(path)
    if not bodies:
        raise RuntimeError(f"未能从 {path} 解析出任何天体")

    for idx in (idx_center, idx1, idx2):
        if idx not in bodies:
            raise KeyError(
                f"文件中没有序号 {idx}，可用序号: {sorted(bodies.keys())}"
            )

    center = bodies[idx_center]
    b1 = bodies[idx1]
    b2 = bodies[idx2]

    # ---------- 中心天体引力参数 ----------
    # mass(kg) -> mu(km^3/s^2)
    mu = center['mass'] * G / 1e9

    # ---------- 转到中心天体参考系 ----------
    r1 = b1['r'] - center['r']
    v1 = b1['v'] - center['v']
    r2 = b2['r'] - center['r']
    v2 = b2['v'] - center['v']

    print(f"中心天体: [{idx_center}] {center['name']}")
    print(f"           mass = {center['mass']:g} kg, mu = {mu:g} km^3/s^2")
    print(f"天体 1:   [{idx1}] {b1['name']}")
    print(f"天体 2:   [{idx2}] {b2['name']}")
    print()

    # ---------- 轨道根数 ----------
    elem1 = rv_to_elements(r1, v1, mu)
    elem2 = rv_to_elements(r2, v2, mu)

    T1 = elem1["T"]
    T2 = elem2["T"]
    print(f"T1 = {T1:.3f} s")
    print(f"T2 = {T2:.3f} s")

    # 用有理数近似周期比，判断是否共振
    frac = Fraction(T1 / T2).limit_denominator(100)
    if frac.denominator > 50:
        print("周期比不可公度或近似分母太大，改用会合周期。")
        T_cycle = T1 * T2 / abs(T1 - T2)
    else:
        T_cycle = frac.denominator * T1
    print(f"T_cycle = {T_cycle:.3f} s")

    def distance(t):
        """二体近似下两条轨道的瞬时距离。"""
        r1t = elements_to_r(elem1, t)
        r2t = elements_to_r(elem2, t)
        return np.linalg.norm(r1t - r2t)

    # ---------- 粗采样 + 局部优化 ----------
    N = 20000
    ts = np.linspace(0, T_cycle, N)
    ds = np.array([distance(t) for t in ts])

    # 收集所有局部极小候选
    candidates = [(ds[0], ts[0]), (ds[-1], ts[-1])]
    for i in range(1, N - 1):
        if ds[i] <= ds[i - 1] and ds[i] <= ds[i + 1]:
            left = ts[i - 1]
            right = ts[i + 1]
            res = minimize_scalar(
                distance, bounds=(left, right),
                method="bounded", options={"xatol": 1e-6}
            )
            candidates.append((res.fun, res.x))

    d_min, t_min = min(candidates, key=lambda x: x[0])

    def fmt_vec(vec, unit):
        return f"[{vec[0]: .6f}, {vec[1]: .6f}, {vec[2]: .6f}] {unit}"

    # ---------- 输出初始状态 ----------
    print()
    print("=== t0 时刻状态 (相对中心天体) ===")
    print(f"天体 1 位置 r1 = {fmt_vec(r1, 'km')}")
    print(f"天体 1 速度 v1 = {fmt_vec(v1, 'km/s')}")
    print(f"天体 2 位置 r2 = {fmt_vec(r2, 'km')}")
    print(f"天体 2 速度 v2 = {fmt_vec(v2, 'km/s')}")
    print()
    print(f"最小距离 = {d_min:.3f} km")
    print(f"对应时刻 = {t_min:.3f} s")
    print(f"对应时刻 = {t_min / 3600:.3f} h")
    print(f"对应时刻 = {t_min / 3600 / 24:.3f} d")
    print()

    # ---------- 输出交会时刻状态 ----------
    r1_min, v1_min = elements_to_rv(elem1, t_min)
    r2_min, v2_min = elements_to_rv(elem2, t_min)
    rel_r = r1_min - r2_min
    rel_v = v1_min - v2_min

    print("=== t_min 时刻状态 ===")
    print(f"天体 1 位置 r1 = {fmt_vec(r1_min, 'km')}")
    print(f"天体 1 速度 v1 = {fmt_vec(v1_min, 'km/s')}")
    print(f"天体 2 位置 r2 = {fmt_vec(r2_min, 'km')}")
    print(f"天体 2 速度 v2 = {fmt_vec(v2_min, 'km/s')}")
    print()
    print(f"相对位置 Δr = {fmt_vec(rel_r, 'km')}")
    print(f"相对速度 Δv = {fmt_vec(rel_v, 'km/s')}")
    print(f"|Δr| = {np.linalg.norm(rel_r):.6f} km  (应与 d_min 一致)")
    print(f"|Δv| = {np.linalg.norm(rel_v):.6f} km/s")
    print()

    # ---------- N 体对照：无修正 ----------
    run_nbody_comparison(
        bodies, idx_center, idx1, idx2,
        t_min, d_min,
        dv_fix=None, dv_target_idx=None,
        dt=1800.0, t_max_factor=5,
        scopes=('selected', 'all'),
        verbose=True,
    )

    # ---------- 二体 Δv 优化（示例：只调天体 1） ----------
    res1 = find_dv_optimal(r1, v1, r2, v2, mu, t_min,
                           target="body1", verbose=False)
    report_dv(res1, d_min, "二体优化：只调天体 1", t_ref=t_min)

    # ---------- N 体对照：有修正 ----------
    run_nbody_comparison(
        bodies, idx_center, idx1, idx2,
        t_min, d_min,
        dv_fix=res1['dv'], dv_target_idx=idx1,
        dt=1800.0, t_max_factor=5,
        scopes=('selected', 'all'),
        verbose=True,
    )

    # ---------- N 体 Δv 优化 ----------
    center_body = bodies[idx_center]
    body1 = bodies[idx1]
    body2 = bodies[idx2]

    # 用当前时刻到中心的距离作为半长轴的粗略近似
    a1_approx = float(np.linalg.norm(body1['r'] - center_body['r']))
    a2_approx = float(np.linalg.norm(body2['r'] - center_body['r']))

    R_H_1 = hill_radius_km(a1_approx, body1['mass'], center_body['mass'])
    R_H_2 = hill_radius_km(a2_approx, body2['mass'], center_body['mass'])
    R_H_ref = max(R_H_1, R_H_2)  # 用较大的那个做“进入希尔球”的判据

    print()
    print("=== 希尔球半径（按当前位置近似半长轴） ===")
    print(f"天体 1 [{idx1}] {body1['name']}: R_H = {R_H_1:.3e} km")
    print(f"天体 2 [{idx2}] {body2['name']}: R_H = {R_H_2:.3e} km")
    print(f"参考希尔球 R_H_ref = {R_H_ref:.3e} km")

    # 用二体优化结果作为起点
    dv_seed = res1['dv']
    print(f"二体优化 Δv = {dv_seed}, |Δv| = {np.linalg.norm(dv_seed) * 1000:.3f} m/s")

    opt_nbody = optimize_dv_nbody(
        bodies, idx_center, idx1, idx2,
        dv_target_idx=idx1,
        t_ref=t_min,
        dt=1800.0,
        t_max_factor=5.0,
        dv_init=dv_seed,
        dv_scales=[0.1, 0.3, 1.0, 3.0],
        target_dmin=R_H_ref,
        method='Nelder-Mead',
        maxiter=200,
        verbose=True,
    )

    print(f"\n=== N 体优化结果 ===")
    print(f"Δv = {opt_nbody['dv_opt']}")
    print(f"|Δv| = {opt_nbody['dv_magnitude'] * 1000:.3f} m/s")
    print(f"d_min = {opt_nbody['d_min']:.3e} km")
    print(f"t_min = {opt_nbody['t_min']:.3f} s")
    print(f"是否进入希尔球 (R_H_ref = {R_H_ref:.3e} km): "
          f"{opt_nbody['d_min'] < R_H_ref}")


if __name__ == "__main__":
    main()
