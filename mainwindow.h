#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE





struct showPix
{
    QPixmap pix;
    double size=1;
    QImage toSave;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void wheelEvent(QWheelEvent *event);// 用鼠标滚轮事件实现图像放大缩小
    void showInLabel(QImage &img,QLabel *label);
    void scaleImage(QLabel* label, double scaleFactor=1.2);

    QLabel* getLabelUnderMouse();


private slots:
    void on_pushButton_clicked();
    void mouseMoveEvent(QMouseEvent *event);

    void on_pushButton_4_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::MainWindow *ui;
    showPix pix1,pix2,pix3,pix4;
};






#endif // MAINWINDOW_H
