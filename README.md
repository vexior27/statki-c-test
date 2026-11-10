# Projekt: Gra w Statki (C) - Battleship LAN & AI

Kompletna implementacja gry w statki w języku C.

## Funkcjonalności
* **Gra jednoosobowa (Singleplayer)**:
  - 3 poziomy trudności bota AI (Łatwy, Średni z algorytmem *Hunt & Target*, Trudny ze wzorem szachownicy).
  - Ręczne lub automatyczne (losowe) rozstawianie statków.
* **Gra wieloosobowa (Multiplayer)**:
  - Obsługa rozgrywki dla 2 graczy przez sieć **LAN** lub między **2 osobnymi terminalami** na tym samym komputerze (`127.0.0.1`).
  - Dedykowany protokół komunikacyjny TCP (uzgodnienie planszy, strzały, wymiana rezultatów, poddanie gry).
  - Automatyczne wykrywanie lokalnego adresu IP LAN i wyświetlanie go Hostowi.
* **Konfiguracja i interfejs**:
  - Konfigurowalny rozmiar planszy (od $6\times6$ do $20\times20$).
  - Automatyczne skalowanie floty do wielkości planszy.
  - Wyrazisty interfejs terminalowy (TUI) z kolorami ANSI (woda `~`, statek `#`, pudło `*`, trafienie `X`, zatopiony `X`).
  - Podwójny widok plansz obok siebie (plansza własna i plansza celowania we wroga).
  - Możliwość poddania gry i wyjścia do menu za pomocą `q` lub `'q'` w dowolnym momencie.

## Kompilacja i uruchomienie

### Szybka kompilacja za pomocą `make`:
```powershell
mingw32-make
.\statki.exe
```

### Uruchomienie testów jednostkowych:
```powershell
mingw32-make test
```

### Alternatywna kompilacja bezpośrednio przez `gcc`:
```powershell
gcc -Wall -Wextra -std=c11 -Iinclude src/*.c -o statki.exe -lws2_32
.\statki.exe
```

## Jak zagrać w dwie osoby?

### Wariant 1: Dwa terminale na jednym komputerze (Localhost)
1. Otwórz pierwsze okno terminala i wpisz `.\statki.exe`.
2. W menu wybierz `[2] Rozgrywka dla 2 graczy - HOSTUJ`.
3. Otwórz drugie okno terminala i wpisz `.\statki.exe`.
4. W menu wybierz `[3] Rozgrywka dla 2 graczy - DOLACZ` i naciśnij Enter (domyślnie połączy się z `127.0.0.1`).

### Wariant 2: Dwa osobne komputery w sieci lokalnej (LAN)
1. Na komputerze 1 uruchom `.\statki.exe` i wybierz opcję `[2] HOSTUJ`. Program wyświetli Twój adres IP (np. `192.168.1.55`).
2. Na komputerze 2 uruchom `.\statki.exe`, wybierz opcję `[3] DOLACZ` i wpisz adres IP wyświetlony na komputerze 1.
