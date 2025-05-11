#ifndef PICINPIC_WINDOW_H
#define PICINPIC_WINDOW_H

#include <QWidget>

namespace Ui {
class picinpic_window;
}

class picinpic_window : public QWidget
{
    Q_OBJECT

public:
    explicit picinpic_window(QWidget *parent = nullptr);
    ~picinpic_window();
public slots:
    void slot_setImage(QImage);
private:
    Ui::picinpic_window *ui;
};

#endif // PICINPIC_WINDOW_H
