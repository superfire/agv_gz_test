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

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void createConnect();

public:
signals:
    void serialStateChanged(bool);

private slots:
    void on_showDetailBtn_toggled(bool checked);
    void on_startBtn_clicked();


    bool openReadWriter();
    void closeReadWriter();
    void readData();
    qint64 writeData(const QByteArray &data);

private:
    QStringList getSerialNameList();
    void logMsg(const QString &message);
    bool isReadWriterConnected();
    void parse_rec_data(QByteArray rec);

private:
    Ui::Widget *ui;
    AbstractReadWriter *_readWriter{nullptr};
};
#endif // WIDGET_H
