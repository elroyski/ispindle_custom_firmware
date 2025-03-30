# iSpindel - *Elektroniczny hydrometr DIY*

**[www.ispindel.de](http://www.ispindel.de)** 

**Wersja polska piwo.org**

***
![Dashboard](/pics/blackedition.png)
***

## O projekcie

**iSpindel** to elektroniczny hydrometr DIY, który pozwala na ciągły monitoring procesu fermentacji piwa bez konieczności otwierania fermentora. Urządzenie pływa w brzeczce podczas fermentacji i przesyła dane o gęstości, temperaturze oraz napięciu baterii bezprzewodowo przez WiFi.

Projekt jest oparty na oryginalnym iSpindel stworzonym przez universam1, do którego dodaliśmy polskie tłumaczenie interfejsu oraz kilka modyfikacji zwiększających użyteczność urządzenia dla polskich piwowarów.

## Funkcje

- Pomiar gęstości brzeczki (w stopniach Plato lub SG)
- Pomiar temperatury
- Monitoring napięcia baterii
- Bezprzewodowe przesyłanie danych przez WiFi
- Długi czas pracy na baterii (2-4 tygodnie, w zależności od ustawień)
- Spolszczony interfejs użytkownika

## Gdzie znaleźć wersję skompilowaną?

Skompilowaną wersję firmware można znaleźć w zakładce [Releases](https://github.com/tomekjarosik/ispindle_custom_firmware/releases) tego repozytorium. Wystarczy pobrać plik `.bin` i wgrać go na swoje urządzenie.

## Jak wgrać firmware?

1. Pobierz najnowszą wersję firmware z [zakładki Releases](https://github.com/tomekjarosik/ispindle_custom_firmware/releases)
2. Podłącz iSpindel do komputera przez USB
3. W przeglądarce otwórz adres `http://192.168.4.1` (po podłączeniu iSpindel do zasilania)
4. Wybierz opcję "Konserwacja"
5. W sekcji "Aktualizacja firmware" wybierz pobrany plik .bin i kliknij "Aktualizuj"

## Jak skompilować firmware samodzielnie?

Projekt wykorzystuje PlatformIO jako środowisko programistyczne. Aby skompilować firmware:

1. Sklonuj to repozytorium
2. Zainstaluj [PlatformIO](https://platformio.org/)
3. Otwórz projekt w PlatformIO
4. Uruchom kompilację i wgrywanie poprzez: `pio run -t upload`

## Jak skonfigurować iSpindel?

1. Po pierwszym uruchomieniu iSpindel utworzy sieć WiFi o nazwie podobnej do "iSpindel123"
2. Połącz się z tą siecią hasło to "12345678"
3. W przeglądarce otwórz adres `http://192.168.4.1`
4. Skonfiguruj urządzenie według potrzeb (dane WiFi, serwer docelowy, interwał pomiarów)

## Wspierane serwery danych

- Ubidots
- HTTP Generic
- FHEM
- CraftBeerPi
- InfluxDB
- Prometheus Pushgateway
- MQTT
- ThingSpeak
- BierBot Bricks
- i inne...

## Kalibracja

Kalibracja jest niezbędna do prawidłowego pomiaru gęstości brzeczki. Instrukcję kalibracji znajdziesz na stronie [kalibracji](http://www.ispindel.de/tools/calibration/calibration.htm).

## Podziękowania

- Dla universam1 za stworzenie oryginalnego projektu iSpindel
- Dla społeczności piwo.org za wsparcie w tłumaczeniu i testach

## Licencja

> Wszelkie komercyjne powielanie lub wykorzystanie jest zasadniczo zabronione. Narusza to obowiązujące patenty.
>
> All rights reserved, any commercial use is hereby prohibited and will violate applicable patents.

***

## Kontakt

Jeśli masz pytania lub sugestie, skontaktuj się z nami przez:
- Forum [piwo.org](https://www.piwo.org)
- Github Issues

## Wsparcie projektu

Jeśli podoba Ci się ten projekt, rozważ wsparcie oryginalnego twórcy:

[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.me/universam)

