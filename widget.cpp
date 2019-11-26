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
#include <QKeyEvent>
#include <QTimer>
#include <QAction>
#include <QMenu>


void Delay_MSec_Suspend(unsigned int msec)
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < _Timer );
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , comTest(new ComTest)
    , testProgressDlg(new MyProgressDlg(this))
{
    ui->setupUi(this);
    this->setWindowTitle(tr("agv工装测试软件"));
    this->setFixedSize( W_MAIN_WIDGET, H_MAIN_WIDGET );

    QStringList serialPortNameList = getSerialNameList();
    ui->serialPortNameComboBox->addItems(serialPortNameList);

    // 测试相关
    qDebug() << "test part num: " << comTest->m_testItemsNum;
//    comTest->m_testItemsNum = 4;

    // 进度条
    testProgressDlg->setMaxNum(100);
    testProgressDlg->setPartNum( comTest->m_testItemsNum );
    testProgressDlg->reset();

    createConnect();
}

Widget::~Widget()
{
    delete ui;
    delete comTest;
    delete testProgressDlg;
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
        stateText = QString("串口关闭\r\n");
    }
    logMsg(stateText);
});

    connect(this, &Widget::readBytesChanged, this, &Widget::dealWithRecData);
    connect(this, &Widget::writeBytesChanged, this, &Widget::dealWithSendData);
    connect(comTest, SIGNAL(sendData(QByteArray)), this, SLOT(readToSend(QByteArray)));
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
//        showWarning(tr("消息"), tr("串口被占用或者不存在"));
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
        qDebug("close");
        delete _readWriter;
        _readWriter = nullptr;
    }
    emit serialStateChanged(false);
}

void Widget::readData() {
    auto data = _readWriter->readAll();
    if (!data.isEmpty()) {
        recBuff = data;
        receiveCount = data.length();

        QString s( QString::fromLocal8Bit(data) );
        qDebug() << s;
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
        startTest();
    }
    else
    {
        qDebug("open failed");
        closeReadWriter();
        ui->serialPortNameComboBox->setDisabled(false);
        disconnect(_readWriter, &AbstractReadWriter::readyRead,
                this, &Widget::readData);

        QMessageBox::warning(this, "端口打开失败", "请检查接口是否被占用", u8"退出");
//        showWarning(tr("消息"), tr("串口被占用或者不存在"));
    }
}

void Widget::startTest(void)
{
        testProgressDlg->setMaxNum(100);
        testProgressDlg->move(this->x() + this->width()/2 - testProgressDlg->width()/2, this->y() + this->height()/2 - testProgressDlg->height()/2 + 10);
        testProgressDlg->show();

        connect(comTest, SIGNAL(progress(int, int)), testProgressDlg, SLOT(showProgress(int, int)));
        connect(comTest, SIGNAL(logInfo(const QString &)), this, SLOT(logMsg(const QString &)));

        int ret = comTest->Test();
        // 进度条处理
        testProgressDlg->reset();
        // 串口处理
        closeReadWriter();
        ui->serialPortNameComboBox->setDisabled(false);

        if(ret == ComTest::GZ_END_SUCCESS){
            QMessageBox::information(this, "测试结果", "测试通过", u8"退出");
        }else if(ret == ComTest::GZ_END_FAILED){
            QMessageBox::warning(this, "测试未通过", comTest->m_result_info, u8"退出");
        }else if(ret == ComTest::GZ_END_COM_TIMEOUT){
//            QMessageBox::warning(this, "通信失败", "请检查通信连接", u8"退出");
            showError("通信失败", "请检查通信连接");
        }

        // 测试对象处理
        comTest->Reset();

        disconnect(comTest, SIGNAL(progress(int, int)), testProgressDlg, SLOT(showProgress(int, int)));
        disconnect(comTest, SIGNAL(logInfo(const QString &)), this, SLOT(logMsg(const QString &)));
}

void Widget::dealWithRecData(qint64 bytes)
{
    QString tmpBuff;
    for(int i=0; i < bytes; i++) {
        tmpBuff.append( recBuff.at(i) );
    }

    // 处理返回的结果
    comTest->DealWithAck( tmpBuff );

    qDebug() << "len:" << bytes << " data:" << tmpBuff;
//    qDebug() << "debug_com buff:" << gzAckBuffList[IDX_DEBUG_COM];
}

void Widget::readToSend(QByteArray data)
{
    sendBuff.clear();
    sendBuff = data;
    emit writeBytesChanged( sendBuff.length() );
}

void Widget::dealWithSendData(qint64 bytes)
{
    auto count = writeData( sendBuff );
    qDebug() << "send data len: " << count << "actual send data len:" << count;
}

MyProgressDlg::MyProgressDlg(QWidget *parent)
{
//    progressTimer = new QTimer();
//    QObject::connect(progressTimer,&QTimer::timeout,[&](){
//        setValue( progressCnt++ );
//    });

    setWindowTitle("工装测试");
    setMinimum(0);
    setMaximum( 100 );
    setValue(0);
    setLabelText("测试中...");
    setWindowFlag( Qt::FramelessWindowHint );
    setModal(true);
    QPushButton *btn = nullptr;
    setCancelButton(btn);
    reset(); // 必须添加，否则new完后，会自动弹出
    setAutoClose(true);
}

