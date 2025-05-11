#include "picinpic_window.h"
#include "ui_picinpic_window.h"

picinpic_window::picinpic_window(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::picinpic_window)
{
    ui->setupUi(this);
    this->setWindowFlag(Qt::FramelessWindowHint);
    this->setWindowFlag(Qt::WindowStaysOnTopHint);
}

picinpic_window::~picinpic_window()
{
    delete ui;
}

//画中画界面设置图像槽函数
void picinpic_window::slot_setImage(QImage img)
{
    // 获取 QLabel 的大小
    QSize labelSize = ui->lb_show->size();
    // 获取图片的大小
    QSize imageSize = img.size();

    // 计算水平和垂直方向的缩放比例
    qreal scaleX = (qreal)labelSize.width() / imageSize.width();
    qreal scaleY = (qreal)labelSize.height() / imageSize.height();

    // 选取较小的缩放比例，以保证等比例缩放且不超出 QLabel
    qreal scale = qMin(scaleX, scaleY);

    // 根据计算出的缩放比例对图片进行缩放，保持宽高比
    QImage scaledImage = img.scaled(imageSize * scale, Qt::KeepAspectRatio);

    // 将缩放后的 QImage 转换为 QPixmap
    QPixmap pixmap = QPixmap::fromImage(scaledImage);

    // 设置 QLabel 的对齐方式为居中对齐
    ui->lb_show->setAlignment(Qt::AlignCenter);
    // 关闭 QLabel 自动缩放内容的功能
    ui->lb_show->setScaledContents(false);
    // 在 QLabel 上显示缩放后的图片
    ui->lb_show->setPixmap(pixmap);
}
