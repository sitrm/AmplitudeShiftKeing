#include <QFileDialog>    //класс для "диалогов" с файлами, если хотим открывать сохранять и тп
#include "widget.h"      //подключил заголовочный файл
#include "ui_widget.h"       //подключил заголовочный файл
#include <cmath>           // математический файл
#include <QTime>            // чтобы время получать
#include <QDebug>           //класс для консоли



Widget::Widget(QWidget *parent) :  QWidget(parent), ui(new Ui::Widget)  //ui- user interface
{                                                                       //используется для обращения к интерфейсу окна
    ui->setupUi(this);
    ui->fileTxtPBN->setProperty("fileRole",100);    //установливаем свойства кнопкам,
    ui->fileTxtPBN_2->setProperty("fileRole",102);

    log("Приложение запущено");
    buf = new float[BUF_SIZE];      //выделил память по размеру BUF_SIZE
    filter = new RealFIR( RealFIR::Bandpass, 2000, 4000);        //создал полосовой фильтр
    //clearSymbol();
    // добавляем символы
    // · = 1
    // - = 111  (т.к. (длительность тире) = 3*(длительность точки))
    //между точкой и тире 0 (т.к. (пауза между элементами символа) =(длительность точки) )
    //на конце каждого символа 000 (т.к. (пауза между соседними символами)=3*(длительность точки))
    //знак пробела ' '=0000 (т.к. (пауза между соседними словами) = 7*(длительность точки))
    //(т.е. 000 на конце символа + 0000 пробел= 7)
//     addSymbol (' ', "0000");
//     addSymbol ('a', "10111000");
//     addSymbol ('b', "111010101000");
//     addSymbol ('c', "11101011101000");
//     addSymbol ('d', "1110101000");
//     addSymbol ('e', "1000");
//     addSymbol ('f', "101011101000");
//     addSymbol ('g', "111011101000");
//     addSymbol ('h', "1010101000");
//     addSymbol ('i', "101000");
//     addSymbol ('j', "1011101110111000");
//     addSymbol ('k', "111010111000");
//     addSymbol ('l', "101110101000");
//     addSymbol ('m', "1110111000");
//     addSymbol ('n', "11101000");
//     addSymbol ('o', "11101110111000");
//     addSymbol ('p', "10111011101000");
//     addSymbol ('q', "1110111010111");
//     addSymbol ('r', "1011101000");
//     addSymbol ('s', "10101000");
//     addSymbol ('t', "111000");
//     addSymbol ('u', "1010111000");
//     addSymbol ('v', "101010111000");
//     addSymbol ('w', "101110111000");
//     addSymbol ('x', "11101010111000");
//     addSymbol ('y', "1110101110111000");
//     addSymbol ('z', "11101110101000");
//     addSymbol ('0', "1110111011101110111000");
//     addSymbol ('1', "10111011101110111000");
//     addSymbol ('2', "101011101110111000");
//     addSymbol ('3', "1010101110111000");
//     addSymbol ('4', "10101010111000");
//     addSymbol ('5', "101010101000");
//     addSymbol ('6', "11101010101000");
//     addSymbol ('7', "1110111010101000");
//     addSymbol ('8', "111011101110101000");
//     addSymbol ('9', "11101110111011101000");
//     addSymbol ('.', "10101010101000");
//     addSymbol (',', "10111010111010111000");
//     addSymbol (';', "11101011101011101000");
//     addSymbol (':', "11101110111010101000");
//     addSymbol ('!', "1110111010101110111000");
//     addSymbol ('?', "101011101110101000");
//     addSymbol ('@', "10111011101011101000");
}

Widget::~Widget()               //деструктор класса
{
    clearSymbol();
    delete filter;
    delete[] buf;
    delete ui;
}

void Widget::clearSymbol()
{
    foreach( float *s, symbol ) { delete [] s; }        // очищаем содержимое контейнера
    symbol.clear();
    symbol2.clear();
}