MyProgressDlg::~MyProgressDlg()
{
}

void MyProgressDlg::keyPressEvent(QKeyEvent *event)
{
    qDebug("widget, esc pressed");
    switch (event->key())
    {
    case Qt::Key_Escape:
        qDebug("widget, esc pressed");
        break;
    default:
        QDialog::keyPressEvent(event);
        break;
    }
}

ComTest::ComTest(void)
{
    QString header = "gz_test com";
    m_gzTestBuffList << header + " uart_debug";
    m_gzTestBuffList << header + " ethernet";
    m_gzTestBuffList << header + " 485";
    m_gzTestBuffList << header + " can";
    m_gzTestBuffList << header + " pmbus";

    m_ackMap.insert("uart_debug", TEST_IDX_DEBUG_COM);
    m_ackMap.insert("ethernet",   TEST_IDX_ETHERNET);
    m_ackMap.insert("485",        TEST_IDX_485);
    m_ackMap.insert("can",        TEST_IDX_CAN);
    m_ackMap.insert("pmbus",      TEST_IDX_PMBUS);

    m_stepMap.insert("ack uart_debug",  GZ_ACK_DEBUG_COM_SUCCESS);
    m_stepMap.insert("nack uart_debug", GZ_ACK_DEBUG_COM_FAILED);
    m_stepMap.insert("ack ethernet",    GZ_ACK_ETHERNET_SUCCESS);
    m_stepMap.insert("nack ethernet",   GZ_ACK_ETHERNET_FAILED);
    m_stepMap.insert("ack 485",         GZ_ACK_485_SUCCESS);
    m_stepMap.insert("nack 485",        GZ_ACK_485_FAILED);
    m_stepMap.insert("ack can",         GZ_ACK_CAN_SUCCESS);
    m_stepMap.insert("nack can",        GZ_ACK_CAN_FAILED);
    m_stepMap.insert("ack pmbus",       GZ_ACK_PMBUS_SUCCESS);
    m_stepMap.insert("nack pmbus",      GZ_ACK_PMBUS_FAILED);
}

ComTest::~ComTest(void)
{

}

