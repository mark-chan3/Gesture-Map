#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebView>
#include <QLineEdit>
#include <QWebHistory>
#include <QListWidget>
#include <QDebug>
#include <QWebFrame>
#include <QDir>
#include<QTimer>
#include"paj.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mode=1;
    loadw=false;
    this->paintstatus(":/qms");
    ui->operation->setGeometry(770,100,191,191);
    qpr=new QProgressBar(ui->statusBar);
    qpr->setFixedWidth(200);
    qpr->setValue(progress);
    qpr->setAlignment(Qt::AlignTrailing);

    JSBridge = new bridge(this);
//    QWebChannel *channel = new QWebChannel(this);
//    channel->registerObject("window",(QObject*)JSBridge);
//    ui->MapWidget->page()->setWebChannel(channel);
//    ui->MapWidget->page()->load(QUrl("qrc:/Baidu_JS/BDMap.html"));
    connect(JSBridge,SIGNAL(DisplayPoint(QString,QString)),this,SLOT(DisplaySlot(QString,QString)));
    mainWebView = new QWebView(this);
    mainWebView->setParent(ui->webframe);
    connect(mainWebView, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    connect(mainWebView, SIGNAL(loadProgress(int)), this, SLOT(setProgress(int)));
    connect(mainWebView, SIGNAL(loadFinished(bool)), this, SLOT(finishLoading(bool)));
    connect(mainWebView, SIGNAL(titleChanged(QString)), this, SLOT(adjustTitle()));
    mainWebView->load(QUrl("qrc:/Baidu_JS/BDMap.html"));

    connect(mainWebView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(slotPopulateJavaScriptWindowObject()));
    mainWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    //timer
    timup=new QTimer();
    connect(timup,&QTimer::timeout,this,[=](){
        movemap("0","10");
    });
    timdown=new QTimer();
    connect(timdown,&QTimer::timeout,this,[=](){
        movemap("0","-10");
    });
    timleft=new QTimer();
    connect(timleft,&QTimer::timeout,this,[=](){
        movemap("10","0");
    });
    timright=new QTimer();
    connect(timright,&QTimer::timeout,this,[=](){
        movemap("-10","0");
    });
    //small

    //paj
    paj *pa;
    pa=new paj();
    timges=new QTimer();


    connect(timges,&QTimer::timeout,[=](){
    pa->gesture_mask();

    dealbuf(pa->inter);
    });


    //paj
}

MainWindow::~MainWindow()
{
    delete ui;
}
//pai
void MainWindow::dealbuf(short  buf0)
{
    if(buf0==1&&loadw )
    {
        this->on_btn_up_clicked();
        qDebug()<<buf0;
    }
    if(buf0==2&&loadw )
    {
        this->on_btn_down_clicked();
        qDebug()<<buf0;
    }
    if(buf0==4&&loadw )
    {
        this->on_btn_left_clicked();
        qDebug()<<buf0;
    }
    if(buf0==8&&loadw )
    {
        this->on_btn_right_clicked();
        qDebug()<<buf0;
    }
    if(buf0==10&&loadw )
    {
        this->on_btn_big_clicked();
        qDebug()<<buf0;
    }
    if(buf0==20&&loadw )
    {
        this->on_btn_small_clicked();
        qDebug()<<buf0;
    }
}

void MainWindow::DisplaySlot(QString lng, QString lat)
{
    ui->lineEdit_RcvMsg->setText(lng+","+lat);
}

void MainWindow::getCoordinate(const QString &l)
{
    ui->lineEdit_RcvMsg->setText(l);
}

//void MainWindow::DisplaySlotZoom(QString zoom)
//{

//}
void MainWindow::slotPopulateJavaScriptWindowObject()
{
    mainWebView->page()->mainFrame()->addToJavaScriptWindowObject("qtwebkit", this);
}


//操作
void MainWindow::movemap(QString ln,QString la)
{
    QString ln0 =ln;
    QString la0 = la;
    mainWebView->page()->mainFrame()->evaluateJavaScript(QString("MapMove(%1,%2)").arg(ln0).arg(la0));
}