void Widget::addSymbol(char c, const QString &code, double &ph)      //метод добавления символа
{
    int f = 3000; //frequence
    if ( symbol.contains(c) )   // если такой символ уже есть, то удаляем его
    {
        delete symbol[c];
        symbol.remove(c);
        symbol2.remove(c);
    }

    int size =S_Simvol*code.count();  //S_Simvol-число отсчетов необходимых сделать для передачи одного символа (0 или 1)
    int n=0;                         //code.count()- число '0' и '1' при кодировке символа азбукой морзе
    float* data = new float [ size ];  // data хранит реализацию целого символа
    for(int i = 0; i < code.count (); ++i)   // цикл который идёт по ноликам и единицам в символе
    {
        if (code[i] == '1')
        {//-------------------------------------------------SIG-----------------------------------------------------

            for( int j = 0 ; j < S_Simvol; ++j )   //цикл который делает S_Simvol=4410 отсчётов для '1'
            {
                data[n] = cos( 2*M_PI*f*j/SMPL_FRQ + ph);
                //if (j==0) qDebug()<<"нач="<<2*M_PI*f*(j)/SMPL_FRQ + ph<<'\t';
                //if (j==S_Simvol-1) qDebug()<<"след ожид="<< 2*M_PI*f*(j+1)/SMPL_FRQ + ph<<'\t';
                n++;
            }
            ph+= 2*M_PI*f*(S_Simvol)/SMPL_FRQ;
            ph=fmod(ph,2*M_PI);
            //qDebug()<<"ph="<<ph<<'\n';
        }
        else
        {
            for( int j = 0 ; j < S_Simvol; ++j  )
            {
                data[n] = 0;
                n++;
            }

        }

    }


    symbol.insert(c,data);      //добавляем в список символ и его реализацию
    symbol2.insert(c,size);     //добавляем в список символ и size - число отсчётов для передачи именно этого символа

}


void Widget::log(const QString &s)   //метод вывода сообщений в окно
{
        QString m;          // создаем временную переменную
        m = QTime::currentTime().toString();  // получаем текущее время в виде строки
         // добавляем разделитель из 4 пробелов
        m += "    ";
        // добавляем переданное сообщение
        m += s;
        // добавляем сообщение к тексту в logPTE
        ui->logPTE->appendPlainText( m );
        // выводим это же сообщение в поток отладки
        qDebug() << m;   //вывод в терминал
}

