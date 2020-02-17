# GcHeapStat [![Build status](https://ci.appveyor.com/api/projects/status/3pcm9r3rai06g891?svg=true)](https://ci.appveyor.com/project/alpinskiy/gcheapstat/build/artifacts) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/3b99c9352dc7495383808c7824c0b420)](https://www.codacy.com/manual/malpinskiy/gcheapstat?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=alpinskiy/gcheapstat&amp;utm_campaign=Badge_Grade)
Command line tool for generating GC heap statistics of a running .NET process.

WinDBG, в паре с расширением SOS(EX), хорошо справляется с задачей инспектирования .NET процесса. Но что если подключение отладчиком к процессу не вариант? Скажем нужно инспектировать запущеный на продакшене вебсайт.

GcHeapStat с помощью только операций чтения адреного пространства процесса, перебирает управляемую кучу и собирает статистику в формате вывода "!dumpheap -stat". Целевой процесс открывается с правами только на чтение, поэтому возможность ему навредить (намерено или из-за бага) исключена.
