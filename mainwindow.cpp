#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <QFileDialog>  //添加的头文件
#include <QDebug>       //添加的头文件
#include<QImage>
#include<QWheelEvent>

#include<iostream>
#include<cmath>
#include<QPainter>

#include <windows.h>


double calculateSSIM(const QImage& image1, const QImage& image2) {
    // 转换QImage到cv::Mat
    cv::Mat img1 = cv::Mat(image1.height(), image1.width(), CV_8UC3, const_cast<uchar*>(image1.bits()), image1.bytesPerLine()).clone();
    cv::Mat img2 = cv::Mat(image2.height(), image2.width(), CV_8UC3, const_cast<uchar*>(image2.bits()), image2.bytesPerLine()).clone();

    // 转换到灰度
    cv::cvtColor(img1, img1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(img2, img2, cv::COLOR_BGR2GRAY);

    // 初始化变量
    cv::Mat img1_sq, img2_sq, img1_img2;
    cv::Scalar mu1, mu2, sigma1_sq, sigma2_sq, sigma12;

    // 计算平均值
    cv::meanStdDev(img1, mu1, sigma1_sq);
    cv::meanStdDev(img2, mu2, sigma2_sq);

    sigma1_sq.val[0] = sigma1_sq.val[0] * sigma1_sq.val[0];
    sigma2_sq.val[0] = sigma2_sq.val[0] * sigma2_sq.val[0];

    // 计算图像的平方和
    cv::pow(img1, 2.0, img1_sq);
    cv::pow(img2, 2.0, img2_sq);
    cv::multiply(img1, img2, img1_img2);

    // 计算平均值
    cv::meanStdDev(img1_sq, mu1, sigma1_sq);
    cv::meanStdDev(img2_sq, mu2, sigma2_sq);
    cv::meanStdDev(img1_img2, mu1, sigma12);

    // 计算SSIM
    double C1 = 6.5025, C2 = 58.5225;
    double ssim_val = (2.0 * mu1.val[0]*mu2.val[0] + C1) * (2.0 * sigma12.val[0] + C2);
    ssim_val /= (mu1.val[0]*mu1.val[0] + mu2.val[0]*mu2.val[0] + C1) * (sigma1_sq.val[0] + sigma2_sq.val[0] + C2);

    return ssim_val;
}




void MainWindow::scaleImage(QLabel* label, double scaleFactor) {
    showPix *processPix;
    if(label==ui->label)        processPix=&pix1;
    else if(label==ui->label_2) processPix=&pix2;
    else if(label==ui->label_3) processPix=&pix3;
    else if(label==ui->label_4) processPix=&pix4;
    else qDebug()<<"illegal label";




    QSize newSize = processPix->pix.size() * ((*processPix).size+scaleFactor);
    processPix->size+=scaleFactor;
            QPixmap scaledPixmap = processPix->pix.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            QPixmap pixmap(label->size());
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            int x = (pixmap.width() - scaledPixmap.width()) / 2;
            int y = (pixmap.height() - scaledPixmap.height()) / 2;
            painter.drawPixmap(x, y, scaledPixmap);

            label->setPixmap(pixmap);
}



qint64 getTimeStanp()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    // 字符串格式化
    QString timestamp = dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    // 转换成时间戳
    qint64 epochTime = dateTime.toMSecsSinceEpoch();
    return epochTime;

}


void MainWindow::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees;                                     // 定义指针类型参数numDegrees用于获取滚轮转角
    numDegrees = event->angleDelta();                      // 获取滚轮转角
    int step = 0;                                          // 设置中间参数step用于将获取的数值转换成整数型
    if (!numDegrees.isNull())                              // 判断滚轮是否转动
    {
        step = numDegrees.y();                             // 将滚轮转动数值传给中间参数step
    }
    event->accept();                                       // 获取事件
    if (step > 0)                                          // 判断图像是放大还是缩小
    {
        scaleImage(getLabelUnderMouse(),0.05);                       // 打印放大后的图像尺寸
    }
    else
    {
         scaleImage(getLabelUnderMouse(),-0.05);                                // 打印缩小后的图像尺寸
    }

}