void Widget::on_processPBN_clicked()   // метод глвной кнопки "обработать"
{
    QTextStream *ts = nullptr;

    try {

             log("Обработка идёт полным ходом");

             QFile outFile;
             QFile in_File;

             QLineEdit *led2 = ui->fileMesLED;



             if (led2->text().isEmpty())       //если не ввели сообщение то обрабатываем файл
             {
                    in_File.setFileName( ui->fileTxtLED->text() );    //передаем пути к файлам конструкторам объекта класса QFile
                    outFile.setFileName( ui->fileTxtLED_2->text() + ".pcm" );

                    // пытаемся открыть файлы
                    if( !in_File.open(QIODevice::ReadOnly) )
                        { throw( "Ошибка при открытии исходного файла" ); }

                    if( !outFile.open(QIODevice::ReadWrite) )
                        { throw( "Ошибка при открытии файла с результатом" ); }

                    // в этой точке оказываемся,если файлы открылись успешно и не было вызвано исключений

                    outFile.resize(0);  // изменяем размер файла для записи

                    log("Входной файл открыт успешно.");


                    ts = new QTextStream( &in_File );

              }
              else
              {
                    Messege = led2->text();
                    ts = new QTextStream( &Messege );
                    outFile.setFileName( ui->fileTxtLED_2->text()+".pcm");
                    if( !outFile.open(QIODevice::ReadWrite) )
                        { throw( "Ошибка при открытии файла с результатом" ); }

                    // в этой точке оказываемся,если файлы открылись успешно и не было вызвано исключений

                    outFile.resize(0);  // изменяем размер файла для записи
                    log("Выходной файл открыт успешно. ура");
                    ui->fileTxtLED->selectAll();  //выделяем строку с путем к входному файлу
                    ui->fileTxtLED->cut();        //удаляем выделенное
              }

              char            c;      // временная переменная
              int counter   = 0;      // число незнакомых символов
              int fltSize   ;         // размер реализации на выходе фильтра

              filter->reset();    // сбрасываем состояние фильтра

              while( !(ts->atEnd()) )    // читаем поток до тех пор, пока не кончатся данные
              {

                    *ts >> c;
                    // ADD
                   //addSymbol (' ', "0000");


                  switch (c)
                  {
                  case (int)' ':
                      addSymbol (' ', "0000",ph);
                      break;
                  case (int)'a':
                           addSymbol ('a', "10111000",ph);
                           break;
                  case (int)'b':
                           addSymbol ('b', "111010101000",ph);
                           break;
                  case (int)'c':
                           addSymbol ('c', "11101011101000",ph);
                           break;
                  case (int)'d':
                           addSymbol ('d', "1110101000",ph);
                           break;
                  case (int)'e':
                           addSymbol ('e', "1000",ph);
                           break;
                  case (int)'f':
                           addSymbol ('f', "101011101000",ph);
                           break;
                  case (int)'g':
                           addSymbol ('g', "111011101000",ph);
                           break;
                  case (int)'h':
                           addSymbol ('h', "1010101000",ph);
                           break;
                  case (int)'i':
                           addSymbol ('i', "101000",ph);
                           break;
                  case (int)'j':
                           addSymbol ('j', "1011101110111000",ph);
                           break;
                  case (int)'k':
                           addSymbol ('k', "111010111000",ph);
                           break;
                  case (int)'l':
                           addSymbol ('l', "101110101000",ph);
                           break;
                  case (int)'m':
                           addSymbol ('m', "1110111000",ph);
                           break;
                  case (int)'n':
                           addSymbol ('n', "11101000",ph);
                           break;
                  case (int)'o':
                           addSymbol ('o', "11101110111000",ph);
                           break;
                  case (int)'p':
                           addSymbol ('p', "10111011101000",ph);
                           break;
                  case (int)'q':
                           addSymbol ('q', "1110111010111000",ph);
                           break;
                  case (int)'r':
                           addSymbol ('r', "1011101000",ph);
                           break;
                  case (int)'s':
                           addSymbol ('s', "10101000",ph);
                           break;
                  case (int)'t':
                           addSymbol ('t', "111000",ph);
                           break;
                  case (int)'u':
                           addSymbol ('u', "1010111000",ph);
                           break;
                  case (int)'v':
                           addSymbol ('v', "101010111000",ph);
                           break;
                  case (int)'w':
                           addSymbol ('w', "101110111000",ph);
                           break;
                  case (int)'x':
                           addSymbol ('x', "11101010111000",ph);
                           break;
                  case (int)'y':
                           addSymbol ('y', "1110101110111000",ph);
                           break;
                  case (int)'z':
                           addSymbol ('z', "11101110101000",ph);
                           break;
                  case (int)'0':
                           addSymbol ('0', "1110111011101110111000",ph);
                           break;
                  case (int)'1':
                           addSymbol ('1', "10111011101110111000",ph);
                           break;
                  case (int)'2':
                           addSymbol ('2', "101011101110111000",ph);
                           break;
                  case (int)'3':
                           addSymbol ('3', "1010101110111000",ph);
                           break;
                  case (int)'4':
                           addSymbol ('4', "10101010111000",ph);
                           break;
                  case (int)'5':
                           addSymbol ('5', "101010101000",ph);
                           break;
                  case (int)'6':
                           addSymbol ('6', "11101010101000",ph);
                           break;
                  case (int)'7':
                           addSymbol ('7', "1110111010101000",ph);
                           break;
                  case (int)'8':
                           addSymbol ('8', "111011101110101000",ph);
                           break;
                  case (int)'9':
                           addSymbol ('9', "11101110111011101000",ph);
                           break;
                  case (int)'.':
                           addSymbol ('.', "10101010101000",ph);
                           break;
                  case (int)',':
                           addSymbol (',', "10111010111010111000",ph);
                           break;
                  case (int)';':
                           addSymbol (';', "11101011101011101000",ph);
                           break;
                  case (int)':':
                           addSymbol (':', "11101110111010101000",ph);
                           break;
                  case (int)'!':
                           addSymbol ('!', "1110111010101110111000",ph);
                           break;
                  case (int)'?':
                           addSymbol ('?', "101011101110101000",ph);
                           break;
                  case (int)'@':
                           addSymbol ('@', "10111011101011101000",ph);
                  }



                    if( symbol.contains(c)  )   // если очередной символ известен,то фильтруем реализацию
                    {
                        filter->filter( symbol[c],
                                     // входная реализация (массив значений в отсчётных точках)
                                     symbol2[c],
                                     // число отсчетов на входе
                                     buf,
                                     // в buf записывается выходная реализация после обработки фильтром
                                     &fltSize );
                                     // число отсчетов после обработки фильтром

                        // записываем результат в файл
                        outFile.write( (char*)buf, fltSize*sizeof(float) );



    //                    outFile.write( (char*)symbol[c],symbol2[c]*sizeof (float) );  // неотфильтрованный сигнал
                     }

                     else { counter++; }   // подсчитываем незнакомые нам символы

                }

                log( "Обработка завершена" );
                log("Файл с результатом сформирован");
                log( "Число незнакомых символов - " + QString::number(counter) );
                log( "Результат работы записан в " + outFile.fileName() );

            }
            catch( const char *s )
                { // перехват исключений типа "строка"
                log(s);
                }
            catch(...)
                { // перехват всех остальных исключений
                log("Неизвестная ошибка обработки");
                }
            delete ts;
}

