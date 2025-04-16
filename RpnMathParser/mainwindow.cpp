#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "rpnmathparser.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("RpnMathParser demo");
    ui->le_result->setReadOnly(true);
    ui->te_log->setReadOnly(true);
    connect(ui->le_expression, &QLineEdit::returnPressed, ui->pb_calculate, &QPushButton::click);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pb_calculate_clicked()
{
    QString err;
    ui->te_log->clear();
    ui->le_result->setText(QString::number(RpnMathParser::parseString(ui->le_expression->text(), err), 'g', precision));
    ui->te_log->append(err);

    QRegExp rx("\\d+");
    QStringList numbers;
    int pos = 0;
    while ((pos = rx.indexIn(err, pos)) != -1) {
        numbers << rx.cap(0);
        pos += rx.matchedLength();
    }
    if (numbers.length() > 0) {
        int startIndex = numbers.at(0).toInt();
        ui->le_expression->setCursorPosition(startIndex);
    }
}


