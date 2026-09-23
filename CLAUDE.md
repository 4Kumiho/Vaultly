# Vaultly

App desktop C++ con interfaccia Qt per tenere traccia di entrate e uscite personali, con un'area cifrata per le password dei propri account.

## Obiettivo

Più utenti usano la stessa app (stesso DB locale). Ognuno fa login, gestisce uno o più conti, registra i movimenti di ogni conto e vede l'andamento del saldo nel tempo su un grafico zoomabile. Ha anche una sezione "Password" dove salva login e password dei suoi account.

## Funzionalità

### 1. Autenticazione
- **Schermata iniziale**: scelta tra *Login* e *Registrazione*.
- **Registrazione**: username + password + conferma password.
  - Le due password devono coincidere, altrimenti errore e niente registrazione.
  - Lo username deve essere univoco.
  - La password **non** viene salvata in chiaro: si salva hash + salt (PBKDF2, vedi sotto).
- **Login**: username + password, verifica contro l'hash salvato.

### 2. Conti
- Ogni utente può avere **più conti** (es. conto corrente, carta, contanti).
- Se l'utente non ha ancora conti (primo accesso), l'app gli fa creare subito il primo.
- **Creazione conto**: nome, **valuta** (scelta da lista: EUR, USD, GBP, CHF, JPY, …) e **saldo iniziale**.
- La valuta è fissata alla creazione e non si cambia dopo (i movimenti sono già in quella valuta).
- Il **saldo iniziale è modificabile** in seguito (per correggere errori di inserimento); cambiandolo si ricalcola tutto lo storico del saldo.
- Un conto si può **eliminare** dal pannello di modifica, con doppio click di conferma; il primo click avvisa di quanti movimenti andranno persi. I movimenti si cancellano a cascata (`ON DELETE CASCADE`).
- Saldo corrente del conto = saldo iniziale + somma entrate − somma uscite.
- Nessuna conversione tra valute: ogni conto mostra i suoi importi nella sua valuta.

### 3. Dashboard (sezione "Conti")
- Selettore del conto attivo.
- Mostra il **saldo attuale** del conto selezionato.
- Lista dei movimenti del conto (data, importo, tipo, categoria, descrizione).
- Pulsanti per aggiungere / modificare / eliminare un movimento, e per creare / modificare un conto.

### 4. Inserimento movimenti
- Tipo: **entrata** o **uscita**.
- Campi: importo, data (default: oggi), **categoria scelta da lista** (obbligatoria), descrizione (opzionale).
- Le categorie sono predefinite e divise per tipo:
  - Entrate: Stipendio, Regali, Rimborsi, Investimenti, Altro
  - Uscite: Casa, Spesa, Trasporti, Bollette, Salute, Svago, Ristoranti, Abbonamenti, Altro

- Data e ora del movimento modificabili (anche nel passato). Eliminazione con doppio click di conferma, senza popup.

### 5. Grafico del saldo
- Grafico a gradini del **saldo nel tempo** del conto selezionato (solo saldo, per ora), calcolato in `core/BalanceHistory`.
- Periodo selezionabile: **1G, 1S, 1M, 6M, 1A, Tutto** (default 1M). Il periodo filtra anche la lista dei movimenti e i totali entrate/uscite.
- Rotella = zoom sul tempo centrato sul mouse, trascinamento = spostamento, doppio click = torna al periodo scelto, passaggio del mouse = saldo in quel momento.
- Il saldo iniziale vale "prima di tutti i movimenti": un movimento inserito con data passata sposta tutto lo storico successivo.

