#include "mainwindow.h"  
#include "ui_mainwindow.h"  
#include <QTextCodec>

MainWindow::MainWindow(QWidget *parent) :  
    QMainWindow(parent),  
    ui(new Ui::MainWindow)  
{  
    ui->setupUi(this);  

    socket = new QTcpSocket();  

    //�����źŲ�  
    QObject::connect(socket, &QTcpSocket::readyRead, this, &MainWindow::socket_Read_Data);  
    QObject::connect(socket, &QTcpSocket::disconnected, this, &MainWindow::socket_Disconnected);  
  
    ui->pushButton_Send->setEnabled(false);  
    ui->lineEdit_IP->setText("101.132.131.164");
    ui->lineEdit_Port->setText("10774");
  
}  
  
MainWindow::~MainWindow()  
{  
    delete this->socket;  
    delete ui;  
}  
  
void MainWindow::on_pushButton_Connect_clicked()  
{  
    if(ui->pushButton_Connect->text() == "����")
    {  
        qDebug() << "!!!";
        QString IP;  
        int port;  
  
        //��ȡIP��ַ  
        IP = ui->lineEdit_IP->text();  

        qDebug() << IP;
        //��ȡ�˿ں�  
        port = ui->lineEdit_Port->text().toInt();  
  
        //ȡ�����е�����  
        socket->abort();  
        //���ӷ�����  
        socket->connectToHost(IP, port);  
  
        //�ȴ����ӳɹ�  
        if(!socket->waitForConnected(30000))
        {  
            qDebug() << "Connection failed!";  
            return;
        }
        qDebug() << "Connect successfully!";
        ui->pushButton_Send->setEnabled(true);
        ui->pushButton_Connect->setText("�Ͽ�����");
    }
    else
    {  
        //�Ͽ�����  
        socket->close();
        socket->disconnectFromHost();
        socket->waitForDisconnected();
        qDebug() << "Disconnected!2";
        //�޸İ�������  
        ui->pushButton_Connect->setText("����");
        ui->pushButton_Send->setEnabled(false);  
    }  
}  
  
void MainWindow::on_pushButton_Send_clicked()  
{  
    qDebug() << "Send: " << ui->textEdit_Send->toPlainText();  
    QString strtest = ui->textEdit_Send->toPlainText();
    QTextCodec *code= QTextCodec::codecForName("UTF-8");
    QByteArray bytest = code->fromUnicode( strtest );
    socket->write(bytest);
    socket->flush();  
}  
  
void MainWindow::socket_Read_Data()  
{  
    QByteArray buffer;  
    //��ȡ����������  
    buffer = socket->readAll();
    if(!buffer.isEmpty())  
    {
        QString str = ui->textEdit_Recv->toPlainText();
        str+=tr(buffer);  
        //ˢ����ʾ  
        ui->textEdit_Recv->setText(str);  
    }  
    buffer = socket->readAll();
    if(!buffer.isEmpty())
    {
        QString str = ui->textEdit_Recv->toPlainText();
        str+=tr(buffer);
        //ˢ����ʾ
        ui->textEdit_Recv->setText(str);
    }
}  
  
void MainWindow::socket_Disconnected()  
{  
    //���Ͱ���ʧ��  
    ui->pushButton_Send->setEnabled(false);  
    //�޸İ�������  
    ui->pushButton_Connect->setText("����");  
    socket->close();
    socket->disconnectFromHost();
    socket->waitForDisconnected();
    qDebug() << "Disconnected!1";
}
