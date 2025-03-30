# iSpindel - *Elektroniczny Densymetr DIY*

**Polska wersja projektu iSpindel**

***
![Dashboard](/pics/blackedition.png)
***

## Polska wersja zmodyfikowanego oprogramowania iSpindel

Ten projekt to polska modyfikacja oryginalnego projektu [iSpindel](https://github.com/universam1/iSpindel) stworzonego przez universam1. Zawiera zmiany i usprawnienia dostosowane do potrzeb polskich piwowarów domowych.

***

## Dokumentacja w innych językach (z oryginalnego projektu)

### [English Documentation (work in progress)](docs/README_en.md) 🇺🇸
### [Nederlandse Vertaling (lopende werkzaamheden)](docs/README_nl.md) 🇳🇱
### [Documentação em Português (trabalho em andamento)](docs/README_pt.md) 🇧🇷
### [中文文档 (完善中)](docs/README_zh.md) CN

***

***Jeśli podoba Ci się projekt, postaw mi piwo*** :beers:

[![Donate](https://www.paypalobjects.com/pl_PL/PL/i/btn/btn_donate_LG.gif)](https://www.paypal.me/universam)

***

## [Pobierz najnowszą wersję oprogramowania](https://github.com/elroyski/ispindle_custom_firmware/releases)

***

## Zasada działania

Pomiar gęstości cieczy przez nachylenie pływającego cylindra jest równie prosty co genialny - nie potrzeba żadnego zewnętrznego odniesienia (poza grawitacją) a cylinder jest niezwykle łatwy do utrzymania w czystości. Kąt nachylenia zmienia się wraz z wyporności, co bezpośrednio koreluje z zawartością cukru w cieczy.
Nie trzeba otwierać fermentora, aby zmierzyć gęstość - brak ryzyka zanieczyszczenia!

![Nachylenie](/pics/kraengung.jpg)

Stąd pomysł, aby umieścić urządzenie IoT z modułem WiFi wraz z akcelerometrem i czujnikiem temperatury w pływającym cylindrze. Urządzenie budzi się co określony czas (np. co 30 minut), łączy się z siecią WiFi i wysyła dane o kącie nachylenia, temperaturze i napięciu baterii do usług chmurowych jak [Ubidots](https://www.ubidots.com) lub na lokalny serwer.

### *Metacentrum*

W rzeczywistości chodzi tutaj o "metacentrum" - cylinder będzie obracał się dopóki metacentrum nie znajdzie się dokładnie nad punktem wyporu. Tę wartość mierzymy.

Można dostroić urządzenie poprzez dodanie kilku gramów na dnie (wówczas cylinder będzie bardziej pionowy) lub na górze (wtedy będzie bardziej nachylony).

W oprogramowaniu obliczamy z wartości przyspieszenia XYZ kąt Eulera dla X i Y, z czego tworzymy kąt bezwzględny. Ten przeliczamy na °Plato przy pomocy skalibrowanych wartości.

***

## Budowa

### Warianty

> Wszystkie trzy warianty są funkcjonalnie identyczne

1. [Wariant A: Płytka uniwersalna i prowadnica](docs/iSpindelbreadboard.md)  
Płytka uniwersalna pozwala na budowę "domowymi sposobami" w każdej chwili
2. [Wariant B: Tylko PCB](docs/iSpindelPCB.md)  
Gotowa płytka PCB umożliwia wygodną budowę
3. [Wariant C: PCB i prowadnica](docs/iSpindelPCB.md)  
Powyższa płytka PCB może być (po odcięciu) używana w połączeniu z plastikową prowadnicą, co gwarantuje idealne dopasowanie.

### Części

***patrz [Części](docs/Parts.md)***

### Schemat układu

***patrz [Schemat](docs/Schaltplan.md)***

### Prowadnica

***patrz [Części](docs/Parts.md)***

***

## Konfiguracja

### Serwer testowy

[prosty serwer testowy](tools/TestServer/Readme.md)

### Ubidots

- Na początku należy utworzyć darmowe konto na [Ubidots.com](https://ubidots.com)
- W menu `API Credentials` otrzymasz swój `Token`, dzięki któremu iSpindel będzie miał uprawnienia do zapisywania danych.  
***Zapisz go.***  

![Token](/pics/UbiToken.jpg)

#### Portal

Poprzez wielokrotne naciśnięcie przycisku `Reset` Wemos tworzy punkt dostępowy (Access Point), z którym można się połączyć, aby skonfigurować niezbędne ustawienia.

> `iSpindel` sygnalizuje, że znajduje się w *trybie konfiguracji* poprzez ciągłe miganie w odstępach sekundowych.  
Tryb konfiguracji można opuścić zapisując ustawienia, wybierając opcję menu `Start iSpindel` lub czekając 5 minut. Następnie urządzenie przechodzi do *trybu pracy*, tzn. wysyła dane i przechodzi bezpośrednio w stan głębokiego uśpienia (Deep Sleep). W normalnym trybie nie jest więc dostępne.

- Token Ubidots ~~i `ID`~~, który zanotowałeś wcześniej, należy wpisać w odpowiednie pole.

- Należy również ustawić `Interwał`, w jakim będą wysyłane dane. Ma to bezpośredni wpływ na żywotność baterii. W praktyce zaleca się ustawienie około `1800` sekund (= 30 minut).

   ![Setup](/pics/configuration.png)


- Dostęp do portalu przez:

   ![AccessPoint](/pics/AP.png)![Portal](/pics/Portal.png)


- Przegląd danych można zobaczyć w zakładce `Info`

  ![Info](/pics/info.png)

Po wprowadzeniu i zapisaniu powyższych danych, Spindel połączy się z siecią WiFi i Ubidots, a następnie prześle dane.  
Jeśli nie ma jeszcze odpowiedniego urządzenia, zostanie ono automatycznie utworzone wraz z czujnikami.  
Na interfejsie Ubidots w zakładce `Sources` zobaczysz, że dane są aktualizowane.

Teraz możesz tworzyć dowolne wykresy na swoim `Dashboard`.

### BierBot Bricks

Konfiguracja z BierBot Bricks jest prosta i darmowa. Potrzebujesz do tego firmware iSpindel > `7.1.0`.

1. Najpierw musisz utworzyć darmowe konto. [Link do rejestracji](https://bricks.bierbot.com/#/register).
2. Po rejestracji kliknij "Bricks" w menu po lewej stronie (patrz 1. na zdjęciu).
3. Kliknij niebieski przycisk "Add Brick" / "Dodaj Brick" w prawym górnym rogu.
4. Wybierz zakładkę "iSpindel" w oknie i skopiuj klucz API do schowka.
5. Otwórz portal konfiguracyjny iSpindel (iSpindel tworzy punkt dostępowy po wielokrotnym naciśnięciu przycisku reset, patrz [tutaj](#portal))
6. Wybierz "BierBot Bricks" jako usługę (patrz 2 na zdjęciu).
7. Wklej klucz API ze schowka w pole "Token/ API key" i kliknij niebieski przycisk Zapisz na dole.
8. Wróć na [bricks.bierbot.com](https://bricks.bierbot.com/#/) i wybierz "Equipment" lub "Sprzęt browarniczy" po lewej stronie (3 na zdjęciu).
9. Utwórz nowe urządzenie (niebieski przycisk w prawym górnym rogu) typu "**Fermenter**".
10. Przypisz iSpindel do odpowiedniego miejsca (zaznaczonego na zielono) fermentora metodą **przeciągnij i upuść** jako czujnik brzeczki (opcjonalnie możesz również przypisać czujnik temperatury).
11. Kliknij "Save" / "Zapisz".
12. Aby rozpocząć rejestrację, potrzebujesz przepisu. Utwórz go w sekcji Przepisy (wystarczy jedna faza fermentacji), zapisz go i uruchom za pomocą pomarańczowego przycisku w przeglądzie przepisów.

**Gotowe!**

![Token](/pics/ispindle_bricks_tutorial.png)

### Skrypty

Ubidots obsługuje również skrypty w wielu miejscach. Dokumentacja funkcji znajduje się bezpośrednio na Ubidots. W ten sposób można na przykład usuwać niepotrzebne dane ze zmiennych.

- [patrz Ubidots-Scripting](docs/ubidotsscripting.md)

***

## Wyświetlanie

`iSpindel` może wysyłać dane do różnych systemów backendowych. Zaleca się rozpoczęcie od usługi chmurowej [Ubidots](https://www.ubidots.com), gdzie można utworzyć darmowe konto i wyświetlać wykresy. Można również przeprowadzać dodatkowe transformacje danych, takie jak obliczanie wartości Plato / SG, EVG itp.
Obecnie obsługiwane są następujące backendy:

### Wykresy Ubidots

- [Formuła Plato](docs/Kalibrierung.md#formel)

### CraftBeerPi

> Opis od @not:  
*Jeśli zainstalowałeś najnowszy firmware na swoim iSpindel, musisz również zaktualizować craftbeerpi: potrzebujesz do tego gałęzi "hydrometer". Gdy konfigurujesz Spindel, oprócz ustawień WiFi wybierz "CraftbeerPi" z menu rozwijanego i podaj adres IP serwera Craftbeerpi w tej samej sieci.
Następnie Spindel wysyła dane do Raspberry Pi, a Craftbeerpi automatycznie rejestruje Spindel. Jeśli połączenie działa, znajdziesz Spindel "jak za sprawą magii" w konfiguracji sprzętu CBP.
Zobaczysz, że dane przychodzą, gdy spojrzysz na swoją fermentację w CBP - wtedy widoczne są nie tylko temperatura (aktualna i zadana), ale także temperatura Spindela i wartość Plato (patrz zrzut ekranu powyżej) z ostatnią sygnaturą czasową.*  
`git clone -b hydrometer https://github.com/Manuel83/craftbeerpi.git`

### FHEM

- [upload do FHEM](docs/upload-FHEM.md)

### iSpindel-TCP-Server

- [https://github.com/DottoreTozzi/iSpindel-TCP-Server](https://github.com/DottoreTozzi/iSpindel-TCP-Server)

***

## Kalibracja

Do przeliczania zmierzonego **kąta ° na °Plato, SG, %mas** itp. konieczne jest początkowo skalibrowanie krzywej odniesienia.
Uzyskane referencje można następnie przekształcić w funkcję przy użyciu arkusza Excel lub [narzędzia kalibracji online](http://www.ispindel.de/tools/calibration/calibration.htm), która zostanie użyta do wyświetlania.

Ponieważ każdy samodzielnie zbudowany Spindel będzie dawał inne pomiary, proces ten należy przeprowadzić jednorazowo dla każdego iSpindel.

[patrz Przeprowadzanie kalibracji](docs/Kalibrierung.md)

***

## Oprogramowanie

### Wgrywanie firmware

- [Wgrywanie firmware](docs/Firmware.md)

### Kompilacja ze źródeł

- [Kompilacja ze źródeł](docs/compile.md)

***Jeśli Ci się podoba, postaw mi piwo***  :beers:

[![Donate](https://www.paypalobjects.com/pl_PL/PL/i/btn/btn_donate_LG.gif)](https://www.paypal.me/universam)

