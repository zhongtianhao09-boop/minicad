#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "CanvasWidget.h"

MainWindow::MainWindow() {
    setupUi();
    setupToolbar();
    setupShortcuts();
    setupConnections();
    applyTheme();
}

void MainWindow::setupUi() {
    setWindowTitle("迷你 CAD（简化版）");
    resize(1000, 700);

    centralContainer_ = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralContainer_);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    tipsToggleButton_ = new QPushButton("收起操作提示", this);
    tipsToggleButton_->setObjectName("tipsToggleButton");
    connect(tipsToggleButton_, &QPushButton::clicked, this, &MainWindow::toggleTipsDoc);

    tipsDocLabel_ = new QLabel(
        "【操作提示】\n"
        "1) 工具切换：S=选择，1=直线，2=矩形，3=圆。\n"
        "2) 选择工具下可拖拽框选多个图形，Delete 删除选中。\n"
        "3) Shift 绘制直线时启用正交约束，Esc 取消当前绘制。\n"
        "4) 中键平移，滚轮缩放，G 切换吸附，H 切换网格。\n"
        "5) Ctrl+O 打开 JSON，Ctrl+S 保存 JSON，Ctrl+Z 撤销。",
        this);
    tipsDocLabel_->setObjectName("tipsDoc");
    tipsDocLabel_->setWordWrap(true);

    canvas_ = new CanvasWidget(this);
    mainLayout->addWidget(tipsToggleButton_);
    mainLayout->addWidget(tipsDocLabel_);
    mainLayout->addWidget(canvas_, 1);
    setCentralWidget(centralContainer_);

    toolLabel_ = new QLabel("工具：选择", this);
    zoomLabel_ = new QLabel("缩放：100%", this);
    coordLabel_ = new QLabel("坐标 X: 0.0  Y: 0.0", this);
    snapLabel_ = new QLabel("吸附：开", this);
    gridLabel_ = new QLabel("网格：开", this);
    statusBar()->addPermanentWidget(toolLabel_);
    statusBar()->addPermanentWidget(zoomLabel_);
    statusBar()->addPermanentWidget(gridLabel_);
    statusBar()->addPermanentWidget(snapLabel_);
    statusBar()->addPermanentWidget(coordLabel_);
    statusBar()->showMessage(
        "S选择 1直线 2矩形 3圆 | 中键平移 | 滚轮缩放 | Ctrl+O打开 | Ctrl+S保存", 4500);
}

void MainWindow::setupShortcuts() {
    auto *undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, this, [this]() {
        const bool ok = canvas_->undoLastShape();
        statusBar()->showMessage(ok ? "已撤销上一步图形" : "没有可撤销的图形", 1500);
    });

    auto *clearShortcut = new QShortcut(QKeySequence(Qt::Key_C), this);
    connect(clearShortcut, &QShortcut::activated, this, [this]() {
        canvas_->clearAll();
        statusBar()->showMessage("画布已清空", 1500);
    });

    auto *snapShortcut = new QShortcut(QKeySequence(Qt::Key_G), this);
    connect(snapShortcut, &QShortcut::activated, this, [this]() {
        canvas_->toggleSnap();
        statusBar()->showMessage("网格吸附状态已切换", 1200);
    });

    auto *deleteShortcut = new QShortcut(QKeySequence::Delete, this);
    connect(deleteShortcut, &QShortcut::activated, this, [this]() {
        const bool ok = canvas_->deleteSelectedShape();
        statusBar()->showMessage(ok ? "已删除选中图形" : "当前没有选中图形", 1200);
    });

    auto *gridShortcut = new QShortcut(QKeySequence(Qt::Key_H), this);
    connect(gridShortcut, &QShortcut::activated, this, [this]() {
        canvas_->toggleGrid();
        statusBar()->showMessage("网格显示状态已切换", 1200);
    });

    auto *lineToolShortcut = new QShortcut(QKeySequence(Qt::Key_1), this);
    connect(lineToolShortcut, &QShortcut::activated, this, [this]() { canvas_->setTool(CanvasWidget::ToolType::Line); });
    auto *selectToolShortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
    connect(selectToolShortcut, &QShortcut::activated, this, [this]() { canvas_->setTool(CanvasWidget::ToolType::Select); });
    auto *rectToolShortcut = new QShortcut(QKeySequence(Qt::Key_2), this);
    connect(rectToolShortcut, &QShortcut::activated, this, [this]() { canvas_->setTool(CanvasWidget::ToolType::Rectangle); });
    auto *circleToolShortcut = new QShortcut(QKeySequence(Qt::Key_3), this);
    connect(circleToolShortcut, &QShortcut::activated, this, [this]() { canvas_->setTool(CanvasWidget::ToolType::Circle); });

    auto *cancelShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(cancelShortcut, &QShortcut::activated, this, [this]() {
        statusBar()->showMessage("已发送取消绘制命令", 1000);
    });

    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() {
        const QString filePath = QFileDialog::getSaveFileName(this, "保存工程", "", "CAD JSON (*.json)");
        if (filePath.isEmpty()) return;
        canvas_->saveToJson(filePath);
    });

    auto *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, [this]() {
        const QString filePath = QFileDialog::getOpenFileName(this, "打开工程", "", "CAD JSON (*.json)");
        if (filePath.isEmpty()) return;
        canvas_->loadFromJson(filePath);
    });
}