void Widget::fileSelectSlot()            //метод вызывающийся при нажатии остальных кнопок
{
      QObject *obj=sender();          //получаем ссылку на вызывающего

      if( obj !=NULL )
      {
          QLineEdit *led1 = NULL;   // "пустой" указатель на строку ввода
          QLineEdit *led3 = NULL;
          QString str;
          // получаем значение динамического свойства
          // если свойства нет, то будет получено значение 0
          int  role = obj->property("fileRole").toInt();
          //Настраиваем окружение в зависимости от нажатой кнопки
          switch( role )
                {

                case 100 :
                led1 = ui->fileTxtLED;    //лед1 указывает на поле ввода входного файла
                str = "входной файл ";
                break;

                case 102:
                led3 = ui->fileTxtLED_2;
                str = "файл куда сохранить результат ";
                break;

                }

                if( led1 != NULL )
                {
                        log( "Выбираем " + str );
                        // создаем диалоговое окно для выбора одного файла
                        QString fileName = QFileDialog::getOpenFileName( this,"Выберите " + str);
                        // если пользователь выбрал файл, то строка не пустая
                        if( !fileName.isEmpty() )
                        {
                            led1->setText( fileName );  // сохраняем выбор
                            ui->fileMesLED->selectAll();  //выделяем строку с сообщением
                            ui->fileMesLED->cut();        //удаляем выделенное
                        }
                        else { log( "Пользователь не захотел выбирать, вот же ..." ); }

                }


                if (led3!= NULL)
                {
                    log( "Выбираем " + str );
                    // создаем диалог для выбора одного файла
                    QString fileName = QFileDialog::getSaveFileName( this,"Выберите " + str);
                    // если пользователь выбрал файл, то строка не пустая
                    if( !fileName.isEmpty() )
                    {
                        led3->setText( fileName ); // сохраняем выбор
                    }
                    else { log( "Пользователь отказался от выбора файла" ); }

                }
       }
}




