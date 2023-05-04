# Адаптер климата Honda Civic 5D 2006-2011 (8 Gen)
ПО адаптера климат-контроля, спасибо [Алексеем OdinokijKot](https://vk.com/odinokij_kot) за разбор протокола общения климата, [подробности тут](https://www.civic-club.ru/f/honda-civic-5d/190506-adapter-klimata-svoimi-rukami/)!

# To-Do

* Требуется рефактор (разделить основной цикл на методы извлечения данных из сырых данных климата, сборка объекта Json в отдельном методе) - Pull Request-ы приветствуются. 
* Добавить поддержку других плат



# Текущие конфигурации / Протестированные устройства
* Arduino Uno / Nano - прекрасно работают
* Arduino Pro Micro 32u4 16Mhz 5V - не работает с [библиотекой для работы с последовательным портом на Android-е](https://github.com/mik3y/usb-serial-for-android)



# Конфигурация прошивки
В шапке прошивки имеются следующие макросы:
```
define CLK 2 // Красный #4 провод в фишке климата (DISP CLK)
#define DAT 5 // Жёлтый #5 провод в фишке климата (AC SO)
#define IS_SLAVE false // Заменить false на true для переключения в режим slave (подключение в параллель с Connects 2 или штатным ГУ) 
```
Ими выбираются выводы микроконтроллера (в моем случае - Arduino Uno). 2-ой контакт ардуины подключается к красному проводу фишки климата, 5-ый - к желтому. 
Макрос IS_SLAVE может быть либо true - если климат подключается в параллель с другим адаптером климата или штатным ГУ, либо false (случай большинства) - если адаптер самобытен. 
В случае, если адаптер работает в Slave-режиме, он не генерирует задержки, а ожидает их на входе от другого адаптера, после чего считывает показания. 

# Подключение без обвязки
Если адаптер будет подключен к ГУ автомобиля по USB, которое уже и так имеет общую массу, то можно подключить всего два контакта фишки климата (от которой идет всего два провода - желтый и касный) к микроконтроллеру в соответствии с прошивкой (по умолчанию ко 2 пину подключается красный, к 5-ому - желтый)
![Пример подключения к nano/uno](/connection-example.png)

P.S. Внимание! Такое подключение в теории не рекомендуется - по-хорошему нужно изолировать блок климата от адаптера. Но так как мы собираем его из того, что было, мы это пока  проигнорируем. К тому же, UNO за 300 рублей не особо-то и жалко, но на всякий пишу: за убитые блоки климата/прочее я ответственности не несу, все на ваш страх и риск. 