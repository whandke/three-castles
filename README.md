# three-castles
Gra wykonana jako zadanie z lab. PW na Politechnice Poznańskiej.

# Jak uruchomić?
Skompilkuj obydwa pliki server.c i client.c samemu lub przy pomocy dołączonego skryptu.
Następnie uruchom program server.exec i 3 instancje programu client.exec. Kolejność uruchamianie nie ma znaczenia.

# Jak grać?
Używaj klawiszy W i S aby przemieszczać się po menu gry. Enterem potwierdzaj wybór.
Szkolenie jednostki dodaje zamówienie na jedną jednostkę wybranego typu. Kolejne zamówienia dokładane są na kolejkę zamówień. Jeżeli przy zamawianiu brakuje surowców, serwer przesyła odpowiednią wiadomość, która zostaje wyświetlona w oknie Info.
Obecny stan naszej krypty można obejrzeć w oknach Vault i Barracks.
Wysłanie ataku na wybranego gracza powoduje wysłanie wszystkich posiadanych wojsk. Atak trwa 5 sekund i następnie wyświetla się powiadomienie w oknie Info.
Z gry można wyjść wybierając opcję Exit.

# Kod
server.c - plik zawierający pełny kod źródłowy serwera. Kożysta z pamięci współdzielonej, aby przechowywać dane o obecnym stanie gry dla wielu podprocesów działających w ramach niego. Podprocesy tworzone są przy pomocy polecenia fork(). Podprocesy tworzą główne moduły aplikacji - Updater, Training, Production, Attack i proces Matka. Updater odpowiada za odświeżanie stanu gry z jednosekundowym okresem pracy. Aktualizuje stan złota, sprawdza czy warunek zwycięstwa nie został spełniony i wysyła dane do aplikacji graczy. Training odbiera zamówienia od graczy i przekazuje je do kolejki komunikatów, która pełni rolę kolejki produkcji. Z tej kolejki wiadomości odbiera modów Production. Odpowiada on za czas produkcji i dodanie jednostek odpowiedniemu graczowi. Moduł Attack oczekuje na rozkazy ataku otrzymywane od graczy, a następnie odpowiada za całą logikę i wynik walki. Moduł Matka nasłuchuje czy któryś z graczy nie zakończył gry przedwcześnie i odpowiada za zakończenie gry.

client.c - plik zawierający pełny kod źródłowy klienta. Kożysta z paimęci współdzielonej w podobny sposób jak serwer. Składa się z dwóch procesów, z których jeden odpowiada za odbieranie danych od serwera i odświeżanie wszystkich paneli poza menu wyboru, a drugi odpowiada za menu, i wysyłanie rozkazów do serwera. Klient wykorzystuje do działania bibliotekę ncurses.