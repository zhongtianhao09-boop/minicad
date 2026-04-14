# MiniCAD (Qt Simplified)

一个基于 `C++ + Qt6 + CMake` 的轻量级二维绘图小项目，适合学习 CAD 基础交互与桌面图形程序结构。

## 功能特性

- 二维图元绘制：直线、矩形、圆
- 选择与框选：支持单选和拖拽框选
- 视图控制：中键平移、滚轮缩放
- 精度辅助：网格显示、网格吸附、直线正交约束（Shift）
- 编辑能力：删除、撤销、清空
- 工程文件：JSON 保存/加载

## 快捷键

- `S`：选择工具
- `1`：直线工具
- `2`：矩形工具
- `3`：圆工具
- `Delete`：删除选中图形
- `Ctrl+Z`：撤销上一步
- `Ctrl+S`：保存 JSON 工程
- `Ctrl+O`：打开 JSON 工程
- `G`：切换网格吸附
- `H`：切换网格显示
- `Shift`：绘制直线时启用正交约束
- `Esc`：取消当前绘制
- `C` 或右键：清空画布

## 构建与运行

### 方式一：Qt Creator（推荐）

1. 打开 Qt Creator
2. 选择 `CMakeLists.txt`
3. 选择可用 Kit（Qt 6.x + MinGW 或 MSVC）
4. 构建并运行

### 方式二：命令行（Windows）

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="你的Qt安装路径"
cmake --build build
```

> 注意：请确保本机已经安装可用 C++ 编译器（MinGW 或 MSVC）。

## 项目结构

```text
mini-cad/
├─ CMakeLists.txt
├─ README.md
└─ src/
   ├─ main.cpp
   ├─ MainWindow.h
   ├─ MainWindow.cpp
   ├─ CanvasWidget.h
   └─ CanvasWidget.cpp
```

## 后续规划

- 图层管理（显示/锁定/颜色）
- 精确输入（长度、角度、半径）
- 夹点编辑与移动/复制
- 对象捕捉（端点/中点/圆心/交点）
- 更多格式导入导出（如 DXF）