int ComTest::Test(void)
{
    quint32 timeout = 0;
    eTestStepDef step = GZ_STEP_DEBUG_COM;
    uint32_t progress_cnt = 0;
    uint32_t progress_part = 0;
    QString log;
    uint8_t mask = 0;

    // 发送查询设备是否在线
    while( true )
    {
restart:
        switch( step ) {
        case GZ_STEP_DEBUG_COM:
            // 测试调试用串口
            emit sendData(m_gzTestBuffList[ TEST_IDX_DEBUG_COM ].toLatin1());
            qDebug() << "debug com test";
            break;
        case GZ_STEP_ETHERNET:
            // 测试以太网
            emit sendData(m_gzTestBuffList[ TEST_IDX_ETHERNET ].toLatin1());
            break;
        case GZ_STEP_485:
            // 测试485
            emit sendData(m_gzTestBuffList[ TEST_IDX_485 ].toLatin1());
            break;
        case GZ_STEP_CAN:
            // 测试can
            emit sendData(m_gzTestBuffList[ TEST_IDX_CAN ].toLatin1());
            break;
        case GZ_STEP_PMBUS:
            // 测试pmbus
            emit sendData(m_gzTestBuffList[ TEST_IDX_PMBUS ].toLatin1());
            break;
        default:
            break;
        }
        m_ack = GZ_ACK_NONE;

        // waiting
        while( true )
        {
            switch( m_ack ) {
            case GZ_ACK_NONE:
                // todo...
                Delay_MSec_Suspend(200);
                timeout ++;
                progress_cnt++;
                emit progress(progress_part, progress_cnt);
                qDebug() << "timeout: " << timeout;
                if( timeout > 15 ) // 3s超时
                {
                    // timeout, debug comm has problem
                    return GZ_END_COM_TIMEOUT;
                }
                break;
            case GZ_ACK_DEBUG_COM_SUCCESS:
                step = GZ_STEP_ETHERNET;
                timeout = 0;
                progress_part = 1;
                progress_cnt = 0;
                log = "debug com success";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\n调试串口  \t正常"));
                goto restart;
                break;
            case GZ_ACK_DEBUG_COM_FAILED:
                step = GZ_STEP_ETHERNET;
                timeout = 0;
                progress_part = 1;
                progress_cnt = 0;
                log = "debug com failed";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\n调试串口  \t异常"));
                goto restart;
                break;
            case GZ_ACK_ETHERNET_SUCCESS:
                step = GZ_STEP_485;
                timeout = 0;
                progress_part = 2;
                progress_cnt = 0;
                log = "ethernet com success";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\n以太网通信  \t正常"));
                goto restart;
                break;
            case GZ_ACK_ETHERNET_FAILED:
                step = GZ_STEP_485;
                timeout = 0;
                progress_part = 2;
                progress_cnt = 0;
                log = "ethernet com failed";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\n以太网通信  \t异常"));
                goto restart;
                break;
            case GZ_ACK_485_SUCCESS:
                step = GZ_STEP_CAN;
                timeout = 0;
                progress_part = 3;
                progress_cnt = 0;
                log = "485 com success";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\n485通信  \t正常"));
                goto restart;
                break;
            case GZ_ACK_485_FAILED:
                step = GZ_STEP_CAN;
                timeout = 0;
                progress_part = 3;
                progress_cnt = 0;
                log = "485 com failed, result:";
                log += QString::number( m_result[TEST_IDX_485].result, 16);
                qDebug() << log;
                emit logInfo(log);

                m_result_info.append(tr("\r\n485通信  \t异常"));
                m_result_info.append(tr("\r\n(详情如下)："));
                for(int i=0; i<8; i++) {
                    m_result_info.append(tr("\r\n\t通道"));
                    m_result_info.append( QString::number( (i+1), 10 ) );

                    mask = 1<<i;
                    if(m_result[TEST_IDX_485].result & mask)
                        m_result_info.append(tr("正常"));
                    else
                        m_result_info.append(tr("异常"));
                }
                goto restart;
                break;
            case GZ_ACK_CAN_SUCCESS:
                step = GZ_STEP_PMBUS;
                timeout = 0;
                progress_part = 4;
                progress_cnt = 0;
                emit progress(progress_part, progress_cnt);
                log = "can com success";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\ncan通信  \t正常"));
                goto restart;
                break;
            case GZ_ACK_CAN_FAILED:
                step = GZ_STEP_PMBUS;
                timeout = 0;
                progress_part = 4;
                progress_cnt = 0;
                emit progress(progress_part, progress_cnt);
                log = "can com failed";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\ncan通信  \t异常"));
                goto restart;
                break;
            case GZ_ACK_PMBUS_SUCCESS:
                // end
                timeout = 0;
                progress_part = 5;
                progress_cnt = 0;
                emit progress(progress_part, progress_cnt);
                log = "pmbus com success";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\npmbus通信  \t正常"));
                goto end;
                break;
            case GZ_ACK_PMBUS_FAILED:
                // end
                timeout = 0;
                progress_part = 5;
                progress_cnt = 0;
                emit progress(progress_part, progress_cnt);
                log = "pmbus com failed";
                qDebug() << log;
                emit logInfo(log);
                m_result_info.append(tr("\r\npmbus通信  \t异常"));
                goto end;
                break;
            default:
                break;
            }
        }
        timeout = 0;
    }

end:
    for(int i = 0; i < TEST_ITEMS_NUM; i++ ) {
        if( m_result[i].isPass != true )
            return GZ_END_FAILED;
    }

    return GZ_END_SUCCESS;
}

void ComTest::DealWithAck( QString ackBuff )
{
    /* eg. ackBuff: gz_com test ack/nack debug_com */
    QStringList ack = ackBuff.split(' ');

    if(ack.size() < 4){
        qDebug("err ack, return");
        return;
    }

    QMap<QString, int>::const_iterator it = m_ackMap.find(ack.at(3));
    if(it == m_ackMap.end()) {
        qDebug("err, not find buff1 ,return");
        return;
    }

    int id = it.value();
    qDebug()<<"id:"<<id;
    SaveResult(ackBuff, id);

    // verify step
    QString toVerifyStep;
    toVerifyStep.append(ack.at(2));
    toVerifyStep.append(" ");
    toVerifyStep.append(ack.at(3));
    QMap<QString, eTestAckDef>::const_iterator it2;
    it2 = m_stepMap.find(toVerifyStep);
    if(it2 == m_stepMap.end())
    {
        qDebug("err, not find buff2, return");
        return;
    }
    m_ack = it2.value();
}

void ComTest::SaveResult(QString ackBuff, int id)
{
    QStringList ack = ackBuff.split(' ');
    qDebug() << "ack list size:" << ack.size();

    if( 0 == ack[2].compare("ack") ){
        m_result[id].isPass = true;
    }else if( 0 == ack[2].compare("nack") ){
        m_result[id].isPass = false;
        if( 0 == ack[3].compare("485") ) {
            if(ack.size() <= 4)
                m_result[id].result = 0;
            else
                m_result[id].result = static_cast<uint32_t>( ack[4].toULong(nullptr, 16) );
        }
    }
}


void Widget::on_btn_about_clicked()
{
    QMessageBox::about(this, tr("关于"), tr("功能: avg充电站工装测试软件\r\n"
                                          "版本:  V1.0.0\r\n"
                                          "编译时间:  20191126 16:26\r\n"
                                          "作者:  李扬\r\n"
                                          "邮箱:  liyang@ecthf.com\r\n"
                                          "公司：安徽博微智能电气有限公司"));
}