void myBilateralFilter(const cv::Mat &src, cv::Mat &dst, int d, double sigmaColor, double sigmaSpace)
{
    CV_Assert(src.depth() == CV_8U);
    sigmaColor/=2;
    sigmaSpace/=2;

    int channel = src.channels();
    int halfSize = d/2;
    double space_coefficient = -0.5 / (sigmaSpace * sigmaSpace);
    double color_coefficient = -0.5 / (sigmaColor * sigmaColor);

    dst.create(src.size(), src.type());

    // Create space weight matrix
    cv::Mat spaceWeights = cv::Mat(2 * halfSize + 1, 2 * halfSize + 1, CV_64F);
    for(int i = -halfSize; i <= halfSize; ++i) {
        for(int j = -halfSize; j <= halfSize; ++j) {
            double value = std::sqrt(i * i + j * j);
            spaceWeights.at<double>(i + halfSize, j + halfSize) = std::exp(value * value * space_coefficient);
        }
    }

    // Apply filter
    std::vector<cv::Mat> srcPlanes;
    cv::split(src, srcPlanes);
    std::vector<cv::Mat> dstPlanes(channel);

    for (int ch = 0; ch < channel; ++ch) {
        dstPlanes[ch].create(src.size(), CV_8UC1);  // Initialize destination planes
    }

    for(int ch = 0; ch < channel; ++ch) {
        for(int i = 0; i < src.rows; ++i) {
            for(int j = 0; j < src.cols; ++j) {
                double totalWeight = 0.0;
                double totalSum = 0.0;
                for(int dx = -halfSize; dx <= halfSize; ++dx) {
                    for(int dy = -halfSize; dy <= halfSize; ++dy) {
                        int x = i + dx;
                        int y = j + dy;
                        if(x >= 0 && x < src.rows && y >= 0 && y < src.cols) {
                            double colorDistance = static_cast<double>(srcPlanes[ch].at<uchar>(i, j) - srcPlanes[ch].at<uchar>(x, y));
                            double colorWeight = std::exp(colorDistance * colorDistance * color_coefficient);
                            double weight = spaceWeights.at<double>(dx + halfSize, dy + halfSize) * colorWeight;
                            totalWeight += weight;
                            totalSum += weight * static_cast<double>(srcPlanes[ch].at<uchar>(x, y));
                        }
                    }
                }
                if (totalWeight > 0) {
                    dstPlanes[ch].at<uchar>(i, j) = cv::saturate_cast<uchar>(totalSum / totalWeight);
                }
                else {
                    dstPlanes[ch].at<uchar>(i, j) = srcPlanes[ch].at<uchar>(i, j);  // Copy source value if total weight is zero
                }
            }
        }
    }
    cv::merge(dstPlanes, dst);
}


void myAdaptiveBilateralFilter(const cv::Mat &src, cv::Mat &dst, int d, double sigmaColor, double sigmaSpace)
{
    CV_Assert(src.depth() == CV_8U);
    sigmaColor/=2;
    sigmaSpace/=2;

    int channel = src.channels();
    int halfSize = d/2;
    double space_coefficient = -0.5 / (sigmaSpace * sigmaSpace);

    dst.create(src.size(), src.type());

    // Create space weight matrix
    cv::Mat spaceWeights = cv::Mat(2 * halfSize + 1, 2 * halfSize + 1, CV_64F);
    for(int i = -halfSize; i <= halfSize; ++i) {
        for(int j = -halfSize; j <= halfSize; ++j) {
            double value = std::sqrt(i * i + j * j);
            spaceWeights.at<double>(i + halfSize, j + halfSize) = std::exp(value * value * space_coefficient);
        }
    }

    // Calculate local variance
    cv::Mat localVariance;
    cv::boxFilter(src, localVariance, CV_32F, cv::Size(d, d));
    localVariance = localVariance.mul(localVariance) / (d * d);

    // Apply filter
    std::vector<cv::Mat> srcPlanes;
    cv::split(src, srcPlanes);
    std::vector<cv::Mat> dstPlanes(channel);

    for (int ch = 0; ch < channel; ++ch) {
        dstPlanes[ch].create(src.size(), CV_8UC1);  // Initialize destination planes
    }

    for(int ch = 0; ch < channel; ++ch) {
        for(int i = 0; i < src.rows; ++i) {
            for(int j = 0; j < src.cols; ++j) {
                double totalWeight = 0.0;
                double totalSum = 0.0;
                for(int dx = -halfSize; dx <= halfSize; ++dx) {
                    for(int dy = -halfSize; dy <= halfSize; ++dy) {
                        int x = i + dx;
                        int y = j + dy;
                        if(x >= 0 && x < src.rows && y >= 0 && y < src.cols) {
                            // Adjust sigmaColor based on local variance
                            double adjustedSigmaColor = sigmaColor * (0.75 + pow(0.5,(localVariance.at<float>(x, y))));
                            double color_coefficient = -0.5 / (adjustedSigmaColor * adjustedSigmaColor);

                            double colorDistance = static_cast<double>(srcPlanes[ch].at<uchar>(i, j) - srcPlanes[ch].at<uchar>(x, y));
                            double colorWeight = std::exp(colorDistance * colorDistance * color_coefficient);
                            double weight = spaceWeights.at<double>(dx + halfSize, dy + halfSize) * colorWeight;
                            totalWeight += weight;
                            totalSum += weight * static_cast<double>(srcPlanes[ch].at<uchar>(x, y));
                        }
                    }
                }
                if (totalWeight > 0) {
                    dstPlanes[ch].at<uchar>(i, j) = cv::saturate_cast<uchar>(totalSum / totalWeight);
                }
                else {
                    dstPlanes[ch].at<uchar>(i, j) = srcPlanes[ch].at<uchar>(i, j);  // Copy source value if total weight is zero
                }
            }
        }
    }
    cv::merge(dstPlanes, dst);
}




MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label->setMouseTracking(true);
    ui->label_2->setMouseTracking(true);
    ui->label_3->setMouseTracking(true);
    ui->label_4->setMouseTracking(true);

    this->centralWidget()->setMouseTracking(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::mouseMoveEvent(QMouseEvent *event)
{

    QMainWindow::mouseMoveEvent(event);
}

QLabel* MainWindow::getLabelUnderMouse()
{
    QPoint pos = mapFromGlobal(QCursor::pos());

    // Assuming 'label1', 'label2' and 'label3' are the objectNames of your labels in the UI designer
    QList<QLabel*> labels = {ui->label, ui->label_2, ui->label_3,ui->label_4};

    for(QLabel *label : labels) {
        if(label->geometry().contains(pos)) {
            return label;
        }
    }

    return nullptr;
}





cv::Mat qim2mat(QImage & image)
{
    cv::Mat mat;
        qDebug() << image.format();
        switch(image.format())
        {
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
            break;
        case QImage::Format_RGB888:
            mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
            break;
        case QImage::Format_Indexed8:
            mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
            break;
        default:
            qDebug()<<"BAD FILE!!";
            break;
        }
        return mat;


}
QImage mat2qim(cv::Mat & mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
        if(mat.type() == CV_8UC1)
        {
            QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
            // Set the color table (used to translate colour indexes to qRgb values)
            image.setColorCount(256);
            for(int i = 0; i < 256; i++)
            {
                image.setColor(i, qRgb(i, i, i));
            }
            // Copy input Mat
            uchar *pSrc = mat.data;
            for(int row = 0; row < mat.rows; row ++)
            {
                uchar *pDest = image.scanLine(row);
                memcpy(pDest, pSrc, mat.cols);
                pSrc += mat.step;
            }
            return image;
        }
        // 8-bits unsigned, NO. OF CHANNELS = 3
        else if(mat.type() == CV_8UC3)
        {
            // Copy input Mat
            const uchar *pSrc = (const uchar*)mat.data;
            // Create QImage with same dimensions as input Mat
            QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
            return image.rgbSwapped();
        }
        else if(mat.type() == CV_8UC4)
        {
            qDebug() << "CV_8UC4";
            // Copy input Mat
            const uchar *pSrc = (const uchar*)mat.data;
            // Create QImage with same dimensions as input Mat
            QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
            return image.copy();
        }
        else
        {
            qDebug() << "ERROR: Mat could not be converted to QImage.";
            return QImage();
        }


}


void MainWindow::showInLabel(QImage &img,QLabel *label)
{
    QPixmap pix=QPixmap::fromImage(img);

    int scalHeight = label->height();
    int scalWidth = pix.scaledToHeight(scalHeight).width();//根据label高度计算得到控件目标宽度

    if(label->width() < scalWidth)
    {//再次调整，确保图片能够在控件中完整显示
        scalWidth = label->width();
        scalHeight = pix.scaledToWidth(scalWidth).height();
    }

    label->setPixmap(pix.scaled(scalWidth,scalHeight,Qt::KeepAspectRatio));

    if(label==ui->label)        pix1.pix=pix;
    else if(label==ui->label_2) pix2.pix=pix;
    else if(label==ui->label_3) pix3.pix=pix;
    else if(label==ui->label_4) pix4.pix=pix;
    else qDebug()<<"illegal label";



}


void MainWindow::on_pushButton_clicked()
{
    QString filename=QFileDialog::getOpenFileName(this,tr("Open Image"),QDir::homePath(),tr("(*.jpg)\n(*.bmp)\n(*.png)"));   //打开图片文件，选择图片
       qDebug()<<"filename:"<<filename;
       QImage image=QImage(filename);   //图片初始化
       qDebug()<<"image:"<<image;
       if(!image.isNull())
       {
           ui->statusbar->showMessage(tr("Open Image Success!"),3000); //打开成功时显示的内容
       }
       else
       {
           ui->statusbar->showMessage(tr("Open Image Failed!"),3000);
           return;
       }
       ui->statusbar->showMessage(tr("Processing............."),3000);
       showInLabel(image,ui->label);
       pix1.toSave=image;
       cv::Mat temp=qim2mat(image),result1,result2,result3,toProcess;

       if(temp.type() == CV_8UC4){
           cv::cvtColor(temp, toProcess, cv::COLOR_BGRA2BGR);
           if(toProcess.type() != CV_8UC3){
               std::cerr << "Error converting image from CV_8UC4 to CV_8UC3" << std::endl;
           }
       }
       else{
           std::cerr << "Source image is not of type CV_8UC4" << std::endl;
           toProcess=temp;
       }


       qint64 start,end,duration1,duration2,duration3;

       start=getTimeStanp();
       myBilateralFilter(toProcess,result1,7,50,50);
       end=getTimeStanp();
       duration1=end-start;
       QImage toShow=mat2qim(result1);
       showInLabel(toShow,ui->label_2);
       pix2.toSave=toShow;

       start=getTimeStanp();
       myAdaptiveBilateralFilter(toProcess,result2,7,50,50);
       end=getTimeStanp();
       duration2=end-start;
       toShow=mat2qim(result2);
       showInLabel(toShow,ui->label_3);
       pix3.toSave=toShow;


       start=getTimeStanp();
       cv::bilateralFilter(toProcess,result3,7,50,50);
       end=getTimeStanp();
       duration3=end-start;
       toShow=mat2qim(result3);
       showInLabel(toShow,ui->label_4);
       pix4.toSave=toShow;


       ui->statusbar->showMessage("Time used(ms):   myBilateralFilter is"+QString::number(duration1)+
                                                    "， myAdaptiveBilateralFilter is"+QString::number(duration2)+
                                                     "， cv::bilateralFilter is"+QString::number(duration3)+
                                                    "   SSIM is:myBilateralFilter is"+QString::number(calculateSSIM(pix1.toSave,pix2.toSave))+
                                  "， myAdaptiveBilateralFilter is"+QString::number(calculateSSIM(pix1.toSave,pix3.toSave))+
                                   "， cv::bilateralFilter is"+QString::number(calculateSSIM(pix1.toSave,pix4.toSave))
                                  ,30000);
}




void MainWindow::on_pushButton_4_clicked()
{
    QString location = QFileDialog::getExistingDirectory(NULL,"caption",".");
    pix4.toSave.save(location+"//cvFilterSave.png","PNG");
    ui->statusbar->showMessage("Save success!",3000);

}





void MainWindow::on_pushButton_2_clicked()
{
    QString location = QFileDialog::getExistingDirectory(NULL,"caption",".");
    pix2.toSave.save(location+"//myFilterSave.png","PNG");
    ui->statusbar->showMessage("Save success!",3000);

}


void MainWindow::on_pushButton_3_clicked()
{
    QString location = QFileDialog::getExistingDirectory(NULL,"caption",".");
    pix3.toSave.save(location+"//myAdaptiveFilterSave.png","PNG");
    ui->statusbar->showMessage("Save success!",3000);
}




