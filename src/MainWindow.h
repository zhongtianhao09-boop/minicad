#pragma once

#include <QMainWindow>

class CanvasWidget;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    void setupUi();
    void setupShortcuts();
    void setupToolbar();
    void setupConnections();
    void applyTheme();
    void toggleTipsDoc();

    CanvasWidget *canvas_ = nullptr;
    QWidget *centralContainer_ = nullptr;
    QPushButton *tipsToggleButton_ = nullptr;
    QLabel *tipsDocLabel_ = nullptr;
    QLabel *coordLabel_ = nullptr;
    QLabel *snapLabel_ = nullptr;
    QLabel *gridLabel_ = nullptr;
    QLabel *toolLabel_ = nullptr;
    QLabel *zoomLabel_ = nullptr;
    bool tipsExpanded_ = true;
};