void MainWindow::setupToolbar() {
    auto *toolbar = addToolBar("工具栏");
    toolbar->setMovable(false);

    auto *selectAction = toolbar->addAction("选择(S)");
    connect(selectAction, &QAction::triggered, this,
            [this]() { canvas_->setTool(CanvasWidget::ToolType::Select); });

    auto *lineAction = toolbar->addAction("直线(1)");
    connect(lineAction, &QAction::triggered, this,
            [this]() { canvas_->setTool(CanvasWidget::ToolType::Line); });

    auto *rectAction = toolbar->addAction("矩形(2)");
    connect(rectAction, &QAction::triggered, this,
            [this]() { canvas_->setTool(CanvasWidget::ToolType::Rectangle); });

    auto *circleAction = toolbar->addAction("圆(3)");
    connect(circleAction, &QAction::triggered, this,
            [this]() { canvas_->setTool(CanvasWidget::ToolType::Circle); });

    toolbar->addSeparator();

    auto *undoAction = toolbar->addAction("撤销");
    connect(undoAction, &QAction::triggered, this, [this]() {
        const bool ok = canvas_->undoLastShape();
        statusBar()->showMessage(ok ? "已撤销上一步图形" : "没有可撤销的图形", 1200);
    });

    auto *clearAction = toolbar->addAction("清空");
    connect(clearAction, &QAction::triggered, this, [this]() {
        canvas_->clearAll();
        statusBar()->showMessage("画布已清空", 1200);
    });

    auto *deleteAction = toolbar->addAction("删除选中");
    connect(deleteAction, &QAction::triggered, this, [this]() {
        const bool ok = canvas_->deleteSelectedShape();
        statusBar()->showMessage(ok ? "已删除选中图形" : "当前没有选中图形", 1200);
    });

    auto *snapAction = toolbar->addAction("切换吸附 (G)");
    connect(snapAction, &QAction::triggered, this, [this]() {
        canvas_->toggleSnap();
        statusBar()->showMessage("网格吸附状态已切换", 1200);
    });

    auto *gridAction = toolbar->addAction("切换网格 (H)");
    connect(gridAction, &QAction::triggered, this, [this]() {
        canvas_->toggleGrid();
        statusBar()->showMessage("网格显示状态已切换", 1200);
    });

    toolbar->addSeparator();

    auto *openAction = toolbar->addAction("打开(Ctrl+O)");
    connect(openAction, &QAction::triggered, this, [this]() {
        const QString filePath = QFileDialog::getOpenFileName(this, "打开工程", "", "CAD JSON (*.json)");
        if (filePath.isEmpty()) return;
        canvas_->loadFromJson(filePath);
    });

    auto *saveAction = toolbar->addAction("保存(Ctrl+S)");
    connect(saveAction, &QAction::triggered, this, [this]() {
        const QString filePath = QFileDialog::getSaveFileName(this, "保存工程", "", "CAD JSON (*.json)");
        if (filePath.isEmpty()) return;
        canvas_->saveToJson(filePath);
    });
}

void MainWindow::setupConnections() {
    connect(canvas_, &CanvasWidget::cursorWorldPositionChanged, this, [this](const QPointF &p) {
        coordLabel_->setText(QString("坐标 X: %1  Y: %2").arg(p.x(), 0, 'f', 1).arg(p.y(), 0, 'f', 1));
    });

    connect(canvas_, &CanvasWidget::snapStateChanged, this, [this](bool enabled) {
        snapLabel_->setText(enabled ? "吸附：开" : "吸附：关");
    });

    connect(canvas_, &CanvasWidget::gridStateChanged, this, [this](bool enabled) {
        gridLabel_->setText(enabled ? "网格：开" : "网格：关");
    });

    connect(canvas_, &CanvasWidget::zoomChanged, this, [this](double scale) {
        zoomLabel_->setText(QString("缩放：%1%").arg(scale * 100.0, 0, 'f', 0));
    });

    connect(canvas_, &CanvasWidget::toolChanged, this, [this](const QString &toolName) {
        toolLabel_->setText(QString("工具：%1").arg(toolName));
    });

    connect(canvas_, &CanvasWidget::hintMessage, this,
            [this](const QString &message) { statusBar()->showMessage(message, 1300); });
}

void MainWindow::applyTheme() {
    setStyleSheet(
        "QMainWindow { background: #1f1f1f; }"
        "QToolBar { background: #2a2a2a; border: 0; spacing: 6px; padding: 6px; }"
        "QToolButton { background: #3a3a3a; color: #f0f0f0; border: 1px solid #555; padding: 5px 10px; }"
        "QToolButton:hover { background: #4b4b4b; }"
        "QPushButton#tipsToggleButton { background: #333943; color: #f0f0f0; border: 1px solid #4f5968;"
        " padding: 6px 10px; border-radius: 4px; text-align: left; }"
        "QPushButton#tipsToggleButton:hover { background: #404957; }"
        "QLabel#tipsDoc { background: #2b2f36; border: 1px solid #4f5968; border-radius: 4px; padding: 8px; }"
        "QStatusBar { background: #2a2a2a; color: #e0e0e0; }"
        "QLabel { color: #e0e0e0; }");
}

void MainWindow::toggleTipsDoc() {
    tipsExpanded_ = !tipsExpanded_;
    tipsDocLabel_->setVisible(tipsExpanded_);
    tipsToggleButton_->setText(tipsExpanded_ ? "收起操作提示" : "展开操作提示");
    statusBar()->showMessage(tipsExpanded_ ? "已展开操作提示" : "已收起操作提示", 1200);
}
