# Simulatore Nodo Edge V2X - Analisi Prestazionale

Questo repository contiene il codice sorgente e gli script di automazione per la simulazione ad eventi discreti di un nodo Edge in un ecosistema Smart City. Il progetto modella le prestazioni del sistema a regime, valuta la resilienza durante una congestione transitoria (incidente stradale) e implementa una soluzione architetturale evoluta per garantire i vincoli SLA.

## Struttura del Repository

Il modello simulato, e le sue differenti configurazioni, sono situati all'interno della directory `Modello/`. Per ogni scenario analizzato, la rispettiva cartella contiene il motore di simulazione scritto in **C** e gli script **Python** utilizzati per automatizzare le esecuzioni, gestire i seed e raccogliere i dati.
All'interno della cartella `Test/` sono presenti diverse suite di verifica per garantire la robustezza del sistema e la coerenza dei risultati generati rispetto ai fondamenti teorici.

### Modello

*   **`Obiettivo1/`**
    *   Modello base utilizzato per l'analisi a regime (Steady-State). 
    *   Implementa la logica per la simulazione a orizzonte infinito utilizzando il metodo **Batch Means**.

*   **`Obiettivo2/`**
    *   Modello configurato per lo *Stress Test* su orizzonte temporale finito.
    *   Simula l'onda d'urto dell'incidente stradale. Lo script Python automatizza l'esecuzione di **Repliche Indipendenti** passando seed incrementali.
      
*   **`Migliorato/`**
    *   Evoluzione del modello.
    *   Implementa un Controller di Stato con logica di *Adaptive Trunk Reservation*. Include il codice per il limite dinamico dei server, il controllo a doppia soglia e il *Soft Recovery* post-crisi.

### Test

*   **`Convergenza/`**
    *   Script per la raccolta e l'analisi dei dati necessari a valutare la fase di riscaldamento (*Warm-up*). Permette di tracciare il grafico di convergenza per assicurare il raggiungimento delle condizioni di Steady-State.
      
*   **`Event_Tracing/`**
    *   Script dedicato al tracciamento passo-passo (log) degli eventi. Raccoglie e stampa lo stato delle variabili interne del sistema per verificare a mano la corretta evoluzione logica delle code e del server.

*   **`RNG/`**
    *   Test dei Generatori di Numeri Casuali (PRNG). Verifica che le sequenze e le variate generate rispettino fedelmente le distribuzioni statistiche attese per il traffico veicolare e i tempi di servizio.

*    **`Edge_Case/`**
     *   Test della risposta del sistema in configurazioni estreme e condizioni al limite.

*    **`Flow_Balance/`**
     *   Test dedicato ad assicurare la conservazione dei job all'interno del sistema. Verifica che le entità in ingresso corrispondano esattamente alla somma di quelle in uscita e di quelle ancora in elaborazione, accertando l'assenza di drop anomali o duplicazioni impreviste.

*    **`Validazione_Analitica/`**
      *    Script di validazione che confronta i risultati empirici della simulazione con i modelli della teoria delle code. Nello specifico, la suite testa la teoria dell'utilizzazione, la legge di Little e la conservazione del tempo di permanenza.

*   **`Indipendenza_Batch/`**
    *   Test che si occupa di calcolare l'autocorrelazione lag-1 sulle medie dei blocchi per confermare che i batch generati siano effettivamente indipendenti tra loro, e di trovare il cut-off lag validando la dimensione scelta per i batch.

*   **`Numero_Repliche/`**
    *   Script che esegue la procedura a due stadi per calcolare il numero di repliche necessarie a garantire la precisione statistica desiderata.
