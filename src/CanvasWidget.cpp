#include "CanvasWidget.h"

#include <algorithm>
#include <cmath>
#include <QFile>
#include <QKeyEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setAutoFillBackground(true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
}

bool CanvasWidget::undoLastShape() {
    if (shapes_.isEmpty()) {
        return false;
    }
    shapes_.removeLast();
    selectedShapeIndices_.clear();
    selectedShapeIndex_ = -1;
    update();
    return true;
}

void CanvasWidget::clearAll() {
    shapes_.clear();
    drawing_ = false;
    selecting_ = false;
    selectedShapeIndices_.clear();
    selectedShapeIndex_ = -1;
    update();
}

void CanvasWidget::toggleSnap() {
    snapEnabled_ = !snapEnabled_;
    emit snapStateChanged(snapEnabled_);
    update();
}

void CanvasWidget::toggleGrid() {
    gridVisible_ = !gridVisible_;
    emit gridStateChanged(gridVisible_);
    update();
}

bool CanvasWidget::deleteSelectedShape() {
    if (selectedShapeIndices_.isEmpty() && (selectedShapeIndex_ < 0 || selectedShapeIndex_ >= shapes_.size())) {
        return false;
    }

    if (!selectedShapeIndices_.isEmpty()) {
        QList<int> indices = selectedShapeIndices_.values();
        std::sort(indices.begin(), indices.end(), std::greater<int>());
        for (int i : indices) {
            if (i >= 0 && i < shapes_.size()) {
                shapes_.removeAt(i);
            }
        }
    } else {
        shapes_.removeAt(selectedShapeIndex_);
    }
    selectedShapeIndices_.clear();
    selectedShapeIndex_ = -1;
    update();
    return true;
}

bool CanvasWidget::isSnapEnabled() const {
    return snapEnabled_;
}

bool CanvasWidget::isGridVisible() const {
    return gridVisible_;
}

void CanvasWidget::setTool(ToolType tool) {
    tool_ = tool;
    drawing_ = false;
    selecting_ = false;
    emit toolChanged(toolName(tool_));
    emit hintMessage(QString("当前工具：%1").arg(toolName(tool_)));
    update();
}

CanvasWidget::ToolType CanvasWidget::currentTool() const {
    return tool_;
}

bool CanvasWidget::saveToJson(const QString &filePath) {
    QJsonArray shapeArray;
    for (const Shape &shape : shapes_) {
        QJsonObject item;
        if (shape.type == Shape::Type::Line) item["type"] = "line";
        if (shape.type == Shape::Type::Rectangle) item["type"] = "rect";
        if (shape.type == Shape::Type::Circle) item["type"] = "circle";
        item["ax"] = shape.a.x();
        item["ay"] = shape.a.y();
        item["bx"] = shape.b.x();
        item["by"] = shape.b.y();
        shapeArray.append(item);
    }

    QJsonObject root;
    root["version"] = 1;
    root["shapes"] = shapeArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit hintMessage("保存失败：无法写入文件");
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    emit hintMessage(QString("已保存到：%1").arg(filePath));
    return true;
}

bool CanvasWidget::loadFromJson(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit hintMessage("打开失败：无法读取文件");
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        emit hintMessage("打开失败：JSON 格式错误");
        return false;
    }

    const QJsonArray arr = doc.object().value("shapes").toArray();
    QVector<Shape> loaded;
    loaded.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();
        Shape s;
        const QString t = o.value("type").toString();
        if (t == "line") s.type = Shape::Type::Line;
        else if (t == "rect") s.type = Shape::Type::Rectangle;
        else if (t == "circle") s.type = Shape::Type::Circle;
        else continue;
        s.a = QPointF(o.value("ax").toDouble(), o.value("ay").toDouble());
        s.b = QPointF(o.value("bx").toDouble(), o.value("by").toDouble());
        loaded.push_back(s);
    }

    shapes_ = loaded;
    selectedShapeIndices_.clear();
    selectedShapeIndex_ = -1;
    drawing_ = false;
    selecting_ = false;
    update();
    emit hintMessage(QString("已打开：%1").arg(filePath));
    return true;
}

void CanvasWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (gridVisible_) {
        drawGrid(painter);
    }

    for (int i = 0; i < shapes_.size(); ++i) {
        drawShape(painter, shapes_[i], i == selectedShapeIndex_ || selectedShapeIndices_.contains(i));
    }

    if (selecting_) {
        painter.setPen(QPen(QColor(180, 220, 255), 1, Qt::DashLine));
        painter.setBrush(QColor(120, 170, 220, 35));
        painter.drawRect(QRectF(toScreen(selectionStart_), toScreen(selectionCurrent_)).normalized());
    }

    drawPreview(painter);
    drawMeasureText(painter);
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    setFocus();

    if (event->button() == Qt::MiddleButton) {
        panning_ = true;
        lastPanMousePos_ = event->position();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::RightButton) {
        clearAll();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    QPointF pos = toWorld(event->position());
    if (snapEnabled_) {
        pos = snapToGrid(pos);
    }

    if (!drawing_) {
        selectedShapeIndex_ = findShapeAt(pos);
        if (selectedShapeIndex_ >= 0) {
            selectedShapeIndices_.clear();
            selectedShapeIndices_.insert(selectedShapeIndex_);
            update();
            return;
        }

        if (tool_ == ToolType::Select) {
            selecting_ = true;
            selectionStart_ = pos;
            selectionCurrent_ = pos;
            selectedShapeIndices_.clear();
            selectedShapeIndex_ = -1;
            update();
            return;
        }

        drawing_ = true;
        startPoint_ = pos;
        previewPoint_ = pos;
        selectedShapeIndices_.clear();
        selectedShapeIndex_ = -1;
        emit hintMessage(QString("开始绘制：%1").arg(toolName(tool_)));
    } else {
        Shape shape;
        shape.a = startPoint_;
        shape.b = pos;
        if (tool_ == ToolType::Line) {
            shape.type = Shape::Type::Line;
        } else if (tool_ == ToolType::Rectangle) {
            shape.type = Shape::Type::Rectangle;
        } else {
            shape.type = Shape::Type::Circle;
        }

        shapes_.push_back(shape);
        drawing_ = false;
        selectedShapeIndex_ = shapes_.size() - 1;
        emit hintMessage(QString("%1 已创建").arg(toolName(tool_)));
    }

    update();
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        panning_ = false;
        setCursor(Qt::CrossCursor);
    }

    if (event->button() == Qt::LeftButton && selecting_) {
        selecting_ = false;
        selectedShapeIndices_.clear();
        const QRectF selectRect(selectionStart_, selectionCurrent_);
        const QRectF normalized = selectRect.normalized();
        for (int i = 0; i < shapes_.size(); ++i) {
            if (normalized.intersects(shapeBounds(shapes_[i])) || normalized.contains(shapeBounds(shapes_[i]))) {
                selectedShapeIndices_.insert(i);
            }
        }
        selectedShapeIndex_ = selectedShapeIndices_.isEmpty() ? -1 : *selectedShapeIndices_.begin();
        emit hintMessage(selectedShapeIndices_.isEmpty()
                             ? "框选完成：未选中图形"
                             : QString("框选完成：选中 %1 个图形").arg(selectedShapeIndices_.size()));
        update();
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    if (panning_) {
        const QPointF delta = event->position() - lastPanMousePos_;
        panOffset_ += delta;
        lastPanMousePos_ = event->position();
        update();
    }

    lastMouseWorld_ = toWorld(event->position());
    emit cursorWorldPositionChanged(lastMouseWorld_);

    if (!drawing_) {
        if (selecting_) {
            selectionCurrent_ = toWorld(event->position());
            update();
        }
        return;
    }

    QPointF pos = toWorld(event->position());
    const bool ortho = (event->modifiers() & Qt::ShiftModifier);
    pos = constrainedPoint(startPoint_, pos, ortho);

    if (snapEnabled_) {
        pos = snapToGrid(pos);
    }

    previewPoint_ = pos;
    update();
}

void CanvasWidget::wheelEvent(QWheelEvent *event) {
    constexpr double zoomFactor = 1.1;
    const QPointF cursorScreen = event->position();
    const QPointF before = toWorld(cursorScreen);

    if (event->angleDelta().y() > 0) {
        scale_ *= zoomFactor;
    } else {
        scale_ /= zoomFactor;
    }

    if (scale_ < 0.2) {
        scale_ = 0.2;
    } else if (scale_ > 8.0) {
        scale_ = 8.0;
    }

    panOffset_ = cursorScreen - before * scale_;
    emit zoomChanged(scale_);
    update();
}

void CanvasWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && drawing_) {
        drawing_ = false;
        emit hintMessage("已取消当前绘制");
        update();
        return;
    }
    QWidget::keyPressEvent(event);
}

void CanvasWidget::drawGrid(QPainter &painter) const {
    painter.setPen(QPen(QColor(50, 50, 50), 1));

    const int step = currentGridStep();
    const QPointF topLeftWorld = toWorld(QPointF(0, 0));
    const QPointF bottomRightWorld = toWorld(QPointF(width(), height()));

    const int xStart = static_cast<int>(topLeftWorld.x() / step) * step;
    const int xEnd = static_cast<int>(bottomRightWorld.x() / step + 1.0) * step;
    const int yStart = static_cast<int>(topLeftWorld.y() / step) * step;
    const int yEnd = static_cast<int>(bottomRightWorld.y() / step + 1.0) * step;

    for (int x = xStart; x <= xEnd; x += step) {
        painter.drawLine(toScreen(QPointF(x, topLeftWorld.y())),
                         toScreen(QPointF(x, bottomRightWorld.y())));
    }
    for (int y = yStart; y <= yEnd; y += step) {
        painter.drawLine(toScreen(QPointF(topLeftWorld.x(), y)),
                         toScreen(QPointF(bottomRightWorld.x(), y)));
    }
}

QPointF CanvasWidget::toWorld(const QPointF &screenPoint) const {
    return (screenPoint - panOffset_) / scale_;
}

QPointF CanvasWidget::toScreen(const QPointF &worldPoint) const {
    return worldPoint * scale_ + panOffset_;
}

QPointF CanvasWidget::snapToGrid(const QPointF &worldPoint) const {
    const double step = static_cast<double>(currentGridStep());
    const double x = std::round(worldPoint.x() / step) * step;
    const double y = std::round(worldPoint.y() / step) * step;
    return QPointF(x, y);
}

QPointF CanvasWidget::constrainedPoint(const QPointF &start, const QPointF &raw, bool ortho) const {
    if (!ortho || tool_ != ToolType::Line) {
        return raw;
    }
    const QPointF d = raw - start;
    if (std::abs(d.x()) >= std::abs(d.y())) {
        return QPointF(raw.x(), start.y());
    }
    return QPointF(start.x(), raw.y());
}

int CanvasWidget::findShapeAt(const QPointF &worldPoint) const {
    const double tolerance = 8.0 / scale_;
    for (int i = shapes_.size() - 1; i >= 0; --i) {
        const Shape &shape = shapes_[i];
        double d = 1e9;
        if (shape.type == Shape::Type::Line) {
            d = distanceToSegment(worldPoint, shape);
        } else if (shape.type == Shape::Type::Rectangle) {
            d = distanceToRectBorder(worldPoint, shape);
        } else {
            d = distanceToCircleBorder(worldPoint, shape);
        }
        if (d <= tolerance) {
            return i;
        }
    }
    return -1;
}

double CanvasWidget::distanceToSegment(const QPointF &p, const Shape &lineShape) const {
    const QPointF ab = lineShape.b - lineShape.a;
    const QPointF ap = p - lineShape.a;
    const double abLenSq = ab.x() * ab.x() + ab.y() * ab.y();
    if (abLenSq <= 1e-9) {
        const QPointF d = p - lineShape.a;
        return std::sqrt(d.x() * d.x() + d.y() * d.y());
    }

    double t = (ap.x() * ab.x() + ap.y() * ab.y()) / abLenSq;
    t = std::max(0.0, std::min(1.0, t));
    const QPointF closest = lineShape.a + t * ab;
    const QPointF d = p - closest;
    return std::sqrt(d.x() * d.x() + d.y() * d.y());
}

double CanvasWidget::distanceToRectBorder(const QPointF &p, const Shape &rectShape) const {
    const QRectF rect = QRectF(rectShape.a, rectShape.b).normalized();
    const QPointF tl = rect.topLeft();
    const QPointF tr = rect.topRight();
    const QPointF br = rect.bottomRight();
    const QPointF bl = rect.bottomLeft();

    Shape edge;
    edge.type = Shape::Type::Line;
    edge.a = tl; edge.b = tr;
    double d = distanceToSegment(p, edge);
    edge.a = tr; edge.b = br;
    d = std::min(d, distanceToSegment(p, edge));
    edge.a = br; edge.b = bl;
    d = std::min(d, distanceToSegment(p, edge));
    edge.a = bl; edge.b = tl;
    d = std::min(d, distanceToSegment(p, edge));
    return d;
}

