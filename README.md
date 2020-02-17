# GcHeapStat [![Build status](https://ci.appveyor.com/api/projects/status/3pcm9r3rai06g891?svg=true)](https://ci.appveyor.com/project/alpinskiy/gcheapstat/build/artifacts) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/3b99c9352dc7495383808c7824c0b420)](https://www.codacy.com/manual/malpinskiy/gcheapstat?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=alpinskiy/gcheapstat&amp;utm_campaign=Badge_Grade)
Command line tool for generating GC heap statistics of a running .NET process.

GcHeapStat с помощью только операций чтения адресного пространства процесса, перебирает его управляемую кучу и собирает статистику в формате вывода WinDBG/SOS команды "!dumpheap -stat". Целевой процесс открывается с правами "только чтение", поэтому возможность ему навредить (намерено или из-за бага) исключена.

В реализации используются следующие моменты:
- Microsoft для каждой версии CLR поставляет Data Access Layer (DAC) библиотеку. Она предоставляет унифицированый интерфейс доступа к деталям рантайма управляемого процесса. DAC библиотека лежит в одной директории с рантаймом, поставляет вместе с ним, поэтому всегда доступна на машине, где выполняется .NET процесс. DAC используется в том числе отладчиком. Для получения DAC интерфейса практически достаточно только предоставить возможность читать память целевого процесса.
- Новые объекты добавлются всегда только в конец управляемой кучи (за исключением LOH)
- Расположение объектов в памяти меняется только при работе GC, который занимает относительно немного (Microsoft стремится к тому, чтобы сборка мусора занимала не больше чем требует PageFault)
