import subprocess
import numpy as np
import scipy.stats as st
import matplotlib.pyplot as plt
import csv
import math

# Configurazione (Incidente)
eseguibile_c = "./msq_obj2_py.exe"  

# Parametri Procedura a Due Stadi
n0 = 20                 # Numero di repliche pilota (Stage 1)
w_desiderata = 0.5      # Semi-ampiezza desiderata dell'intervallo (es. 0.5%)
alpha = 0.05            # Livello di significatività (1 - 0.95 = 0.05)

print(f"Avvio di {n0} repliche per lo Scenario di Congestione Dinamica...")

# Liste per raccogliere i dati
tassi_offload = []
prob_violazioni = []
totali_offloaded = []
picchi_latenza = []
tempi_recupero_E = []
tempi_recupero_full = []
picchi_coda_N = []
picchi_coda_T = []

def esegui_replica(seed):
    """Funzione helper per eseguire una singola replica e salvare i dati"""
    comando = [eseguibile_c, str(seed)]
    processo = subprocess.run(comando, capture_output=True, text=True)
    
    if processo.returncode != 0:
        print(f"Errore al seed {seed}")
        return False
        
    output = processo.stdout.strip()
    try:
        dati = output.split(',')
        totali_offloaded.append(int(dati[1]))
        tassi_offload.append(float(dati[2]))
        prob_violazioni.append(float(dati[4]))
        picchi_latenza.append(float(dati[5]))
        
        rec_time_E = float(dati[6])
        if rec_time_E >= 0: tempi_recupero_E.append(rec_time_E)

        rec_time_full = float(dati[7])
        if rec_time_full >= 0: tempi_recupero_full.append(rec_time_full)

        picchi_coda_N.append(int(dati[8]))
        picchi_coda_T.append(int(dati[9]))
        return True
    except Exception as e:
        print(f"Errore parsing output {seed}: {output}")
        return False

# ==========================================
# STADIO 1: REPLICHE PILOTA
# ==========================================
print(f"--- STADIO 1: Avvio di {n0} repliche pilota ---")
for seed in range(1, n0 + 1):
    esegui_replica(seed)

# Calcolo della deviazione standard (s0) sulla metrica di riferimento
s0 = np.std(prob_violazioni, ddof=1)
t_val = st.t.ppf(1 - alpha/2, df=n0-1)

# Formula per il calcolo delle repliche necessarie (n)
n_stimato = math.ceil((t_val * s0 / w_desiderata)**2)
n_totale = max(n0, n_stimato)

print("\n" + "="*50)
print(" RISULTATI PROCEDURA A DUE STADI")
print("="*50)
print(f"Deviazione Standard (s0) stimata: {s0:.4f}")
print(f"Errore tollerato desiderato (w):  {w_desiderata}%")
print(f"Repliche totali necessarie (n):   {n_totale}")
print("="*50 + "\n")

# ==========================================
# STADIO 2: REPLICHE AGGIUNTIVE (se necessarie)
# ==========================================
if n_totale > n0:
    print(f"--- STADIO 2: Esecuzione di altre {n_totale - n0} repliche aggiuntive ---")
    for seed in range(n0 + 1, n_totale + 1):
        esegui_replica(seed)
else:
    print("--- STADIO 2 IGNORATO: Le repliche pilota sono già sufficienti! ---")