double CanvasWidget::distanceToCircleBorder(const QPointF &p, const Shape &circleShape) const {
    const QPointF c = circleShape.a;
    const QPointF d = circleShape.b - circleShape.a;
    const double radius = std::sqrt(d.x() * d.x() + d.y() * d.y());
    const QPointF cp = p - c;
    const double distCenter = std::sqrt(cp.x() * cp.x() + cp.y() * cp.y());
    return std::abs(distCenter - radius);
}

void CanvasWidget::drawShape(QPainter &painter, const Shape &shape, bool selected) const {
    painter.setPen(QPen(selected ? QColor(255, 120, 120) : QColor(80, 200, 255), selected ? 3 : 2));

    if (shape.type == Shape::Type::Line) {
        painter.drawLine(toScreen(shape.a), toScreen(shape.b));
        return;
    }
    if (shape.type == Shape::Type::Rectangle) {
        painter.drawRect(QRectF(toScreen(shape.a), toScreen(shape.b)).normalized());
        return;
    }

    const QPointF c = toScreen(shape.a);
    const QPointF d = toScreen(shape.b) - c;
    const double r = std::sqrt(d.x() * d.x() + d.y() * d.y());
    painter.drawEllipse(c, r, r);
}

void CanvasWidget::drawPreview(QPainter &painter) const {
    if (!drawing_) {
        return;
    }
    painter.setPen(QPen(QColor(255, 200, 90), 1, Qt::DashLine));
    Shape preview;
    preview.a = startPoint_;
    preview.b = previewPoint_;
    if (tool_ == ToolType::Line) {
        preview.type = Shape::Type::Line;
    } else if (tool_ == ToolType::Rectangle) {
        preview.type = Shape::Type::Rectangle;
    } else {
        preview.type = Shape::Type::Circle;
    }
    drawShape(painter, preview, false);
}

void CanvasWidget::drawMeasureText(QPainter &painter) const {
    if (!drawing_) {
        return;
    }
    const QString text = currentMeasureText();
    if (text.isEmpty()) {
        return;
    }
    const QPointF textPos = toScreen(previewPoint_) + QPointF(12, -12);
    painter.setPen(QPen(QColor(240, 240, 240), 1));
    painter.drawText(textPos, text);
}

int CanvasWidget::currentGridStep() const {
    if (scale_ > 2.4) return 10;
    if (scale_ > 1.4) return 20;
    if (scale_ > 0.8) return 25;
    if (scale_ > 0.5) return 50;
    return 100;
}

QString CanvasWidget::toolName(ToolType tool) const {
    if (tool == ToolType::Select) return "选择";
    if (tool == ToolType::Line) return "直线";
    if (tool == ToolType::Rectangle) return "矩形";
    return "圆";
}

QString CanvasWidget::currentMeasureText() const {
    if (tool_ == ToolType::Select) {
        return QString();
    }
    const QPointF d = previewPoint_ - startPoint_;
    if (tool_ == ToolType::Line) {
        const double len = std::sqrt(d.x() * d.x() + d.y() * d.y());
        return QString("长度: %1").arg(len, 0, 'f', 1);
    }
    if (tool_ == ToolType::Rectangle) {
        return QString("宽: %1 高: %2").arg(std::abs(d.x()), 0, 'f', 1).arg(std::abs(d.y()), 0, 'f', 1);
    }
    const double r = std::sqrt(d.x() * d.x() + d.y() * d.y());
    return QString("半径: %1").arg(r, 0, 'f', 1);
}

QRectF CanvasWidget::shapeBounds(const Shape &shape) const {
    if (shape.type == Shape::Type::Line || shape.type == Shape::Type::Rectangle) {
        return QRectF(shape.a, shape.b).normalized();
    }
    const QPointF d = shape.b - shape.a;
    const double r = std::sqrt(d.x() * d.x() + d.y() * d.y());
    return QRectF(shape.a.x() - r, shape.a.y() - r, r * 2.0, r * 2.0);
}
