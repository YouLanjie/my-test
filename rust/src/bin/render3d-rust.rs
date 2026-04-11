// AI生成代码（基于c语言版）
// src/main.rs
use crossterm::{
    cursor::MoveTo,
    terminal::{Clear, ClearType, size},
    ExecutableCommand,
};
use rand::Rng;
use std::io::{stdout, Write};
use std::time::{Duration, Instant};
use std::f64::consts::PI;

// ======================== 配置常量 ========================
const FPS: u64 = 40;
const MAX_FRAME: usize = 10000;
const VIEW_SCALE: f64 = 30.0;      // 焦距 / 投影平面距离
const MAX_CHR: usize = CHRTABLE.len() - 1;
const CHRTABLE: &[char] = &[
    '$', '@', 'B', '%', '8', '&', 'W', 'M', '#', '*', 'o', 'a', 'h', 'k', 'b', 'd', 'p', 'q',
    'w', 'm', 'Z', 'O', '0', 'Q', 'L', 'C', 'J', 'U', 'Y', 'X', 'z', 'c', 'v', 'u', 'n', 'x',
    'r', 'j', 'f', 't', '/', '\\', '|', '(', ')', '1', '{', '}', '[', ']', '?', '-', '_', '+',
    '~', '<', '>', 'i', '!', 'l', 'I', ';', ':', ',', '"', '^', '`', '\'', '.', ' ',
];

// ======================== 数据结构 ========================
#[derive(Debug, Clone, Copy)]
struct Point3D {
    x: f64,
    y: f64,
    z: f64,
}

struct Object {
    center: Point3D,
    points: Vec<Point3D>,
}

impl Object {
    fn new(center: Point3D, points: &[Point3D]) -> Self {
        Self {
            center,
            points: points.to_vec(),
        }
    }

    /// 绕 Y 轴旋转所有点（相对于物体中心）
    fn rotate_y(&mut self, theta: f64) {
        let (sin_t, cos_t) = theta.sin_cos();
        for p in &mut self.points {
            let rx = p.x - self.center.x;
            let rz = p.z - self.center.z;
            let new_x = rx * cos_t - rz * sin_t;
            let new_z = rx * sin_t + rz * cos_t;
            p.x = new_x + self.center.x;
            p.z = new_z + self.center.z;
        }
    }

    /// 整体平移
    fn translate(&mut self, dx: f64, dy: f64, dz: f64) {
        self.center.x += dx;
        self.center.y += dy;
        self.center.z += dz;
        for p in &mut self.points {
            p.x += dx;
            p.y += dy;
            p.z += dz;
        }
    }
}

