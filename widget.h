#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QProgressDialog>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class AbstractReadWriter;
class ComTest;
class MyProgressDlg;

class Widget : public QWidget
{
    Q_OBJECT

#define    W_MAIN_WIDGET      580
#define    H_MAIN_WIDGET      100
#define    H_EXT_MAIN_WIDGET  350


public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void createConnect();

public:
signals:
    void serialStateChanged(bool);
    void writeBytesChanged(qint64 bytes);
    void readBytesChanged(qint64 bytes);

private slots:
    void on_showDetailBtn_toggled(bool checked);
    void on_startBtn_clicked();

    bool openReadWriter();
    void closeReadWriter();
    void readData();
    qint64 writeData(const QByteArray &data);

    void readToSend(QByteArray data);
    void dealWithRecData(qint64 bytes);
    void dealWithSendData(qint64 bytes);
    void startTest(void);

    void logMsg(const QString &message);

private:
    QStringList getSerialNameList();
    bool isReadWriterConnected();
    void parse_rec_data(QByteArray rec);

private:
    Ui::Widget *ui;
    AbstractReadWriter *_readWriter{nullptr};
    qint64 sendCount{0};
    qint64 receiveCount{0};
    QByteArray recBuff;
    QByteArray sendBuff;
    ComTest *comTest = nullptr;
    MyProgressDlg *testProgressDlg = nullptr;
};


class MyProgressDlg : public QProgressDialog
{
    Q_OBJECT

public:
    explicit MyProgressDlg(QWidget *parent = nullptr);
    ~MyProgressDlg();

    void partFinish(int part) {
        progressCnt = (m_maxValue/m_partNum) * part;
        setValue( static_cast<int>(progressCnt) );
    }
    void setPartNum(int num) {
        m_partNum = num;
    }
    void setMaxNum(int max) {
        m_maxValue = max;
        setMaximum(max);
    }

    void keyPressEvent(QKeyEvent *event);

    QTimer *progressTimer = nullptr;
    int progressCnt = 0;
public slots:
    void showProgress(int part, int cnt) {
        int base = (m_maxValue/m_partNum) * part;
        progressCnt = base + cnt;
        setValue(progressCnt);
    }

private:
    int m_maxValue = 0;
    int m_partNum = 0; // m_maxValue一共分为m_partNum份，用来作为阶段进度显示
};

class ComTest : public QObject
{
    Q_OBJECT

#define    TEST_ITEMS_NUM  4

#define    MAX_FAIL_CNT    3

#define    TEST_IDX_DEBUG_COM    0
#define    TEST_IDX_ETHERNET     1
#define    TEST_IDX_485          2
#define    TEST_IDX_CAN          3
#define    TEST_IDX_PMBUS        4

    typedef enum _gz_test_step{
        GZ_STEP_DEBUG_COM,
        GZ_STEP_ETHERNET,
        GZ_STEP_485,
        GZ_STEP_CAN,
        GZ_STEP_PMBUS
    }eTestStepDef;
    typedef enum _gz_test_ack_def{
        GZ_ACK_NONE,
        GZ_ACK_DEBUG_COM_SUCCESS,
        GZ_ACK_DEBUG_COM_FAILED,
        GZ_ACK_ETHERNET_SUCCESS,
        GZ_ACK_ETHERNET_FAILED,
        GZ_ACK_485_SUCCESS,
        GZ_ACK_485_FAILED,
        GZ_ACK_CAN_SUCCESS,
        GZ_ACK_CAN_FAILED,
        GZ_ACK_PMBUS_SUCCESS,
        GZ_ACK_PMBUS_FAILED
    }eTestAckDef;
    Q_ENUM(eTestAckDef)

    typedef struct {
        bool isPass;
        uint32_t result;
    }eTestDetailDef;

    typedef enum _gz_test_end {
        GZ_END_SUCCESS,
        GZ_END_FAILED,
        GZ_END_COM_TIMEOUT
    }eTestEndResult;

public:
    ComTest();
    ~ComTest();

public slots:
    int Test(void);
    void DealWithAck( QString ackBuff );
    void SaveResult(QString ack, int id);
    void Reset(void) {
        m_ack = GZ_ACK_NONE;
        m_result_info = "\r\n结果如下:\r\n";
        for(int i=0; i<TEST_ITEMS_NUM; i++) {
            m_result[i].isPass = false;
            m_result[i].result = 0;
        }
    }
signals:
    void sendData(QByteArray data);
    void progress(int part, int cnt);
    void logInfo(const QString &message);

private:
    friend class Widget;

    eTestAckDef m_ack = GZ_ACK_NONE;
    eTestDetailDef m_result[TEST_ITEMS_NUM];
    QString m_result_info = "\r\n结果如下:\r\n";

    QStringList m_gzTestBuffList;
    QMap<QString, int> m_ackMap;
    QMap<QString, eTestAckDef> m_stepMap;
    int m_testItemsNum;
};


#endif // WIDGET_H
