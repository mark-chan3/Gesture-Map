#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QUrl>
#include<QtWebKitWidgets>
#include<QFile>
#include<QProgressBar>
#include<QProcess>
#include "bridge.h"
namespace Ui {
class MainWindow;
}
class QWebView;
class QLineEdit;
class QListWidget;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void dealbuf(short  buf0);
public slots:
    void DisplaySlot(QString lng,QString lat);
    void on_pushButton_clicked();
    void on_btn_big_clicked();
    void on_btn_small_clicked();
    void on_btn_up_clicked();
    void on_btn_down_clicked();
    void on_btn_left_clicked();
    void on_btn_right_clicked();
    void loadStarted();
    void setProgress(int);
    void adjustTitle();
    void finishLoading(bool);
    //void DisplaySlotZoom(QString zoom);
    void getCoordinate(const QString &l);
    void movemap(QString ln,QString la);
    void paintstatus(QString q);
protected slots:
    void slotPopulateJavaScriptWindowObject();
private slots:
    void on_btn_mode_clicked();

    void on_gesture_clicked();

private:
    Ui::MainWindow *ui;
    bridge *JSBridge;
    QWebView *mainWebView;
    QLineEdit *mainLineEdit;
    QListWidget *historyList;
    int progress;
    int mode;
    bool upflag;
    bool downflag;
    bool leftflag;
    bool rightflag;
    QTimer * timup;
    QTimer * timdown;
    QTimer * timleft;
    QTimer * timright;
    QTimer * timbig;
    QTimer * timsmall;
    bool loadw;
    QTimer * timges;
    QProgressBar *qpr;
};

#endif // MAINWINDOW_H
