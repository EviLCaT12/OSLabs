#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QTimer>
#include <QPushButton>
#include <QDateEdit>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

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

private slots:
    void onGetCurrentTemperature();
    void onTimerTimeout();
    void onGetTemperatureData(const QString &selectedDate);
    void onGetPeriodTemperatureData(const QString &startDate, const QString &endDate);
    void onNetworkReply(QNetworkReply *reply);
    void on_btn_getTemperatureData_clicked();
    void on_btn_getPeriodTemperatureData_clicked();

private:
    QNetworkAccessManager networkManager;
    QTimer *updateTimer;
    Ui::MainWindow *ui;
    QwtPlot *temperaturePlot;
    QwtPlotCurve *temperatureCurve;
};
#endif // MAINWINDOW_H