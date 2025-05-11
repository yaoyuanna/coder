#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QImage>
#include"common.h"
#include"picinpic_window.h"
#include"savevideofilethread.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void SIG_open();
    void SIG_close();
public slots:
    void SLO_setVideoFrame(QImage img);
private slots:
    void on_pb_satrt_clicked();

    void on_pb_end_clicked();

    void on_pb_seturl_clicked();

    void on_cb_camera_stateChanged(int arg1);

    void on_cb_screen_checkStateChanged(const Qt::CheckState &arg1);

    void on_cb_voice_checkStateChanged(const Qt::CheckState &arg1);

    void on_cb_camera_checkStateChanged(const Qt::CheckState &arg1);

    void on_cb_screen_stateChanged(int arg1);

    void on_cb_voice_stateChanged(int arg1);

private:
    QString m_saveUrl;
    Ui::MainWindow *ui;
    picinpic_window* m_picinpic_window;
    SaveVideoFileThread* m_VideoFileSaver;
};
#endif // MAINWINDOW_H
