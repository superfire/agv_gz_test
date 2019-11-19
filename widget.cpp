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

    QString header = "gz_test com";
 #if 0
    gzSendBuffList[IDX_DEBUG_COM] =  header + "uart_debug";
    gzSendBuffList[IDX_ETHERNET] = header + "ethernet";
    gzSendBuffList[IDX_485] = header + "485";
    gzSendBuffList[IDX_CAN] = header + "can";
    gzSendBuffList[IDX_PMBUS] = header + "pmbus";

    gzAckBuffList[IDX_DEBUG_COM] = header + "ack uart_debug";
    gzAckBuffList[IDX_ETHERNET] = header + "ack ethernet";
    gzAckBuffList[IDX_485] = header + "ack 485";
    gzAckBuffList[IDX_CAN] = header + "ack can";
    gzAckBuffList[IDX_PMBUS] = header + "ack pmbus";
#endif

    gzSendBuffList << header + "uart_debug";
    gzSendBuffList << header + "ethernet";
    gzSendBuffList << header + "485";
    gzSendBuffList << header + "can";
    gzSendBuffList << header + "pmbus";

    gzAckBuffList << header + "ack uart_debug";
    gzAckBuffList << header + "ack ethernet";
    gzAckBuffList << header + "ack 485";
    gzAckBuffList << header + "ack can";
    gzAckBuffList << header + "ack pmbus";

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

    connect(this, &Widget::readBytesChanged, this, &Widget::dealWithRecData);
    connect(this, &Widget::writeBytesChanged, this, &Widget::dealWithSendData);
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
        recBuff = data;
        qDebug() << QString::fromLocal8Bit(data);

        QString s( QString::fromLocal8Bit(data) );
        logMsg(s);
        emit readBytesChanged(receiveCount);
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
        connect(_readWriter, &AbstractReadWriter::readyRead,
                this, &Widget::readData);

//        ui->startBtn->setDisabled(true);
    }else{
        qDebug("open failed");
        closeReadWriter();
        ui->serialPortNameComboBox->setDisabled(false);
        disconnect(_readWriter, &AbstractReadWriter::readyRead,
                this, &Widget::readData);
    }
}

void Widget::dealWithRecData(qint64 bytes)
{
    QString tmpBuff;
    for(int i=0; i < bytes; i++) {
        tmpBuff.append( recBuff.at(i) );
    }

    qDebug() << "len:" << bytes << " data:" << recBuff;

    if( 0 == tmpBuff.compare( gzAckBuffList[IDX_DEBUG_COM] ) ) {
        ;
    }else if( 0 == tmpBuff.compare( gzAckBuffList[IDX_ETHERNET] ) ) {
        ;
    }else if( 0 == tmpBuff.compare( gzAckBuffList[IDX_485] ) ) {
        ;
    }else if( 0 == tmpBuff.compare( gzAckBuffList[IDX_CAN] ) ) {
        ;
    }else if( 0 == tmpBuff.compare( gzAckBuffList[IDX_PMBUS] ) ) {
        ;
    }
}

void Widget::dealWithSendData(qint64 bytes)
{

}
