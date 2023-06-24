#ifndef WIDGET_H
#define WIDGET_H
#include <string.h>
#include <QWidget>
#include <QFile>
#include "RealFIR.h"


namespace Ui
{
        class Widget;
}

class Widget : public QWidget
{
         Q_OBJECT

public:
        explicit Widget(QWidget *parent = nullptr);
        ~Widget();

private slots:
        void on_processPBN_clicked();       //слот для кнопки "обработать"
                                            //(метод запускается при нажатии на кнопку обработать)

        void fileSelectSlot();              //слот для кнопок выбора файла

private:

        // частота дискретизации реализаций
        const int SMPL_FRQ = 44100;

        // размер реализации для символов - 0.1 секунды
        //const int S_SIZE = SMPL_FRQ/10;
        const int S_Simvol=44100/10;

        // размер буфера для работы с файлом
        const int BUF_SIZE = 1024*1024;

private:
        Ui::Widget *ui;

        float       *buf;                   // буфер для работы с файлом

        QHash < char , float* >    symbol;   //список символов
        QHash < char ,  int   >    symbol2;   // список алфавита.(хранит символ и размер его реализации)

        double ph = 0; //phase

        RealFIR *filter;   // указатель на класс фильтрации

        void clearSymbol();  // метод для очистки алфавита

        void addSymbol(char c, const QString &code, double &ph);   // метод добавляет  новый символ ’c’ и
                                                        // то как он закодирован

        QString Messege;    //строка для хранения введенного сообщения

        void log( const QString &s );  // метод вывода сообщения о работе приложения


};

#endif // WIDGET_H
