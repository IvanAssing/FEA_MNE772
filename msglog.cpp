#include "msglog.h"


int MsgLog::count = 0;
QListWidget* MsgLog::output = nullptr;


void MsgLog::information(QString str)
{
    if(output==nullptr) return;

    QDateTime now = QDateTime::currentDateTime();
    QString strnow = now.toString("[hh:mm:ss] ");

    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(strnow + str);
    QFont *font =  new QFont("Ubuntu", 10, QFont::Light);
    //font->setItalic(true);

    newItem->setFont(*font);
    newItem->setTextColor(QColor(0,0,0));
    output->insertItem(count, newItem);
    output->scrollToBottom();
    output->repaint();

    count++;
}

void MsgLog::result(QString str)
{
    if(output==nullptr) return;

    QDateTime now = QDateTime::currentDateTime();
    QString strnow = now.toString("[hh:mm:ss] ");

    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(strnow + str);
    QFont *font =  new QFont("Ubuntu", 10, QFont::Bold);
    //font->setItalic(true);

    newItem->setFont(*font);
    newItem->setTextColor(QColor(0,0,255));
    output->insertItem(count, newItem);
    output->scrollToBottom();
    output->repaint();

    count++;
}

void MsgLog::error(QString str)
{
    if(output==nullptr) return;

    QDateTime now = QDateTime::currentDateTime();
    QString strnow = now.toString("[hh:mm:ss] ");

    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(strnow + str);
    QFont *font =  new QFont("Ubuntu", 10, QFont::Bold);
    //font->setItalic(true);

    newItem->setFont(*font);
    newItem->setTextColor(QColor(255,0,0));
    output->insertItem(count, newItem);
    output->scrollToBottom();
    output->repaint();

    count++;
}


void MsgLog::information(char *str)
{
    MsgLog::information(QString(str));
}

void MsgLog::result(char *str)
{
    MsgLog::result(QString(str));
}

void MsgLog::error(char *str)
{
    MsgLog::error(QString(str));
}


