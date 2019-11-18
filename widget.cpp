#include "widget.h"
#include "ui_widget.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/qserialport.h>
#include "SerialReadWriter.h"
#include "global.h"
#include <QDebug>
#include <QDate>
#include <QMessageBox>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("gz test tool"));
    this->setFixedSize( W_MAIN_WIDGET, H_MAIN_WIDGET );

    QStringList serialPortNameList = getSerialNameList();
    ui->serialPortNameComboBox->addItems(serialPortNameList);

    createConnect();
}

Widget::~Widget()
{
    delete ui;
}

QStringList Widget::getSerialNameList() {

    auto serialPortInfoList = QSerialPortInfo::availablePorts();
    QStringList l;
    for (auto &s:serialPortInfoList) {
        l.append(s.portName());
    }
    return l;
}

void Widget::on_showDetailBtn_toggled(bool checked)
{
    if(checked)
    {
        this->setFixedSize( W_MAIN_WIDGET, H_EXT_MAIN_WIDGET);
        ui->textBrowser_ExecInfo->setVisible(checked);
        ui->showDetailBtn->setText(tr("隐藏详细信息"));
    }
    else
    {
        this->setFixedSize( W_MAIN_WIDGET, H_MAIN_WIDGET);
        ui->textBrowser_ExecInfo->setVisible(false);
        ui->showDetailBtn->setText(tr("显示详细信息"));
    }
}

void Widget::createConnect()
{
    connect(this, &Widget::serialStateChanged, [this](bool isOpen) {
//    setOpenButtonText(isOpen);
    QString stateText;
    if (isOpen) {
        stateText = QString("串口打开成功，%1").arg(_readWriter->settingsText());
    } else {
        stateText = QString("串口关闭");
    }

    logMsg(stateText);
});
}

void Widget::logMsg(const QString &msg)
{
    QString str = QDateTime::currentDateTime().toString(QString("[yyyy-MM-dd HH:mm:ss] "));
    str.append(msg);
    ui->textBrowser_ExecInfo->append(str);
}


bool Widget::openReadWriter()
{
    bool result;
    auto settings = new SerialSettings();
    settings->name = ui->serialPortNameComboBox->currentText();
    settings->baudRate = QSerialPort::Baud115200;
    settings->dataBits = QSerialPort::Data8;
    settings->stopBits = QSerialPort::OneStop;
    settings->parity = QSerialPort::NoParity;
    settings->flowControl = QSerialPort::NoFlowControl;

    auto readWriter = new SerialReadWriter(this);
    readWriter->setSerialSettings(*settings);

    qDebug() << settings->name << settings->baudRate << settings->dataBits << settings->stopBits << settings->parity;
    result = readWriter->open();
    if (!result) {
        showWarning(tr("消息"), tr("串口被占用或者不存在"));
        return result;
    }
    _readWriter = readWriter;
    connect(_readWriter, &AbstractReadWriter::readyRead,
            this, &Widget::readData);

    emit serialStateChanged(result);

    return result;
}

void Widget::closeReadWriter()
{
    if (_readWriter != nullptr) {
        _readWriter->close();
        delete _readWriter;
        _readWriter = nullptr;
    }
    emit serialStateChanged(false);
}

void Widget::readData() {
    auto data = _readWriter->readAll();
    if (!data.isEmpty()) {
        qDebug() << QString::fromLocal8Bit(data);

        QString s( QString::fromLocal8Bit(data) );
        logMsg(s);
    }
}

qint64 Widget::writeData(const QByteArray &data)
{
    if (!data.isEmpty() && isReadWriterConnected()) {
        auto count = _readWriter->write(data);
        return count;
    }
    return 0;
}

bool Widget::isReadWriterConnected() {
    return _readWriter != nullptr && _readWriter->isConnected();
}

void Widget::on_startBtn_clicked()
{
    qDebug() << "clicked";
    if( openReadWriter() ) {
        qDebug("open success");
        ui->serialPortNameComboBox->setDisabled(true);
//        ui->startBtn->setDisabled(true);
    }else{
        qDebug("open failed");
        closeReadWriter();
        ui->serialPortNameComboBox->setDisabled(false);
    }
}