### 6. Area password
- Sezione "Password", raggiungibile dal selettore **Conti | Password** nell'intestazione.
- Ogni voce: nome (obbligatorio), sito web, username/email, password, note. Serve almeno uno tra username e password.
- Ricerca per nome, utente o sito. Pulsanti "Copia utente" / "Copia password": gli appunti si svuotano dopo 30 s (se contengono ancora quel testo) e al logout; il contenuto viene marcato per essere escluso dalla cronologia appunti di Windows e dal cloud.
- Generatore di password (20 caratteri, con minuscole, maiuscole, cifre e simboli, senza caratteri ambigui).
- **Cifratura**: ogni voce è serializzata in JSON e cifrata per intero con AES-256-GCM (`core/Crypto`, API CNG di Windows). Chiave = PBKDF2-SHA256(password di login, `users.vault_salt`), diversa dall'hash di login. La chiave esiste solo in memoria nella `Session`, ricavata a login/registrazione.
- Conseguenza: **se l'utente dimentica la password, le voci non sono recuperabili**. Se un giorno si aggiunge il cambio password, bisogna ri-cifrare tutte le voci con la nuova chiave.

## Stack tecnico

| Cosa | Scelta |
|---|---|
| Linguaggio | C++17 |
| GUI | Qt 6.10.3 Widgets |
| Compilatore | MinGW-w64 13.1 (g++) |
| Build | CMake 4.x + Ninja |
| Database | SQLite tramite `QtSql` (driver `QSQLITE`), un file locale |
| Grafici | Qt Charts (`QChart`, `QLineSeries`, `QDateTimeAxis`) |
| Hash password | PBKDF2-SHA256 con salt casuale (`QPasswordDigestor`) |
| Cifratura area password | AES-256-GCM + `BCryptGenRandom` dalle API CNG di Windows (`bcrypt`, linkata in CMake) |
| Aggiornamenti | WinSparkle 0.9.4 (firma EdDSA) + GitHub Releases |
| Installer | Inno Setup 6 |

## Ambiente e build

