#ifndef LTLFP240_H
#define LTLFP240_H

#ifndef SET_U_LOW
#define SET_U_LOW       2550
#endif
//Те 40.80В на батарее
//Минимальное напряжение на элементе 2500 по паспорту
//На батарее 40V

#ifndef SET_U_MIN_BAL
#define SET_U_MIN_BAL   3340
#endif
//те буферное напряжение- соответственно для батареи = 53,44V
//Переход в буферный режим при 3400 на элементе и напряжении батареи = 54,40В

#ifndef SET_U_MAX_BAL
#define SET_U_MAX_BAL   3650
#endif
//Те 58,4 В на батарее
//Максимальное напряжение на элементе 3700 по паспорту
//Те 59,2В на батарее

#ifndef SET_U_MIN_IND
#define SET_U_MIN_IND   3000
#endif
//напряжение 95% разряда при 0.2С

#ifndef SET_U_MAX_IND
#define SET_U_MAX_IND   3300
#endif
//напряжкние 5% разряда при 0.2с

#endif // LTLFP240_H
