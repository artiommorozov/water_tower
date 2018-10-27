[English](#en)

Управление насосом скважины с контролем уровня напроной башни по радио. На основе
микроконтроллера (MCU) atmega328 и usart радио модуля e32ttl100

1) Зачем

В небольшой деревне насос со скажиной и напорная башня расположены в километре друг от друга.
Использовать кабель для передачи сигнала уровня башни невозможно. Предыдущий вариант управления -
ручной контроль насоса и интуитивный контроль уровня башни

2) Внутри

Две части - для башни и для насоса. Исходный код для MCU в atmel studio 7 и схемы в kicad. Без печатных плат.
Этот проект предназначен в первую очередь для обслуживания существующей установки.

3) Часть башни

Работает на Atmega328 MCU с кварцем 1.84MHz для нажёности работы usart. Использует китайский клон модуля
E32TTL100. Электричества нет, поэтому часть питается от солнечных панелей и 12В свинцового аккумулятора.
Контроллер батарей - простой PWM из магазина. Преобразование напряжения на регуляторе L4940V5 по типовой схеме.

Датчик (манометр с контактной группой) соединяет входы MCU с GND. MCU большую часть времени спит, по таймауту
проверяет состояние датчика. Если уровень изменился, или не было подтверждения сигнала от насоса по прошлому
запросу, MCU отправляет новый запрос "много/мало воды". Запрос повторяется несколько раз пока не будет
получено подтвержение.

Светодиоды - D1 запуск, D2 - отправка, D3 - получено подтвержение.

4) Насос

Также работает на Atmega328 с кварцем 1.84Mhz, E32TTL100 для радио, DS18B20 для контроля температуры воздуха,
LCD экраном на ST7066U и твердотельным реле K293 для дальнейшего управления магнитным пускателем. Внешние модули
планого пуска и таймера на схеме не указаны.

MCU контролирет насос по запросу радио, по температуре и по времени. MCU ограничивает время работы насоса,
ограничивает время простоя насоса (в случае потери радио сигнала от башни); зимой насос периодически запускается
и сбрасывает воду в дренаж для защиты башни от образования льда.

MCU использует watchdog для контроля своего состояния. Таймер не учитывает переполнение корректно, поэтому
через ~100лет потребуется перезагрузка. К счастью, электричество пропадает значительно чаще.

Экран показывает историю работы по нажатию кнопки, от последнего действия к более старым.
Просто так показывает последнее действие.

5) Сборка

last_good - текущие собранные прошивки

Насосу нужна библиотека 1wire, используем [atmel AVR318 sample](http://www.microchip.com/wwwAppNotes/AppNotes.aspx?appnote=en591191) с некоторыми изменениями

```
Разархивируем  to src/pump/src
// common_files
// OWIBitFunctions.h
// OWIHighLevelFunctions.c
// OWIHighLevelFunctions.h
// OWIPolled.h
// OWISWBitFunctions.c
```

И применяем avr318.patch


### en

Radio control for village water pump on atmega328 and e32ttl100 radio usart modules

1) Why

A small village has water tower and a pump about 1km away. Using cables to connect tower level sensor to pump is not possible. Option used before radio was an oldman switching pump on and off manually few times a day.

2) What's inside

Two components: tower and motor (pump). Source code for microcontrollers in atmel studio 7 and some schematics in kicad. No printed boards. This contents is mainly intended to allow other people repair existing devices.

3) Tower side

Runs on Atmega328 MCU with 1.84MHz quartz to make usart communication reliable. Uses china clone of E32TTL100 radio module with usart interface. As there's no power supply at tower side MCU is powered from solar panels and 12v lead-acid accumulator, using simple PWM solar controller from store. Voltage conversion uses L4940V5 low-dropout regulator.

Water level sensor connects ground to MCU pins. MCU spends most time idle and wakes only to check sensor status from time to time. If level change needs pump action or pump didn't responds (either with ack or no level change occured), MCU requests on/off from pump side via radio (sends req and waits for ack a few times). Radio uses wake-up mode assuming motor part radio is asleep.

D1 is startup diagnostics LED, D2 indicates sending data, D3 - ack received.

4) Pump side

Runs on Atmega328 with 1.84Mhz quartz. Uses same E32TTL100 for radio, DS18B20 for air temp sense, ST7066U-based LCD and K293 solid-state 220V relay that's further controls powerful relay for switching pump. Schematics doesn't show timer relay (protects from MCU faults) and soft start module used to protect pump.

MCU monitors radio, temperature and time. It makes sure pump does no run for long periods, does not stay off for long periods (those items assume tower side is having problems but water must flow anyway); in winter pump should run for short periods spilling water to drainage to protect tower from forming ice.

MCU deploys watchdog to reset itself. MCU can't run for over 100 years because time counter doesn't account for overflow, still there's a power failure every year so that's not an issue.

LCD shows device operation history, activated with log button push it cycles through log items back. Otherwise just shows last action taken. Format is

Msg#: time passed since event, action taken
text

5) Building

Pump side needs 1wire library, it uses [atmel AVR318 sample](http://www.microchip.com/wwwAppNotes/AppNotes.aspx?appnote=en591191) with some changes

```
Extract following to src/pump/src
// common_files
// OWIBitFunctions.h
// OWIHighLevelFunctions.c
// OWIHighLevelFunctions.h
// OWIPolled.h
// OWISWBitFunctions.c
```

And apply avr318.patch there
