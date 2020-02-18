# GcHeapStat [![Build status](https://ci.appveyor.com/api/projects/status/3pcm9r3rai06g891?svg=true)](https://ci.appveyor.com/project/alpinskiy/gcheapstat/build/artifacts) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/3b99c9352dc7495383808c7824c0b420)](https://www.codacy.com/manual/malpinskiy/gcheapstat?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=alpinskiy/gcheapstat&amp;utm_campaign=Badge_Grade)

[**For English version click here.**](README_en.md)

Делает то же что и WinDBG/SOS команда "!dumpheap -stat", но без подключения отладчика к целевому процессу. По отношению к целевому процессу используется только ReadProcessMemory API.
## Использование
```
GCHEAPSTAT [/VERSION] [/HELP] [/VERBOSE] [/SORT:{+|-}{SIZE|COUNT}[:gen]]
           [/LIMIT:count] [/GEN:gen] [/RUNAS:LOCALSYSTEM] /PID:pid

  HELP     Display usage information.
  VERSION  Display version.
  VERBOSE  Display warnings. Only errors are displayed by default.
  SORT     Sort output by either total SIZE or COUNT, ascending '+' or
           descending '-'. You can also specify generation to sort on (refer to
           GEN option description).
  LIMIT    Limit the number of rows to output.
  GEN      Count only objects of the generation specified. Valid values are
           0 to 2 (first, second the third generations respectevely) and 3 for
           Large Object Heap. The same is for gen parameter of SORT option.
  RUNAS    The only currently available value is LOCALSYSTEM (run under
           LocalSystem computer account). This is to allow inspection of managed
           services running under LocalSystem (administrator account is not
           powerful enough for that).
  PID      Target process ID.

Zero status code on success, non-zero otherwise.
```
Требуется, чтобы разрядность GcHeapStat и целевого процесса совпадала. В AppVeyor можно найти gcheapstat32.exe и gcheapstat32.exe для x86 и x64 процессов соответственно (артифакт gcheapstatN.zip, N номер билда).
## Детали
### Почему не WinDBG/SOS?
Да, можно для этих целей использовать WinDBG (или любой другой отладчик). Но он не очень хорошо подходит для инспекции работающих на продакшене приложений:
1. Отладчик приостанавливает выполнение процесса. Подключиться отладчиком быстро, чтобы никто не заметил, скорее всего не выйдет - это занимает время даже если автоматизировать процесс.
1. Отладчик тянет за собой (убивает) процесс, если закрыть его в момент отладки (без отключения от отлаживаемого процесса). Если например забыть отсоединить отладчик перед выходом, или выполнить команду вроде ".kill", или ошибка в отладчике приведет к его краху, то отлаживаемый процесс также завершится. Это опасно.
1. Только одним отладчиком снять статистику с процесса, который работает под LocalSystem аккаунтом, нельзя. Даже у учетки администратора недостаточно прав для этого. Хочется иметь возможность и в этом случае решить задачу быстро, без дополнительных инструментов (ОК, единственным инструментом).
### Механизм работы
Получить информацию об внутреннем устройстве кучи без остановки процесса возможно благодаря следующим особенностям управляемой кучи:
1. Новые объекты добавлются всегда только в конец управляемой кучи (исключение составляет LOH, но большинство приложений не создают большой memory-traffic в LOH сегменте).
2. Расположение объектов в памяти меняется только в процессе compacting этапа работы GC, который занимает относительно немного времени (Microsoft стремится к тому, чтобы сборка мусора вообще занимала не больше чем требует PageFault).

Да, значения внутри объектов могут меняться. Да, могут меняться флаги в заголовке объекта. Но неизменны:
1. MethodTable объекта (нельзя поменять тип объекта)
1. Размер объекта (нельзя поменять размер объекта)

Соответсвенно, в целях перечисления типов-размеров объектов, большую часть времени кучу можно считать readonly структурой. 

GcHeapStat получает информацию об устройстве управляемой кучи через Data Access Layer (DAC) библиотеку, которую Microsoft поставляет с каждой версией CLR. Она предоставляет унифицированый интерфейс доступа к деталям управляемого процесса. DAC библиотека лежит в одной директории с рантаймом, поставляет вместе с ним, поэтому всегда доступна на машине, где выполняется .NET процесс. DAC используется в том числе отладчиком. Для получения DAC интерфейса достаточно уметь читать память целевого процесса. Целевой процесс открывается с правами "только чтение" (точнее ```PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ```), поэтому возможность ему навредить (намерено или по ошибке) исключена.
### Корректность работы
1. Все нестыковки оформляются сообщением об ошибке или предупреждении (вывод предупреждений можно включить флажком /VERBOSE). Например, проверяется что все сегменты управляемой кучи содержат объекты. Это, в свою очередь, означает, что по всем адресам, где начинается объект, мы должны увидеть валидный адрес MethodTable, плюс считать затем информацию о нем из DAC. Вероятность того, что по случайному адресу окажется валидная MethodTable (адрес в нативной куче) довольно мала.
1. Сравнение с выводом отладчика. Формат вывода совпадает с форматом команды "!dumpheap -stat", поэтому можно сравнивать в любом diff приложении. GcHeapStat можно запускать параллельно с отладчиком и в этом случае вывод должен полностью совпадать.
1. Я очень старался.
