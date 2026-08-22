import pandas as pd
import matplotlib.pyplot as plt

# Caricamento dei dati
print("Caricamento dati in corso... (potrebbe richiedere qualche secondo)")
try:
    df = pd.read_csv("convergence.csv")
except FileNotFoundError:
    print("Errore: File convergence.csv non trovato.")
    exit()

# Configurazione del grafico
plt.figure(figsize=(12, 6))

# Tracciamento delle medie cumulative delle tre code
plt.plot(df['Time'], df['Avg_Queue_T'], label='Media Coda Telemetria (Bassa Prio)', color='orange', linewidth=1.5)
plt.plot(df['Time'], df['Avg_Queue_N'], label='Media Coda Navigazione (Media Prio)', color='blue', linewidth=1.5)
plt.plot(df['Time'], df['Avg_Queue_E'], label='Media Coda Emergenza (Alta Prio)', color='red', linewidth=1.5)

plt.title('Grafico della Convergenza - Medie Cumulative delle Code', fontsize=14, fontweight='bold')
plt.xlabel('Tempo di Simulazione (t)', fontsize=12)
plt.ylabel('Lunghezza Media Cumulativa (Lq)', fontsize=12)

# Limita l'asse X usando uno step di notazione scientifica se i numeri sono grandi
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))

plt.legend(loc='upper right')
plt.grid(True, linestyle='--', alpha=0.5)
plt.tight_layout()

plt.show()