#include "main_window.h"
#include "ui_main_window.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QLabel>

#include <qwt_scale_div.h>
#include <qwt_date_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>
#include <qwt_date_scale_engine.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
    updateTimer->start(10000);
    onGetCurrentTemperature();

    connect(ui->btn_getTemperatureData, &QPushButton::clicked, this, &MainWindow::on_btn_getTemperatureData_clicked);
    connect(ui->btn_getPeriodTemperatureData, &QPushButton::clicked, this, &MainWindow::on_btn_getPeriodTemperatureData_clicked);

    temperaturePlot = new QwtPlot(this);
    temperaturePlot->setTitle("Temperature Data");
    temperaturePlot->setCanvasBackground(Qt::white);
    temperaturePlot->setAxisTitle(QwtPlot::xBottom, "Timestamp");
    temperaturePlot->setAxisTitle(QwtPlot::yLeft, "Temperature");

    temperatureCurve = new QwtPlotCurve("Temperature");
    temperatureCurve->attach(temperaturePlot);
    ui->verticalLayout->addWidget(temperaturePlot);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onGetCurrentTemperature()
{
    QNetworkRequest request(QUrl("http://localhost:3000/getCurrentTemperature"));
    networkManager.get(request);
}
void MainWindow::onTimerTimeout()
{
    onGetCurrentTemperature();
}

void MainWindow::onGetTemperatureData(const QString &selectedDate)
{
    QNetworkRequest request(QUrl("http://localhost:3000/getTemperatureData?selectedDate=" + selectedDate));
    networkManager.get(request);
}
void MainWindow::on_btn_getTemperatureData_clicked()
{
    QDate selectedDate = ui->dateEdit->date();
    QString selectedDateString = selectedDate.toString("yyyy-MM-dd");
    onGetTemperatureData(selectedDateString);
}

void MainWindow::onGetPeriodTemperatureData(const QString &startDate, const QString &endDate)
{
    QNetworkRequest request(QUrl("http://localhost:3000/getPeriodTemperatureData?selectedStartDate=" + startDate + "&selectedEndDate=" + endDate));
    networkManager.get(request);
}
void MainWindow::on_btn_getPeriodTemperatureData_clicked()
{
    QDate selectedStartDate = ui->dateTimeEdit_startDate->date();
    QDate selectedEndDate = ui->dateTimeEdit_endDate->date();
    QTime selectedStartTime = ui->dateTimeEdit_startDate->time();
    QTime selectedEndTime = ui->dateTimeEdit_endDate->time();

    QString selectedStartDateString = selectedStartDate.toString("yyyy-MM-dd ") + selectedStartTime.toString("HH:mm");
    QString selectedEndDateString = selectedEndDate.toString("yyyy-MM-dd ") + selectedEndTime.toString("HH:mm");
    qDebug() << selectedStartDateString << " " << selectedEndDateString;
    onGetPeriodTemperatureData(selectedStartDateString, selectedEndDateString);
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if (jsonDoc.isObject())
        {
            QJsonObject jsonObject = jsonDoc.object();
            if (jsonObject.contains("currentTemperature"))
            {
                double currentTemperature = jsonObject["currentTemperature"].toDouble();
                ui->currentTemperatureLabel->setText("Current Temperature: " + QString::number(currentTemperature));
            }
            else if (jsonObject.contains("temperatureData") && jsonObject.contains("chartLabels") && jsonObject.contains("chartData"))
            {
                temperatureCurve->setData(nullptr);

                QVector<QPointF> dataPoints;
                QJsonArray temperatureData = jsonObject["temperatureData"].toArray();
                for (int i = 0; i < temperatureData.size(); ++i) {
                    //qDebug() << temperatureData[i].toObject()["timestamp"].toString();
                    QJsonObject obj = temperatureData[i].toObject();
                    QDateTime dateTime = QDateTime::fromString(obj["timestamp"].toString(), "yyyy/MM/dd HH:mm:ss");
                    double temperature = obj["temperature"].toDouble();
                    dataPoints.append(QPointF(dateTime.toMSecsSinceEpoch(), temperature));
                }

                QwtPointSeriesData *seriesData = new QwtPointSeriesData(dataPoints);
                temperatureCurve->setData(seriesData);

                QwtText xAxisTitle("Date/Time");
                temperaturePlot->setAxisTitle(QwtPlot::xBottom, xAxisTitle);
                temperaturePlot->setAxisScaleDraw(QwtPlot::xBottom, new QwtDateScaleDraw(Qt::LocalTime));
                QwtScaleEngine *scaleEngine = new QwtLinearScaleEngine();
                temperaturePlot->setAxisScaleEngine(QwtPlot::xBottom, scaleEngine);

                temperaturePlot->replot();
            }
        }

        reply->deleteLater();
    }
    else
    {
        qDebug() << "Network error:" << reply->errorString();
    }
}