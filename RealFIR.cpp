//---------------------------------------------------------------------------
#include <cmath>
#include "RealFIR.h"
//---------------------------------------------------------------------------
#include <QDebug>
//---------------------------------------------------------------------------
RealFIR::RealFIR( FltrType       type,          // тип фильтра, константы см. выше
                  double         freq1,         // первая характерная частота - имеет смысл для всех фильтров
                  double         freq2,         // вторая характерная частота - имеет смысл только для полосовых и режекторных
                  double         sfrq,          // частота дискретизации
                  FltrWinType    winType,       // тип окна для рассчета фильтра
                  double         winLvl,
                  int            tCount,
                  Borders        strategy )
{
    // корректируем переданные значения
    if(!(tCount & 1)                          ) tCount++; // должен быть нечетным
    if( tCount    < 8                         ) tCount = 257;
    if( sfrq      < 1.                        ) sfrq   = 1.;
    if( freq1     < 0.    || freq1 >= sfrq/2. ) freq1  =  sfrq*0.25;
    if( freq2     < freq1 || freq2 >= sfrq/2. ) freq2  =  sfrq*0.49;
    if(                      freq1 >= freq2   ) freq1  = (sfrq*0.5 + freq2)/2.;

    smplFreq   = sfrq;

    // переводим частоты в относительные
    freq1 /= smplFreq;
    freq2 /= smplFreq;
    try {
        taps    = new float[tCount];
     // генерируем фильтр
        genFilter( freq1, freq2, type, taps, tCount, winType, winLvl);
     // выделяем память под массивы
        initMemory( tCount );
     // настраиваем переходный процесс
        setBorderStrategy( strategy );
    }
    catch(...) {
        outputErr( "Ошибка инициализации фильтра", "Перезапустите программу" );
    }
}
//---------------------------------------------------------------------------
RealFIR::~RealFIR( void )
{
    delete[] firDly;
    delete[] tmpF;
}
//---------------------------------------------------------------------------
void RealFIR::initMemory( int tCnt )
{
    try {
     // сохраняем значения переменных и инициализируем
        error      = false;

        tapCnt      = tCnt;
        finalized   = false;
        firDly      = NULL;
        tmpF        = NULL;
     // выбор размера блока, которым ведется обработка
        blockSizeSMPL = BLOCK_SIZE > tapCnt*2 ? BLOCK_SIZE : tapCnt*2;
     // выделяем память под массивы
        tmpF        = new float[ blockSizeSMPL ];
        firDly      = new float[ tapCnt-1 ] ;
     // буферы для работы с short
        src16Buf    = new float[ BUF16 ];
        dst16Buf    = new float[ BUF16 + tapCnt];
    }
    catch(...) {
        outputErr( "Ошибка выделения памяти", "Перезапустите программу" );
    }
}
//---------------------------------------------------------------------------
void RealFIR::outputErr( const QString &problem,
                         const QString &solve )
{
    error = true;
    qCritical() << problem << " - " << solve;
}
//---------------------------------------------------------------------------
void RealFIR::realInvDft(float *real_mas, int razmFFT, float *result ) {
    double wt;
 // Рассчитываем действительную часть для сигнала,
 // у которого мнимая часть спектра равна нулю.
    for( int n = 0  ; n < razmFFT ; ++n ) {
        result[n] = 0;
        for( int k = 0 ; k < razmFFT ; ++k ) {
            wt = 2*M_PI*n*k / razmFFT;
            result[n] += real_mas[k]*cos(wt);// - imag_mas[k]*sin(a); imag_mas все равно нулевая
        }
    }
}
//---------------------------------------------------------------------------
void RealFIR::genFilter( double        freq1,
                         double        freq2,
                         FltrType      fltType,
                         float         *t,
                         int           tCnt,
                         FltrWinType   winType,
                         double        winLvl )
{
    float   *win_R  = NULL;

    if( !(tCnt&0x01) ) throw("размер должен быть нечетным");

    try {
        int     bgn = -(tCnt-1) / 2; // 256 -> [-127..127]=255 ; 257 -> [-128..128]=257
        int     end =  (tCnt-1) / 2;
        double  fN, fV;

        if(end-bgn+1 > tCnt) throw("filter taps length error");

        win_R = new float[end-bgn+1];
     // настраиваем параметры генерации импульсной характеристики
        switch(fltType) {
            case Lowpass  : fN = 0.0;   fV = freq1; break;
            case Highpass : fN = freq1; fV = 0.5;   break;
            case Bandpass : fN = freq1; fV = freq2; break;
            case Bandstop : fN = freq2; fV = freq1; break;
            default: throw("no such type");
        }
     // формируем импульсную характеристику
        for( int n = bgn ; n <= end ; ++n ) {
            t[n-bgn] = 2*( fV*sinc(2*M_PI*n*fV)  -  fN*sinc(2*M_PI*n*fN) );
        }
        if( fltType == Bandstop ) { // для режекторного фильтра
            t[-bgn] += 1;        // (он сумма ФНЧ и ФВЧ),
        }                           // т.к. sinc(M_PI*n) будет везде 0, кроме n=0 --- там он 1
     // получаем оконную функцию
        if( winType == WinChebyshev ) {
         // рассчет окна чебышева
            chebyshev_window( winLvl, win_R,  tCnt );
        } else {
         // по умолчанию --- окно прямоугольное
            for( int i = 0 ; i < tCnt ; ++i ) {
                win_R[i] = 1.;
            }
        }
     // накладываем рассчитанное окно
        for( int i = 0 ; i < tCnt ; ++i ) {
            t[i] *= win_R[i];
        }
    }
    catch(...) {
        outputErr( "Ошибка рассчета фильтра", "Измените параметры рассчета" );
    }
    delete[] win_R;
}
//---------------------------------------------------------------------------
void RealFIR::setBSValues( void )
{
    switch( bStrat ) {
        case FullDlyLine :
            dlyLineBefore   = tapCnt-1;                     // в начале - полностью заполненная линия задержки
            dlyLineAfter    = 0;                            // нулей в конце не добавляем
            break;
        case HalfDlyLine :
            dlyLineBefore   = tapCnt/2;                     // заполняем половину линии задержки исходными данными
            dlyLineAfter    = tapCnt - dlyLineBefore - 1;   // FDlyLineBefore - это сколько отсчетов будет потерянно ДО первого выхода. Это надо компенсировать ПОСЛЕ
            break;
        case EmptyDlyLine :
            dlyLineBefore   = 0;    // сразу же начинаем фильтровать
            dlyLineAfter    = 0;    // после завершения фильтрации нулей не добавляем
            break;
        default : outputErr( "Неверное значение", "" );
    }
}
//---------------------------------------------------------------------------
void RealFIR::setBorderStrategy( Borders strategy )
{
    try {
        if( bStrat  != strategy &&
            strategy >= FullDlyLine &&
            strategy <= EmptyDlyLine )
        {
            bStrat = strategy;
         // устанавливаем счетчики переходных процессов
            setBSValues();
        }
        reset();
    }
    catch(...) {
        outputErr( "Ошибка смены стратегии", "" );
    }
}
//---------------------------------------------------------------------------
RealFIR::Borders RealFIR::getBorderStrategy( void )
{
    return bStrat;
}
//---------------------------------------------------------------------------
void RealFIR::reset()
{
    try {
     // устанавливаем линию задержки  в ноль
        memset( firDly, 0, (tapCnt-1)*sizeof(float) );
        firDlyPtr = tapCnt-2;
     // устанавливаем счетчики переходных процессов
        setBSValues();

        finalized = false;
    }
    catch(...) {
        outputErr( "Ошибка сброса", "" );
    }
}
//---------------------------------------------------------------------------
void RealFIR::filter( const float   *src,          // исходная реализация
                      int           srcSize,       // число ОТСЧЕТОВ на входе
                      float         *dst,          // массив для сохранения результатов фильтрации
                      int           *dstSize,      // число ОТСЧЕТОВ на выходе фильтра
                      bool          fin )     // флаг должен быть true для фильтрации последнего блока данных !!!РАЗМЕР НА ВЫХОДЕ МОЖЕТ БЫТЬ БОЛЬШЕ ВХОДНОГО!!!
{
 // разбиваем входную реализацию на кусочки, и передаем ее во внутреннюю ф-ю обработки
    int         stepCnt;    // число блоков для обработки
    int         stepLst;    // размер остатка для обработки
    int         tmpI;

    try {
        if( !finalized && !error && srcSize > 0 ) {
         // обработка переходного процесса в начале
            if( dlyLineBefore ) {
                tmpI = dlyLineBefore < srcSize ? dlyLineBefore : srcSize; // определили размер для начального заполнения линии задержки
                internalFIR( src,       // передаем в фильтр данные, не требуя выходной реализации
                             tmpI,
                             NULL );
                dlyLineBefore   -= tmpI;            // уменьшаем число отсчетов,требуемых для уменьшения переходного процесса
                srcSize         -= tmpI;            // уменьшаем размер данных на входе - часть уже использовали Х!
                src             += tmpI;            // смещаем указатель на позицию еще не обработанных данных
            }
         // расчет числа блоков, на которое разбиваем входную реализацию
            stepCnt = srcSize/blockSizeSMPL;
            stepLst = srcSize%blockSizeSMPL;
           *dstSize = srcSize;  // начальная инициализация выходного размера (может еще измениться при финализации)
         // фильтруем по блокам
            for( int i = 0 ; i < stepCnt && !error ; i++ ) {
                internalFIR( src,               // исходные данные
                             blockSizeSMPL,    // размер исходных данных
                             dst );
                src += blockSizeSMPL;
                dst += blockSizeSMPL;
            }
         // фильтрация последнего, не кратного блоку обработки, кусочка
            if( stepLst && !error ) {
                internalFIR( src,       // исходные данные
                             stepLst,   // размер исходных данных
                             dst );
                dst += stepLst;    // смещаем указатель, т.к. могут появиться еще данные для записи при финализации
            }
         // обработка финализации
            if( fin && dlyLineAfter && !error ) {
             // формируем массив нулей
                memset( tmpF, 0, dlyLineAfter*sizeof(float) );
             // выталкиваем данные нулями
                internalFIR( tmpF,             // исходные данные - нули
                             dlyLineAfter,     // размер исходных данных в ОТСЧЕТАХ
                             dst );
               *dstSize += dlyLineAfter;   // индицируем вновь появившиеся отсчеты
                finalized = true;          // финализируем обработку
            }
        } else {
           *dstSize = 0;
        }
    }
    catch(...) {
       *dstSize = 0;
        outputErr( "Ошибка фильтрации", "" );
    }
}
//---------------------------------------------------------------------------
void RealFIR::filter(const qint16 *src, int srcSize, qint16 *dst, int *dstSize, bool fin)
{
    const qint16    *srcPtr = src;
    qint16          *dstPtr = dst;
    int             outSize;
    int             partSize;

    (*dstSize) = 0;

    while( srcSize > 0  ) {
        partSize    = qMin( BUF16, srcSize );
        srcSize    -= partSize; // уменьшаем порцию входных данных

     // преобразуем формат данных
        for(int i = 0 ; i < partSize ; ++i ) {
            src16Buf[i] = srcPtr[i];
        }
        srcPtr += partSize; // смещаем указатель

     // фильтруем
        filter( src16Buf,
                partSize,
                dst16Buf,
               &outSize,
               (srcSize==0) && fin ); // финализируем если это последняя порция  finalize==true

        (*dstSize) += outSize; // аккумулируем число выходных отсчетов

        // переносим выходные отчеты в выходной массив short
        for(int i = 0 ; i < outSize ; ++i ) {
            dstPtr[i] = dst16Buf[i];
        }
        dstPtr += outSize; // смещаем указатель
    }
}
//---------------------------------------------------------------------------
void RealFIR::internalFIR( const float   *src,
                           int           srcSize,
                           float         *dst )
{
    int     cntr;
    float   result;

    try {
        for( int i = 0 ; i < srcSize ; ++i ) {  // для каждого отсчета
         // первый отсчет
            result  = src[i]*taps[0];
            cntr    = firDlyPtr+1;
         // остальные FTapCnt-1 отсчетов
            for( int j = 0 ; j < tapCnt-1 ; ++j ) {
                if( cntr == tapCnt-1 ) {
                    cntr =  0;
                }

                result += firDly[cntr] * taps[j+1];
                cntr++;
            }
         // сохраняем отсчет в линию задержки
            firDly[firDlyPtr] = src[i];
            firDlyPtr--;
            if( firDlyPtr < 0 ) {
                firDlyPtr = tapCnt-2;
            }
         // сохраняем результат (именно здесь, т.к. если src=dst то при ином порядке будет ошибка)
            if( dst ) {
                dst[i] = result;
            }
        }
    }
    catch(...) {
        outputErr( "Ошибка внутреннего фильтра", "" );
    }
}
//---------------------------------------------------------------------------
void RealFIR::chebyshev_window( float beta, float *win_chReal, int sizeFFT ) {
    int     ind;
    float   alfa, arg, chisl;
    float   znam, val;
    float  *tmpF = NULL;

    try{
        tmpF = new float[sizeFFT];

     // обнуление выделенных массивов
        memset(win_chReal, 0, sizeof(float)*sizeFFT);

        val = (float)pow(10., beta / 20.);

        alfa = (float)cosh( log(val + sqrt(val * val - 1.)) / (sizeFFT - 1) );

        // заполнение массива
        for(ind=0; ind < sizeFFT; ind++ ) {
            // Функции Чебышева определяются по-разному в зависимости от
            // аргумента:
            //   Tn(w) = cos(n*arccos(w))    |w| < 1;
            //   Tn(w) = ch(n*arch(w))       |w| > 1.
            // При -1 <= w <= 1 они выражаются через тригонометрические функции,
            // а для других значений аргумента через гиперболические (см. А.Анго
            // "Математика для электро- и радиоинженеров" стр.478).

            arg = (float)fabs( alfa * cos(M_PI * ind / sizeFFT) );

            if( -1 <= arg && arg <= 1 ) {
                chisl = (float)cos((sizeFFT - 1) * acos(arg));
            } else {
                chisl = (float)cosh( (sizeFFT - 1) * log(arg + sqrt(arg * arg - 1.)));
            }

            znam = (float)cosh( (sizeFFT - 1) * log(alfa + sqrt(alfa * alfa - 1.)) );
            win_chReal[ind] = chisl / znam;
        }

        // функция обратного преобразоания фурье
        realInvDft( win_chReal, sizeFFT, tmpF );
        // Нормировка окна
        for(ind = 1; ind < sizeFFT; ind++ ) {
            tmpF[ind] /= tmpF[0];
        }
        tmpF[0] = 1.f;

        memcpy( win_chReal + sizeFFT/2, tmpF,                  sizeof(float)*(sizeFFT/2 + 1) );
        memcpy( win_chReal,             tmpF  + sizeFFT/2 + 1, sizeof(float)*(sizeFFT/2    ) );
    }
    catch(...) {
        delete[] tmpF;
        throw("cheb window error");
    }
    delete[] tmpF;
}
//--------------------------------------------------------------------------
double RealFIR::sinc(double x)
{
    double result = sin(x);
    if( fabs(x) > 10e-200 ) {
        result /= x;
    } else {
        result = 1.;
    }
    if( qIsNaN(result) ) {
        throw("nan in sinc");
    }
    return result;
}
//--------------------------------------------------------------------------