Toolchain installata (Windows):
- Qt: `C:\Qt\6.10.3\mingw_64` (moduli base + Charts, installato con `aqtinstall`)
- MinGW: `C:\Qt\Tools\mingw1310_64\bin`
- Entrambi i `bin` sono nel PATH utente; `CMAKE_PREFIX_PATH=C:\Qt\6.10.3\mingw_64`

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
.\build\Vaultly.exe
ctest --test-dir build --output-on-failure
```

I test vanno lanciati con `ctest` (o dalla cartella `build`), **non** dalla root: lì ci sono le DLL Qt del deploy e l'eseguibile di test caricherebbe quelle, senza trovare il driver SQLite.

Eseguibile distribuibile nella root del progetto (exe + DLL Qt + cartelle dei plugin accanto):

```powershell
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target Vaultly
Copy-Item build-release\Vaultly.exe . -Force
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw Vaultly.exe
```

Dopo un clone, prima della build: `powershell -File scripts\fetch-deps.ps1` (scarica WinSparkle in `third_party/`, ignorata da git; hash SHA-256 verificato).

## Release e aggiornamenti automatici

- **Versione**: una sola fonte, `project(Vaultly VERSION x.y.z)` in `CMakeLists.txt` → `build/generated/Version.h` (da `src/Version.h.in`).
- **Aggiornamenti**: WinSparkle (`src/platform/Updater`), caricato a runtime da `WinSparkle.dll`. Controlla ogni 24 h `https://github.com/<repo>/releases/latest/download/appcast.xml`, chiede all'utente, scarica l'installer, **verifica la firma EdDSA**, chiude l'app e lancia l'installer in `/SILENT`; l'app riparte da sola. Senza `VAULTLY_GITHUB_REPO` (build di sviluppo) gli aggiornamenti sono disattivati.
- **Chiave privata di firma**: `%USERPROFILE%\.vaultly\update-signing.key`. **Mai nel repository** (`*.key` è in `.gitignore`). Va salvata anche altrove: se si perde, le app già installate non accetteranno più aggiornamenti. La chiave pubblica è in `CMakeLists.txt` e `scripts/release.ps1`.
- **Installer**: Inno Setup (`installer/Vaultly.iss`), installazione per utente in `%LOCALAPPDATA%\Programs\Vaultly`, senza UAC. `AppId` non va mai cambiato. Installazione e disinstallazione non toccano i dati in `%APPDATA%\Vaultly`.
- **Database degli utenti**: ogni installazione crea il proprio DB vuoto; non esiste un DB "di default" da distribuire. Le modifiche allo schema arrivano agli utenti tramite le migrazioni in `Database.cpp`, che partono al primo avvio della nuova versione: devono sempre conservare i dati esistenti.
- **Fare una release**: alzare la versione in `CMakeLists.txt`, committare, poi
  `powershell -File scripts\release.ps1 -Notes "novità 1`nnovità 2" -Publish`
  (compila con il repo di `scripts/release.json`, esegue i test, crea e firma l'installer, scrive `appcast.xml`, crea tag e release GitHub con `gh`). Senza `-Publish` prepara solo i file in `release/<versione>/`.

Il DB dell'app sta in `%APPDATA%\Vaultly\vaultly.db`. Lo schema è versionato con `PRAGMA user_version` (vedi `src/db/Database.cpp`): per modificarlo si aggiunge un nuovo step di migrazione, non si cambia quello esistente.

Moduli Qt da linkare: `Widgets`, `Sql`, `Charts`, `Network` (serve per `QPasswordDigestor`).

## Schema DB

```sql
PRAGMA foreign_keys = ON;

CREATE TABLE users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT    NOT NULL UNIQUE COLLATE NOCASE,
    password_hash BLOB    NOT NULL,
    salt          BLOB    NOT NULL,
    iterations    INTEGER NOT NULL,         -- iterazioni PBKDF2 usate per questo hash
    created_at    TEXT    NOT NULL,         -- ISO 8601
    vault_salt    BLOB                      -- v2: salt della chiave dell'area password
);

CREATE TABLE currencies (
    code        TEXT    PRIMARY KEY,        -- ISO 4217: 'EUR', 'USD', ...
    symbol      TEXT    NOT NULL,           -- '€', '$', ...
    minor_units INTEGER NOT NULL            -- cifre decimali: 2 per EUR, 0 per JPY
);

CREATE TABLE accounts (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id         INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name            TEXT    NOT NULL,
    currency_code   TEXT    NOT NULL REFERENCES currencies(code),
    initial_balance INTEGER NOT NULL,       -- in unità minime (centesimi), può essere negativo
    created_at      TEXT    NOT NULL,
    UNIQUE (user_id, name)
);

CREATE TABLE categories (
    id   INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT    NOT NULL,
    type TEXT    NOT NULL CHECK (type IN ('income', 'expense')),
    UNIQUE (name, type)
);

CREATE TABLE transactions (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    account_id  INTEGER NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    category_id INTEGER NOT NULL REFERENCES categories(id),
    amount      INTEGER NOT NULL CHECK (amount > 0),  -- in unità minime
    type        TEXT    NOT NULL CHECK (type IN ('income', 'expense')),
    description TEXT,
    occurred_at TEXT    NOT NULL            -- ISO 8601
);

CREATE INDEX idx_tx_account_date ON transactions(account_id, occurred_at);

-- v2
CREATE TABLE vault_entries (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id    INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    data       BLOB    NOT NULL,            -- nonce | JSON cifrato | tag (AES-256-GCM)
    created_at TEXT    NOT NULL,
    updated_at TEXT    NOT NULL
);
CREATE INDEX idx_vault_user ON vault_entries(user_id);
```

`currencies` e `categories` vengono popolate alla prima creazione del DB. Il `type` del movimento deve coincidere con il `type` della categoria (controllo lato codice).

## Regole importanti

- **Soldi come interi nelle unità minime della valuta** (`qint64`), mai `double`. La formattazione usa `minor_units` e `symbol` della valuta del conto, solo in visualizzazione.
- Formattazione e parsing degli importi passano sempre da `core/Money` (`Money::format`, `Money::parse`): non convertire a mano. Il parsing accetta sia `,` sia `.` come separatore decimale.
- Le scritture con validazione passano dai service (`AuthService`, `AccountService`, `TransactionService`, `VaultService`), non direttamente dai repository.
- **Area password**: nulla in chiaro nel DB, né su disco, né nei log. `VaultRepository` vede solo blob cifrati; cifratura/decifratura solo in `VaultService`. Non salvare mai `Session::vaultKey`.
- Date dei movimenti salvate come ora locale `yyyy-MM-ddTHH:mm:ss` (ordinabile come stringa).
- **Query sempre parametrizzate** (`QSqlQuery::prepare` + `bindValue`), mai concatenare stringhe SQL.
- Ogni accesso a conti/movimenti verifica che il conto appartenga all'utente loggato (`accounts.user_id`).
- `PRAGMA foreign_keys = ON` a ogni apertura della connessione (SQLite lo tiene spento di default).
- Logica separata dalla UI: le classi di accesso al DB e di calcolo non dipendono dai widget.

## Struttura progetto (proposta)

```
Vaultly/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── db/            # Database (apertura, migrazioni, seed) e un repository per tabella
│   ├── core/          # modelli (User/Session, Account, Transaction, Category, Currency, VaultEntry),
│   │                  # service con la validazione, BalanceHistory, Money, Crypto
│   ├── platform/      # integrazione con Windows: Updater (WinSparkle)
│   ├── ui/            # MainWindow → LoginPage | RegisterPage | HomePage (DashboardPage, VaultPage),
│   │                  # pannelli (SidePanel e derivati), Theme, Animations, Components, Toast
│   └── Version.h.in   # versione, URL e chiave pubblica degli aggiornamenti
├── assets/            # vaultly.png (1024) e vaultly.ico (16–256): icona originale dell'app
├── tools/make-icon/   # programma Qt che disegna l'icona e genera png + ico (non fa parte della build)
├── installer/         # script Inno Setup
├── scripts/           # fetch-deps.ps1, release.ps1, release.json
└── tests/             # un eseguibile Qt Test per area (auth, money, accounts, transactions,
                       # balance, crypto, vault)
```

## Interfaccia

- **Una sola finestra** (`MainWindow`): le pagine stanno in uno `SlideStack` e cambiano con uno scorrimento laterale. Dopo il login si entra in `HomePage`: intestazione (saluto, selettore Conti | Password, Esci) + un secondo `SlideStack` con le sezioni.
- Niente `QDialog` o popup: i moduli sono pannelli che entrano da destra con sfondo scurito. Un nuovo modulo deriva da `SidePanel` (vedi `AccountPanel`, `TransactionPanel`, `VaultPanel`) e ha come genitore la `HomePage` (`overlayHost`), così copre anche l'intestazione. Conferme brevi con `Toast`.
- Dashboard: schede dei conti in alto; sotto saldo + totali del periodo a sinistra, grafico (`BalanceChart`) a destra; lista movimenti (`TransactionList`) in fondo. La pagina scorre in verticale.
- Dopo ogni modifica al codice si rigenera anche `Vaultly.exe` nella root (build Release + `windeployqt`, vedi sotto).
- **Tema scuro** in `ui/Theme.cpp` (palette + QSS centralizzato). I widget non hanno stili inline: scelgono l'aspetto con proprietà dinamiche (`role`, `variant`, `tone`, `selected`) o `objectName`, documentate in `Theme.h`. Dopo aver cambiato una di queste proprietà a runtime serve `Components::repolish()`.
- **Animazioni** in `ui/Animations.h` (`fadeIn`, `shake`); gli importi importanti usano `AmountLabel`, che conta fino al nuovo valore.
- **Icona**: `assets/vaultly.ico` finisce nell'exe tramite `src/app.rc` e nell'installer (`SetupIconFile`); `assets/vaultly.png` è nelle risorse Qt (`:/assets/vaultly.png`) per la finestra e il logo di login/registrazione (`Components::brand`). Niente loghi o immagini di terzi senza licenza.
- Locale di default forzata a italiano (`main.cpp`), così date e importi sono coerenti con i testi.

## Domande aperte

- Trasferimenti tra conti (es. prelievo dal conto ai contanti): servono come tipo a parte?
- L'utente può aggiungere categorie proprie oltre a quelle predefinite?
- Serve una vista con il totale di tutti i conti (solo tra conti con la stessa valuta)?
