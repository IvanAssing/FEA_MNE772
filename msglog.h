#ifndef MSGLOG_H
#define MSGLOG_H

#include <QObject>
#include <QtWidgets>
#include <QListWidget>
#include <QString>
#include <iostream>


class MsgLog : public QObject
{
        Q_OBJECT

public:
    static int count;
    static QListWidget *output;

    static void information(QString str);
    static void result(QString str);
    static void error(QString str);

    static void information(char *str);
    static void result(char *str);
    static void error(char *str);

};


#endif // MSGLOG_H