//bigbutton
void MainWindow::on_btn_big_clicked()
{
    //ui->MapWidget->page()->runJavaScript(QString("SetZoomIn()"));
   mainWebView->page()->mainFrame()->evaluateJavaScript(QString("SetZoomIn()"));
   this->paintstatus(":/big2.jpg");
   char buf[2];
   buf[0]='q';
   buf[1]='1';
   qDebug()<<QString("buf=%1%2").arg(buf[0]).arg(buf[1]);

}
//small
void MainWindow::on_btn_small_clicked()
{
    mainWebView->page()->mainFrame()->evaluateJavaScript(QString("SetZoomOut()"));
    this->paintstatus(":/small3.jpg");
}
//up
void MainWindow::on_btn_up_clicked()
{
    this->paintstatus(":/up.jpg");
    if(mode==1)
    {
        movemap("0","20");
    }
    else
    {
        if(downflag)
        {
            downflag=false;
            timdown->stop();
        }
        else
        {
            timup->start(1);
            upflag=true;
        }

    }
}
//down
void MainWindow::on_btn_down_clicked()
{
    this->paintstatus(":/down.jpg");
    if(mode==1)
    {
        movemap("0","-20");
    }
    else
    {
        if(upflag)
        {
            upflag=false;
            timup->stop();
        }
        else
        {
            timdown->start(1);
            downflag=true;
        }

    }
}
//left
void MainWindow::on_btn_left_clicked()
{
    this->paintstatus(":/left.jpg");

    if(mode==1)
    {
        movemap("20","0");
    }
    else
    {
        if(rightflag)
        {
            rightflag=false;
            timright->stop();
        }
        else
        {
            timleft->start(1);
            leftflag=true;
        }

    }
}
//right
void MainWindow::on_btn_right_clicked()
{
    this->paintstatus(":/right.jpg");

    if(mode==1)
    {
        movemap("-20","0");
    }
    else
    {
        if(leftflag)
        {
            leftflag=false;
            timleft->stop();
        }
        else
        {
            timright->start(1);
            rightflag=true;
        }

    }

}
//send
void MainWindow::on_pushButton_clicked()
{
    QString context = ui->lineEdit_SendMsg->text();
    if(!context.contains(','))
    {
        qDebug()<<"输入格式错误";        //输入格式 经度+纬度，中间以英文逗号‘,’隔开
        return;
    }
    QString lng = context.split(',').at(0);
    QString lat = context.split(',').at(1);
    mainWebView->page()->mainFrame()->evaluateJavaScript(QString("SetPoint(%1,%2)").arg(lng).arg(lat));
    //ui->MapWidget->page()->runJavaScript(QString("SetPoint(%1,%2)").arg(lng).arg(lat));
}
void MainWindow::loadStarted()
{
    qDebug()<<"start:"<<mainWebView->title()<<endl;
}

void MainWindow::setProgress(int p)
{
    progress = p;
    adjustTitle();
    qpr->setValue(progress);
}
/*
 * \brief MainWindow::adjustTitle
 * 加载网页标题显示
 */
void MainWindow::adjustTitle()
{
    if( progress <= 0 || progress >= 100 )
    {
        setWindowTitle(mainWebView->title());
    }
    else
    {
        ui->statusBar->showMessage(QString("%1 (%2%)").arg(mainWebView->title()).arg(progress));
        setWindowTitle(QString("%1 (%2%)").arg(mainWebView->title()).arg(progress));
    }
}

void MainWindow::finishLoading(bool finished)
{
    if( finished )
    {
        progress = 100;
        setWindowTitle(mainWebView->title());
        ui->statusBar->showMessage(QString("%1").arg(mainWebView->title()));
        qDebug()<<"load finished";
        loadw=true;


        QString  lng;
        QString  lat;
        mainWebView->page()->mainFrame()->evaluateJavaScript(QString("getpoint()"));

    }
    else
    {
        setWindowTitle("web page loading error");
    }
}

void MainWindow::on_btn_mode_clicked()
{
    if(mode==1)
    {
        ui->btn_mode->setText("static");
        mode=0;
    }
    else{
        ui->btn_mode->setText("dynamic");
        mode=1;
    }

}
void MainWindow::paintstatus(QString q)
{
    QPixmap pix;
    pix.load(q);
    pix=pix.scaled(ui->operation->width(),ui->operation->height());
    ui->operation->setPixmap(pix);
}

void MainWindow::on_gesture_clicked()
{
    if(ui->gesture->text()=="ges_start")
    {
        timges->start(20);
        ui->gesture->setText("ges_end");
    }
    else
    {
        timges->stop();
        ui->gesture->setText("ges_start");
    }
}
