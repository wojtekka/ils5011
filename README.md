ils5011
=======

Program pozwala korzystać z symulatora pamięci EPROM ILS-5011 EPSIM polskiej firmy Inline Systems pod kontrolą systemu Linux. Egzemplarz symulatora, który posiadam, został wyprodukowany w 1995 roku. Nie udało mi się znaleźć żadnej wzmianki o tym produkcje w sieci, a tym bardziej oryginalnego oprogramowania. Niniejszy program powstał w całości dzięki inżynierii wstecznej. Komputer komunikuje się z symulatorem za pomocą portu równoległego. Na płytce znajdują się dwa układy pamięci 32 KB, co pozwala na symulację pamięci EPROM do 27512, oraz kilka układów logicznych i elementy dyskretne.

Po krótkiej analizie połączeń na płytce, stało się jasne, który sygnał kontrolny odpowiada za którą funkcję:

* Line Feed -- zatrzask dolnej połówki adresu
* Reset -- zatrzask górnej połówki adresu
* Strobe -- pin zapisu pamięci RAM
* Select -- wybór trybu pracy (symulacja/programowanie)

Program pozwala używać portu równoległego przez bezpośredni dostęp do portów wejścia-wyjścia lub przez urządzenie `/dev/parport`. Konieczne jest podanie rozmiaru symulowanej pamięci, żeby program mógł zapisać wiele kopii tego samego pliku, po to by starsze bity adresu (potencjalnie nie podłączone) nie miały wpływu na odczytywane dane.

Przykład użycia:

    ils5011 -s 16 test.bin

Program jest rozprowadzany na zasadach 3-klauzulowej licencji BSD (patrz plik LICENSE).
