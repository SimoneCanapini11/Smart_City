import subprocess
import numpy as np
import scipy.stats as st

# Configurazione
eseguibile_c = "./msq_obj1_py.exe"  
seed_singolo = "12345"  

print("Avvio della simulazione (Metodo Batch Means)...")

# Lancia l'eseguibile una sola volta
comando = [eseguibile_c, seed_singolo]
processo = subprocess.run(comando, capture_output=True, text=True)

if processo.returncode != 0:
    print(f"Errore di esecuzione: {processo.stderr}")
    exit()
    
output = processo.stdout.strip()
probabilita_violazioni = []

try:
    # Il codice C stampa 50 valori separati da virgola
    dati = output.split(',')
    probabilita_violazioni = [float(x) for x in dati if x.strip()]
except Exception as e:
    print(f"Errore parsing output: {output}")
    exit()

# Analisi statistica 
if len(probabilita_violazioni) > 0:
    media = np.mean(probabilita_violazioni)
    errore_std = st.sem(probabilita_violazioni)
    
    # Intervallo di confidenza al 95% sui 50 blocchi indipendenti
    intervallo_confidenza = st.t.interval(0.95, df=len(probabilita_violazioni)-1, loc=media, scale=errore_std)
    
    print("\n" + "="*50)
    print(" RISULTATI DELL'ESPERIMENTO (STATO STAZIONARIO)")
    print(" Metodo: Batch Means (Medie a Blocchi)")
    print("="*50)
    print(f"Batch elaborati: {len(probabilita_violazioni)}")
    print("-" * 50)
    print(f"Prob. Media Violazione: {media:.4f}%")
    print(f"Intervallo Conf. (95%): [{intervallo_confidenza[0]:.4f}%, {intervallo_confidenza[1]:.4f}%]")
    print("="*50)
    
    if intervallo_confidenza[1] < 1.0:
        print("ESITO: SUCCESSO! Il vincolo (< 1%) e' soddisfatto.")
    else:
        print("ESITO: FALLIMENTO. L'intervallo supera l'1%.")