// ======================== 硬编码点云数据（直接从原 C 复制） ========================
const POINTS_DATA: &[Point3D] = &[
    // 原始数据共约 324 个点，此处仅展示开头和结尾以节省篇幅。
    // 实际运行时请将原 C 文件中的整个点列表完整粘贴到此处。
    // 下方给出完整数据的占位示意，你可以在实际编译时替换为完整列表。
    // 完整数据可从原代码中的巨型数组获取，格式完全一致。
    // ------------------------------------------------------------
    // 为了方便演示，这里只保留几个点，实际使用请务必替换为全部点。
    Point3D { x: -1.0, y: -1.0, z: -1.0 },
    Point3D { x: -1.0, y: -1.0, z: -1.0 },
    // ... 此处省略 300+ 个点 ...
    Point3D { x: 0.1, y: -0.8, z: 0.0 },
Point3D{x:-1.0,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-1.0},Point3D{x:-1.0,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-1.0},
Point3D{x:1.0,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:1.0},
Point3D{x:1.0,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:1.0},Point3D{x:-0.9,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-0.9},Point3D{x:-1.0,y:-0.9,z:-1.0},
Point3D{x:-0.9,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-0.9},Point3D{x:1.0,y:-0.9,z:-1.0},Point3D{x:-0.9,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-0.9},
Point3D{x:-1.0,y:-0.9,z:1.0},Point3D{x:-0.9,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.9},Point3D{x:1.0,y:-0.9,z:1.0},Point3D{x:-0.8,y:-1.0,z:-1.0},
Point3D{x:-1.0,y:-1.0,z:-0.8},Point3D{x:-1.0,y:-0.8,z:-1.0},Point3D{x:-0.8,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-0.8},Point3D{x:1.0,y:-0.8,z:-1.0},
Point3D{x:-0.8,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-0.8},Point3D{x:-1.0,y:-0.8,z:1.0},Point3D{x:-0.8,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.8},
Point3D{x:1.0,y:-0.8,z:1.0},Point3D{x:-0.7,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-0.7},Point3D{x:-1.0,y:-0.7,z:-1.0},Point3D{x:-0.7,y:1.0,z:-1.0},
Point3D{x:1.0,y:-1.0,z:-0.7},Point3D{x:1.0,y:-0.7,z:-1.0},Point3D{x:-0.7,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-0.7},Point3D{x:-1.0,y:-0.7,z:1.0},
Point3D{x:-0.7,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.7},Point3D{x:1.0,y:-0.7,z:1.0},Point3D{x:-0.6,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-0.6},
Point3D{x:-1.0,y:-0.6,z:-1.0},Point3D{x:-0.6,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-0.6},Point3D{x:1.0,y:-0.6,z:-1.0},Point3D{x:-0.6,y:-1.0,z:1.0},
Point3D{x:-1.0,y:1.0,z:-0.6},Point3D{x:-1.0,y:-0.6,z:1.0},Point3D{x:-0.6,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.6},Point3D{x:1.0,y:-0.6,z:1.0},
Point3D{x:-0.5,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-0.5},Point3D{x:-1.0,y:-0.5,z:-1.0},Point3D{x:-0.5,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-0.5},
Point3D{x:1.0,y:-0.5,z:-1.0},Point3D{x:-0.5,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-0.5},Point3D{x:-1.0,y:-0.5,z:1.0},Point3D{x:-0.5,y:1.0,z:1.0},
Point3D{x:1.0,y:1.0,z:-0.5},Point3D{x:1.0,y:-0.5,z:1.0},Point3D{x:-0.4,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-0.4},Point3D{x:-1.0,y:-0.4,z:-1.0},
Point3D{x:-0.4,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-0.4},Point3D{x:1.0,y:-0.4,z:-1.0},Point3D{x:-0.4,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-0.4},
Point3D{x:-1.0,y:-0.4,z:1.0},Point3D{x:-0.4,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.4},Point3D{x:1.0,y:-0.4,z:1.0},Point3D{x:-0.3,y:-1.0,z:-1.0},
Point3D{x:-1.0,y:-1.0,z:-0.3},Point3D{x:-1.0,y:-0.3,z:-1.0},Point3D{x:-0.3,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-0.3},Point3D{x:1.0,y:-0.3,z:-1.0},
Point3D{x:-0.3,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-0.3},Point3D{x:-1.0,y:-0.3,z:1.0},Point3D{x:-0.3,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.3},
Point3D{x:1.0,y:-0.3,z:1.0},Point3D{x:-0.2,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-0.2},Point3D{x:-1.0,y:-0.2,z:-1.0},Point3D{x:-0.2,y:1.0,z:-1.0},
Point3D{x:1.0,y:-1.0,z:-0.2},Point3D{x:1.0,y:-0.2,z:-1.0},Point3D{x:-0.2,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:-0.2},Point3D{x:-1.0,y:-0.2,z:1.0},
Point3D{x:-0.2,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.2},Point3D{x:1.0,y:-0.2,z:1.0},Point3D{x:-0.1,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:-0.1},
Point3D{x:-1.0,y:-0.1,z:-1.0},Point3D{x:-0.1,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:-0.1},Point3D{x:1.0,y:-0.1,z:-1.0},Point3D{x:-0.1,y:-1.0,z:1.0},
Point3D{x:-1.0,y:1.0,z:-0.1},Point3D{x:-1.0,y:-0.1,z:1.0},Point3D{x:-0.1,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:-0.1},Point3D{x:1.0,y:-0.1,z:1.0},
Point3D{x:0.0,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.0},Point3D{x:-1.0,y:0.0,z:-1.0},Point3D{x:0.0,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.0},
Point3D{x:1.0,y:0.0,z:-1.0},Point3D{x:0.0,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.0},Point3D{x:-1.0,y:0.0,z:1.0},Point3D{x:0.0,y:1.0,z:1.0},
Point3D{x:1.0,y:1.0,z:0.0},Point3D{x:1.0,y:0.0,z:1.0},Point3D{x:0.1,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.1},Point3D{x:-1.0,y:0.1,z:-1.0},
Point3D{x:0.1,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.1},Point3D{x:1.0,y:0.1,z:-1.0},Point3D{x:0.1,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.1},
Point3D{x:-1.0,y:0.1,z:1.0},Point3D{x:0.1,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.1},Point3D{x:1.0,y:0.1,z:1.0},Point3D{x:0.2,y:-1.0,z:-1.0},
Point3D{x:-1.0,y:-1.0,z:0.2},Point3D{x:-1.0,y:0.2,z:-1.0},Point3D{x:0.2,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.2},Point3D{x:1.0,y:0.2,z:-1.0},
Point3D{x:0.2,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.2},Point3D{x:-1.0,y:0.2,z:1.0},Point3D{x:0.2,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.2},
Point3D{x:1.0,y:0.2,z:1.0},Point3D{x:0.3,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.3},Point3D{x:-1.0,y:0.3,z:-1.0},Point3D{x:0.3,y:1.0,z:-1.0},
Point3D{x:1.0,y:-1.0,z:0.3},Point3D{x:1.0,y:0.3,z:-1.0},Point3D{x:0.3,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.3},Point3D{x:-1.0,y:0.3,z:1.0},
Point3D{x:0.3,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.3},Point3D{x:1.0,y:0.3,z:1.0},Point3D{x:0.4,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.4},
Point3D{x:-1.0,y:0.4,z:-1.0},Point3D{x:0.4,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.4},Point3D{x:1.0,y:0.4,z:-1.0},Point3D{x:0.4,y:-1.0,z:1.0},
Point3D{x:-1.0,y:1.0,z:0.4},Point3D{x:-1.0,y:0.4,z:1.0},Point3D{x:0.4,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.4},Point3D{x:1.0,y:0.4,z:1.0},
Point3D{x:0.5,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.5},Point3D{x:-1.0,y:0.5,z:-1.0},Point3D{x:0.5,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.5},
Point3D{x:1.0,y:0.5,z:-1.0},Point3D{x:0.5,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.5},Point3D{x:-1.0,y:0.5,z:1.0},Point3D{x:0.5,y:1.0,z:1.0},
Point3D{x:1.0,y:1.0,z:0.5},Point3D{x:1.0,y:0.5,z:1.0},Point3D{x:0.6,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.6},Point3D{x:-1.0,y:0.6,z:-1.0},
Point3D{x:0.6,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.6},Point3D{x:1.0,y:0.6,z:-1.0},Point3D{x:0.6,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.6},
Point3D{x:-1.0,y:0.6,z:1.0},Point3D{x:0.6,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.6},Point3D{x:1.0,y:0.6,z:1.0},Point3D{x:0.7,y:-1.0,z:-1.0},
Point3D{x:-1.0,y:-1.0,z:0.7},Point3D{x:-1.0,y:0.7,z:-1.0},Point3D{x:0.7,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.7},Point3D{x:1.0,y:0.7,z:-1.0},
Point3D{x:0.7,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.7},Point3D{x:-1.0,y:0.7,z:1.0},Point3D{x:0.7,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.7},
Point3D{x:1.0,y:0.7,z:1.0},Point3D{x:0.8,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.8},Point3D{x:-1.0,y:0.8,z:-1.0},Point3D{x:0.8,y:1.0,z:-1.0},
Point3D{x:1.0,y:-1.0,z:0.8},Point3D{x:1.0,y:0.8,z:-1.0},Point3D{x:0.8,y:-1.0,z:1.0},Point3D{x:-1.0,y:1.0,z:0.8},Point3D{x:-1.0,y:0.8,z:1.0},
Point3D{x:0.8,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.8},Point3D{x:1.0,y:0.8,z:1.0},Point3D{x:0.9,y:-1.0,z:-1.0},Point3D{x:-1.0,y:-1.0,z:0.9},
Point3D{x:-1.0,y:0.9,z:-1.0},Point3D{x:0.9,y:1.0,z:-1.0},Point3D{x:1.0,y:-1.0,z:0.9},Point3D{x:1.0,y:0.9,z:-1.0},Point3D{x:0.9,y:-1.0,z:1.0},
Point3D{x:-1.0,y:1.0,z:0.9},Point3D{x:-1.0,y:0.9,z:1.0},Point3D{x:0.9,y:1.0,z:1.0},Point3D{x:1.0,y:1.0,z:0.9},Point3D{x:1.0,y:0.9,z:1.0},

Point3D{x:-0.6,y:0.9,z:0.0},Point3D{x:-0.5,y:0.9,z:0.0},Point3D{x:-0.4,y:0.9,z:0.0},Point3D{x:-0.3,y:0.9,z:0.0},Point3D{x:0.1,y:0.9,z:0.0},
Point3D{x:0.2,y:0.9,z:0.0},Point3D{x:0.3,y:0.9,z:0.0},Point3D{x:0.4,y:0.9,z:0.0},Point3D{x:-0.7,y:0.8,z:0.0},Point3D{x:0.1,y:0.8,z:0.0},
Point3D{x:0.5,y:0.8,z:0.0},Point3D{x:-0.6,y:0.7,z:0.0},Point3D{x:-0.5,y:0.7,z:0.0},Point3D{x:-0.4,y:0.7,z:0.0},Point3D{x:0.1,y:0.7,z:0.0},
Point3D{x:0.2,y:0.7,z:0.0},Point3D{x:0.3,y:0.7,z:0.0},Point3D{x:0.4,y:0.7,z:0.0},Point3D{x:0.5,y:0.7,z:0.0},Point3D{x:-0.3,y:0.6,z:0.0},
Point3D{x:0.1,y:0.6,z:0.0},Point3D{x:0.5,y:0.6,z:0.0},Point3D{x:-0.7,y:0.5,z:0.0},Point3D{x:-0.6,y:0.5,z:0.0},Point3D{x:-0.5,y:0.5,z:0.0},
Point3D{x:-0.4,y:0.5,z:0.0},Point3D{x:0.1,y:0.5,z:0.0},Point3D{x:0.2,y:0.5,z:0.0},Point3D{x:0.3,y:0.5,z:0.0},Point3D{x:0.4,y:0.5,z:0.0},
Point3D{x:-0.9,y:0.2,z:0.0},Point3D{x:-0.1,y:0.2,z:0.0},Point3D{x:0.3,y:0.2,z:0.0},Point3D{x:0.4,y:0.2,z:0.0},Point3D{x:0.5,y:0.2,z:0.0},
Point3D{x:0.6,y:0.2,z:0.0},Point3D{x:-0.9,y:0.1,z:0.0},Point3D{x:-0.1,y:0.1,z:0.0},Point3D{x:0.3,y:0.1,z:0.0},Point3D{x:0.7,y:0.1,z:0.0},
Point3D{x:-0.9,y:0.0,z:0.0},Point3D{x:-0.1,y:0.0,z:0.0},Point3D{x:0.3,y:0.0,z:0.0},Point3D{x:0.4,y:0.0,z:0.0},Point3D{x:0.5,y:0.0,z:0.0},
Point3D{x:-0.9,y:-0.1,z:0.0},Point3D{x:-0.4,y:-0.1,z:0.0},Point3D{x:-0.1,y:-0.1,z:0.0},Point3D{x:0.3,y:-0.1,z:0.0},Point3D{x:0.6,y:-0.1,z:0.0},
Point3D{x:-0.9,y:-0.2,z:0.0},Point3D{x:-0.8,y:-0.2,z:0.0},Point3D{x:-0.7,y:-0.2,z:0.0},Point3D{x:-0.3,y:-0.2,z:0.0},Point3D{x:-0.2,y:-0.2,z:0.0},
Point3D{x:-0.1,y:-0.2,z:0.0},Point3D{x:0.3,y:-0.2,z:0.0},Point3D{x:0.7,y:-0.2,z:0.0},Point3D{x:-0.9,y:-0.5,z:0.0},Point3D{x:-0.7,y:-0.5,z:0.0},
Point3D{x:-0.5,y:-0.5,z:0.0},Point3D{x:-0.3,y:-0.5,z:0.0},Point3D{x:-0.1,y:-0.5,z:0.0},Point3D{x:0.2,y:-0.5,z:0.0},Point3D{x:0.3,y:-0.5,z:0.0},
Point3D{x:0.5,y:-0.5,z:0.0},Point3D{x:0.7,y:-0.5,z:0.0},Point3D{x:-0.9,y:-0.6,z:0.0},Point3D{x:-0.8,y:-0.6,z:0.0},Point3D{x:-0.7,y:-0.6,z:0.0},
Point3D{x:-0.4,y:-0.6,z:0.0},Point3D{x:-0.3,y:-0.6,z:0.0},Point3D{x:-0.2,y:-0.6,z:0.0},Point3D{x:0.1,y:-0.6,z:0.0},Point3D{x:0.2,y:-0.6,z:0.0},
Point3D{x:0.6,y:-0.6,z:0.0},Point3D{x:-0.9,y:-0.7,z:0.0},Point3D{x:-0.7,y:-0.7,z:0.0},Point3D{x:-0.4,y:-0.7,z:0.0},Point3D{x:-0.2,y:-0.7,z:0.0},
Point3D{x:0.2,y:-0.7,z:0.0},Point3D{x:0.5,y:-0.7,z:0.0},Point3D{x:0.7,y:-0.7,z:0.0},Point3D{x:0.1,y:-0.8,z:0.0},
];

