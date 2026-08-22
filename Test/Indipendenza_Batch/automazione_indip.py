import subprocess
import numpy as np
import scipy.stats as st

eseguibile_c = "./verifica_batch_indip_py.exe"
seed_singolo = "12345"

def calcola_autocorrelazione_lag1(dati):
    """
    Calcola il coefficiente di autocorrelazione lag-1 tra batch consecutivi.
    Ritorna un valore in [-1, 1]: vicino a 0 indica batch ~indipendenti.
    """
    n = len(dati)
    media = np.mean(dati)
    
    # Covarianza a lag 1: somma (x_i - x̄)(x_{i+1} - x̄)
    numeratore = sum((dati[i] - media) * (dati[i + 1] - media) for i in range(n - 1))
    # Varianza campionaria (denominatore, stesso stimatore della varianza usata per l'IC)
    denominatore = sum((x - media) ** 2 for x in dati)
    
    if denominatore == 0:
        return 0.0
    return numeratore / denominatore


def esegui_simulazione():
    comando = [eseguibile_c, seed_singolo]
    processo = subprocess.run(comando, capture_output=True, text=True)
    if processo.returncode != 0:
        print(f"Errore di esecuzione: {processo.stderr}")
        exit()
    output = processo.stdout.strip()
    try:
        dati = output.split(',')
        return [float(x) for x in dati if x.strip()]
    except Exception:
        print(f"Errore parsing output: {output}")
        exit()

def trova_cutoff_lag(dati_grezzi, soglia=None, max_lag=200):
    n = len(dati_grezzi)
    if soglia is None:
        soglia = 1.96 / np.sqrt(n)   # soglia di significatività al 95%
    
    media = np.mean(dati_grezzi)
    denom = np.sum((dati_grezzi - media) ** 2)
    
    for lag in range(1, max_lag + 1):
        num = np.sum((dati_grezzi[:-lag] - media) * (dati_grezzi[lag:] - media))
        rho = num / denom
        if abs(rho) < soglia:
            return lag  # primo lag in cui l'autocorrelazione scende sotto soglia
    return max_lag  # non trovato entro max_lag


print("Avvio della simulazione (Metodo Batch Means)...")
probabilita_violazioni = esegui_simulazione()

# --- VERIFICA INDIPENDENZA TRA BATCH ---
SOGLIA_CORRELAZIONE = 0.20   # soglia empirica comunemente usata nel libro di Leemis-Park
rho1 = calcola_autocorrelazione_lag1(probabilita_violazioni)

print(f"\nAutocorrelazione lag-1 tra batch: {rho1:.4f}")

if abs(rho1) > SOGLIA_CORRELAZIONE:
    print(f"ATTENZIONE: |rho1| = {abs(rho1):.4f} supera la soglia ({SOGLIA_CORRELAZIONE}).")
    print("I batch NON sono sufficientemente indipendenti.")
    print("Suggerimento: raddoppiare BATCH_SIZE (e dimezzare NUM_BATCHES) nel codice C, poi ripetere.")
else:
    print(f"OK: |rho1| = {abs(rho1):.4f} entro la soglia. Batch considerati indipendenti.")

# --- ANALISI STATISTICA ---
if len(probabilita_violazioni) > 0:
    media = np.mean(probabilita_violazioni)
    errore_std = st.sem(probabilita_violazioni)
    intervallo_confidenza = st.t.interval(0.95, df=len(probabilita_violazioni) - 1,
                                           loc=media, scale=errore_std)

    print("\n" + "=" * 50)
    print(" RISULTATI DELL'ESPERIMENTO (STATO STAZIONARIO)")
    print(" Metodo: Batch Means (Medie a Blocchi)")
    print("=" * 50)
    print(f"Batch elaborati: {len(probabilita_violazioni)}")
    print(f"Autocorrelazione lag-1: {rho1:.4f} ({'OK' if abs(rho1) <= SOGLIA_CORRELAZIONE else 'VIOLATA'})")
    print("-" * 50)
    print(f"Prob. Media Violazione: {media:.4f}%")
    print(f"Intervallo Conf. (95%): [{intervallo_confidenza[0]:.4f}%, {intervallo_confidenza[1]:.4f}%]")
    print("=" * 50)

    if intervallo_confidenza[1] < 1.0:
        print("ESITO: SUCCESSO! Il vincolo (< 1%) e' soddisfatto.")
    else:
        print("ESITO: FALLIMENTO. L'intervallo supera l'1%.")

latenze = np.loadtxt("latenze.csv")
print(f"Numero di latenze grezze salvate: {len(latenze)}")
 
L = trova_cutoff_lag(latenze)
print(f"Cut-off lag stimato: {L}")
print(f"Batch size minima raccomandata: 2 x {L} = {2*L} osservazioni")