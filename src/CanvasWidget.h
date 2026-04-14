#pragma once

#include <QPointF>
#include <QString>
#include <QSet>
#include <QVector>
#include <QWidget>

class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    enum class ToolType {
        Select,
        Line,
        Rectangle,
        Circle
    };

    explicit CanvasWidget(QWidget *parent = nullptr);
    bool undoLastShape();
    void clearAll();
    void toggleSnap();
    void toggleGrid();
    bool deleteSelectedShape();
    bool isSnapEnabled() const;
    bool isGridVisible() const;
    void setTool(ToolType tool);
    ToolType currentTool() const;
    bool saveToJson(const QString &filePath);
    bool loadFromJson(const QString &filePath);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void cursorWorldPositionChanged(const QPointF &worldPoint);
    void snapStateChanged(bool enabled);
    void gridStateChanged(bool enabled);
    void zoomChanged(double scale);
    void toolChanged(const QString &toolName);
    void hintMessage(const QString &message);

private:
    struct Shape {
        enum class Type { Line, Rectangle, Circle };
        Type type = Type::Line;
        QPointF a;
        QPointF b;
    };

    void drawGrid(QPainter &painter) const;
    void drawShape(QPainter &painter, const Shape &shape, bool selected) const;
    void drawPreview(QPainter &painter) const;
    void drawMeasureText(QPainter &painter) const;
    int currentGridStep() const;
    QPointF toWorld(const QPointF &screenPoint) const;
    QPointF toScreen(const QPointF &worldPoint) const;
    QPointF snapToGrid(const QPointF &worldPoint) const;
    QPointF constrainedPoint(const QPointF &start, const QPointF &raw, bool ortho) const;
    int findShapeAt(const QPointF &worldPoint) const;
    double distanceToSegment(const QPointF &p, const Shape &lineShape) const;
    double distanceToRectBorder(const QPointF &p, const Shape &rectShape) const;
    double distanceToCircleBorder(const QPointF &p, const Shape &circleShape) const;
    QString toolName(ToolType tool) const;
    QString currentMeasureText() const;
    QRectF shapeBounds(const Shape &shape) const;

    QVector<Shape> shapes_;
    QSet<int> selectedShapeIndices_;
    bool drawing_ = false;
    bool selecting_ = false;
    QPointF startPoint_;
    QPointF previewPoint_;
    QPointF selectionStart_;
    QPointF selectionCurrent_;
    QPointF lastMouseWorld_;
    int selectedShapeIndex_ = -1;
    ToolType tool_ = ToolType::Select;
    bool snapEnabled_ = true;
    bool gridVisible_ = true;
    double scale_ = 1.0;
    QPointF panOffset_{0.0, 0.0};
    bool panning_ = false;
    QPointF lastPanMousePos_;
};
