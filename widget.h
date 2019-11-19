#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class AbstractReadWriter;

class Widget : public QWidget
{
    Q_OBJECT

#define    W_MAIN_WIDGET      580
#define    H_MAIN_WIDGET      140
#define    H_EXT_MAIN_WIDGET  350

#define    MAX_FAIL_CNT    3

#define    IDX_DEBUG_COM    0
#define    IDX_ETHERNET     1
#define    IDX_485          2
#define    IDX_CAN          3
#define    IDX_PMBUS        4

typedef enum _gz_test_step{
    GZ_STEP_DEBUG_COM,
    GZ_STEP_ETHERNET,
    GZ_STEP_485,
    GZ_STEP_CAN,
    GZ_STEP_PMBUS
}gzTestStepDef;

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void createConnect();

public:
signals:
    void serialStateChanged(bool);
    void writeBytesChanged(qint64 bytes);
    void readBytesChanged(qint64 bytes);

    void gzTestFailed(void);
    void gzTestSuccess(void);

private slots:
    void on_showDetailBtn_toggled(bool checked);
    void on_startBtn_clicked();


    bool openReadWriter();
    void closeReadWriter();
    void readData();
    qint64 writeData(const QByteArray &data);

    void dealWithRecData(qint64 bytes);
    void dealWithSendData(qint64 bytes);

private:
    QStringList getSerialNameList();
    void logMsg(const QString &message);
    bool isReadWriterConnected();
    void parse_rec_data(QByteArray rec);

private:
    Ui::Widget *ui;
    AbstractReadWriter *_readWriter{nullptr};
    qint64 sendCount{0};
    qint64 receiveCount{0};
    QByteArray recBuff;
    QByteArray sendBuff;

    gzTestStepDef gzTestStep;

    QStringList gzSendBuffList;
    QStringList gzAckBuffList;

    uint32_t gzTestResult{0};
};
#endif // WIDGET_H