// ======================== 投影与渲染 ========================
struct Screen {
    width: usize,
    height: usize,
    buffer: Vec<u8>,      // 存储深度值（0..MAX_CHR，越小越近）
}

impl Screen {
    fn new(width: usize, height: usize) -> Self {
        let size = width * height;
        Self {
            width,
            height,
            buffer: vec![MAX_CHR as u8; size],
        }
    }

    /// 清除屏幕缓冲区（全部设为最远深度）
    fn clear(&mut self) {
        for cell in &mut self.buffer {
            *cell = MAX_CHR as u8;
        }
    }

    /// 将 3D 点投影到屏幕并更新深度缓冲区
    fn project_point(&mut self, p: Point3D) {
        // 只绘制在摄像机前方（z < 0）的点
        if p.z >= 0.0 {
            return;
        }
        let inv_z = 1.0 / -p.z;
        let sx = (p.x * inv_z * VIEW_SCALE) as i32;
        let sy = (p.y * inv_z * VIEW_SCALE) as i32;
        let cx = self.width as i32 / 2;
        let cy = self.height as i32 / 2;
        let x = cx + sx;
        let y = cy - sy; // Y 轴翻转
        if x < 0 || x >= self.width as i32 || y < 0 || y >= self.height as i32 {
            return;
        }
        let depth = (-p.z) as u8;       // 深度值，越小表示越近
        let idx = (y * self.width as i32 + x) as usize;
        if depth < self.buffer[idx] {
            self.buffer[idx] = depth;
        }
    }

