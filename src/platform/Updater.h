#pragma once

// Aggiornamenti automatici tramite WinSparkle (WinSparkle.dll accanto all'exe).
// La DLL si carica a runtime: se manca, o se l'app è compilata senza URL degli
// aggiornamenti, l'app funziona normalmente e gli aggiornamenti sono disattivati.
//
// WinSparkle controlla una volta al giorno il file appcast.xml pubblicato con l'ultima
// release, chiede all'utente se aggiornare, scarica l'installer, ne verifica la firma
// EdDSA, chiude l'app e lancia l'installer.
namespace Updater {

// Da chiamare dopo aver mostrato la finestra principale.
void start();

// Da chiamare prima di uscire.
void stop();

bool isAvailable();

// Controllo manuale, con finestra di dialogo anche se non ci sono aggiornamenti.
void checkNow();

} // namespace Updater
