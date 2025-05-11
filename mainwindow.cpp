#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_picinpic_window=new picinpic_window();
    m_VideoFileSaver=new SaveVideoFileThread();
    m_picinpic_window->move(0,0);
    m_picinpic_window->show();
    //将开始与结束信号绑定至视频保存线程类中的画面读取类的开始与绑定槽函数
    connect(this,&MainWindow::SIG_open, m_VideoFileSaver,&SaveVideoFileThread::slot_openVideo);
    connect(this,&MainWindow::SIG_close,m_VideoFileSaver,&SaveVideoFileThread::slot_closeVideo);
    //接收来自视频保存线程类的转发信号设置画中画与预览画面
    connect(m_VideoFileSaver,&SaveVideoFileThread::SIG_sendVideoFrame,this,&MainWindow::SLO_setVideoFrame);
    connect(m_VideoFileSaver,&SaveVideoFileThread::SIG_sendVideoFrame,m_picinpic_window,&picinpic_window::slot_setImage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//预览画面设置图像槽函数
void MainWindow::SLO_setVideoFrame(QImage img)
{
    qDebug()<<"set frame--------------------------------";
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

void MainWindow::on_pb_satrt_clicked()
{
    this->showMinimized();
    m_picinpic_window->show();
    av_infor infor;
    infor.filename=m_saveUrl;
    infor.frame_rate=FRAME_RATE;
    infor.have_audio=false;
    infor.have_camera=false;
    infor.have_screen=true;
    infor.videoBiteRate=VIDEOBITRATE;
    QScreen *src =QApplication::primaryScreen();
    QRect rect = src->geometry();
    infor.w=rect.width();
    infor.h=rect.height();
    m_VideoFileSaver->SLO_setInfor(infor);
    emit SIG_open();
}


void MainWindow::on_pb_end_clicked()
{
    emit SIG_close();
}


void MainWindow::on_pb_seturl_clicked()
{
    m_saveUrl=ui->le_editurl->text();
    m_saveUrl=m_saveUrl.replace("/","\\");
}




void MainWindow::on_cb_camera_stateChanged(int arg1)
{
    if(arg1==0)m_picinpic_window->hide();
    if(arg1==2)m_picinpic_window->show();
}


void MainWindow::on_cb_screen_stateChanged(int arg1)
{
    qDebug()<<arg1;
}


void MainWindow::on_cb_voice_stateChanged(int arg1)
{
    qDebug()<<arg1;
}