    /// 将深度缓冲区转换为 ASCII 字符串并输出到终端
    fn render(&self, out: &mut impl Write) -> std::io::Result<()> {
        for y in 0..self.height {
            for x in 0..self.width {
                let depth = self.buffer[y * self.width + x] as usize;
                let ch = CHRTABLE[depth.min(MAX_CHR)];
                write!(out, "{}", ch)?;
            }
            writeln!(out)?;
        }
        out.flush()?;
        Ok(())
    }
}

// ======================== 主程序 ========================
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 获取终端尺寸（减去边框留白）
    let (term_w, term_h) = size()?;
    let scr_w = (term_w as usize).saturating_sub(5);
    let scr_h = (term_h as usize).saturating_sub(5);
    if scr_w < 10 || scr_h < 10 {
        eprintln!("终端太小，请放大窗口后重试");
        return Ok(());
    }

    let mut screen = Screen::new(scr_w, scr_h);
    let mut obj = Object::new(Point3D { x: 0.0, y: 0.0, z: 0.0 }, POINTS_DATA);

    // 动画状态变量
    let mut theta = PI / FPS as f64;
    let mut step = 0.10;
    let mut dstep = 0.001;
    let mut vx;
    let mut vy;
    let mut rng = rand::thread_rng();

    // 初次随机速度
    vx = (rng.gen_range(0..1000) as f64 - 500.0) / 10000.0;
    vy = (rng.gen_range(0..1000) as f64 - 500.0) / 10000.0;

    // 清屏并隐藏光标（可选）
    let mut stdout = stdout();
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    for frame_idx in 0..MAX_FRAME {
        let frame_start = Instant::now();

        // 1. 清空深度缓冲区
        screen.clear();

        // 2. 更新物体状态（旋转、平移、边界反弹）
        // 旋转（绕 Y 轴）
        if theta > 0.0 {
            obj.rotate_y(theta);
        }
        // 边界反弹（X/Y）
        if obj.center.x > 1.0 || obj.center.x < -1.0 {
            vx = -vx;
        }
        if obj.center.y > 1.0 || obj.center.y < -1.0 {
            vy = -vy;
        }
        // 平移
        obj.translate(vx, vy, -step);

        // 3. 投影所有点
        for &p in &obj.points {
            screen.project_point(p);
        }

        // 4. 更新动画参数（step, theta, 随机速度等）
        step -= dstep;
        if step < 0.0 && obj.center.z >= -1.0 {
            dstep += 0.003;
            step = -step + dstep / 5.0;
            if step > 0.2 {
                step = 0.1;
                dstep = 0.001;
            }
            theta *= 2.0;
            vx = (rng.gen_range(0..1000) as f64 - 500.0) / 10000.0;
            vy = (rng.gen_range(0..1000) as f64 - 500.0) / 10000.0;
        }
        if theta > PI / FPS as f64 {
            theta *= 0.7;
        } else {
            theta *= 0.99;
        }

        // 5. 渲染并输出到终端
        stdout.execute(MoveTo(0, 0))?;
        screen.render(&mut stdout)?;
        // 打印调试信息（与原 C 一致）
        println!(
            "=> FPS: {}, FRAME: {}",
            FPS, frame_idx
        );
        println!(
            "=> CP xyz: {:.3}, {:.3}, {:.3}",
            obj.center.x, obj.center.y, obj.center.z
        );
        println!(
            "=> P[0] xyz: {:.3}, {:.3}, {:.3}",
            obj.points[0].x, obj.points[0].y, obj.points[0].z
        );

        // 6. 帧率控制
        let elapsed = frame_start.elapsed();
        let target = Duration::from_micros(1_000_000 / FPS);
        if elapsed < target {
            std::thread::sleep(target - elapsed);
        }
    }

    // 恢复光标显示
    stdout.execute(crossterm::cursor::Show)?;
    Ok(())
}
