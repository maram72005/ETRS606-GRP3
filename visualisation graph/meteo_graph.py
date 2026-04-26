"""
meteo_graph.py — Lecture port série STM32 + graphes temps réel (Historique 5h)
=============================================================================
"""

import sys
import re
import threading
import time
from collections import deque
from datetime import datetime

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.gridspec import GridSpec

# ── Configuration ────────────────────────────────────────────
BAUD        = 115200
# Calcul pour 5 heures : 5h * 60min * 30 mesures/min (si 1 mesure toutes les 2s)
MAX_POINTS  = 5 * 60 * 30  
INTERVALLE  = 2000 # Rafraîchissement graphique (ms)

CLASSES    = ['Pluie', 'Beau temps', 'Nuageux', 'Brouillard', 'Vent fort', 'Gel']
COULEURS_C = ['#E24B4A', '#378ADD', '#639922'] # temp, hum, pres
COULEURS_P = ['#378ADD', '#EF9F27', '#888780', '#7F77DD', '#1D9E75', '#D4537E']

# ── Données partagées ────────────────────────────────────────
lock      = threading.Lock()
horodates = deque(maxlen=MAX_POINTS)
temps     = deque(maxlen=MAX_POINTS)
hums      = deque(maxlen=MAX_POINTS)
press     = deque(maxlen=MAX_POINTS)
preds     = [deque(maxlen=MAX_POINTS) for _ in range(6)]

# ── Regex de parsing ─────────────────────────────────────────
RE_CAPTEURS = re.compile(r'Temp:\s*([-\d.]+).*?Hum:\s*([-\d.]+).*?Pres:\s*([-\d.]+)', re.IGNORECASE)
RE_CLASSE   = [re.compile(f'{c}\s*:\s*([\\d.]+)%', re.IGNORECASE) for c in CLASSES]

# ── Détection port ───────────────────────────────────────────
def detecter_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        desc = (p.description or '').lower()
        if any(k in desc for k in ['stm32', 'st-link', 'usb serial', 'uart']):
            return p.device
    return ports[0].device if ports else None

# ── Lecture Série ────────────────────────────────────────────
def lire_serie(port):
    global tampon_preds
    try:
        ser = serial.Serial(port, BAUD, timeout=2)
        print(f"[SERIE] Connecté sur {port}")
    except Exception as e:
        print(f"[ERREUR] : {e}"); return

    dans_bloc_ai = False
    tampon_preds = {}

    while True:
        try:
            ligne = ser.readline().decode('utf-8', errors='ignore').strip()
            if not ligne: continue

            # Capteurs
            m = RE_CAPTEURS.search(ligne)
            if m:
                with lock:
                    horodates.append(datetime.now().strftime('%H:%M:%S'))
                    temps.append(float(m.group(1)))
                    hums.append(float(m.group(2)))
                    press.append(float(m.group(3)))
                continue

            # Bloc IA
            if '=== METEO AI ===' in ligne:
                dans_bloc_ai = True
                tampon_preds = {}
            elif '================' in ligne and dans_bloc_ai:
                dans_bloc_ai = False
                with lock:
                    for i in range(6):
                        preds[i].append(tampon_preds.get(i, 0.0))
            elif dans_bloc_ai:
                for i, rx in enumerate(RE_CLASSE):
                    mm = rx.search(ligne)
                    if mm: tampon_preds[i] = float(mm.group(1)); break
        except: time.sleep(1)

# ── Graphique ────────────────────────────────────────────────
fig = plt.figure(figsize=(14, 8), facecolor='#1a1a2e')
gs = GridSpec(3, 2, figure=fig, hspace=0.5, wspace=0.3)

ax_temp = fig.add_subplot(gs[0, 0])
ax_hum  = fig.add_subplot(gs[1, 0])
ax_pres = fig.add_subplot(gs[2, 0])
ax_pred = fig.add_subplot(gs[:, 1])

def style_ax(ax, titre, couleur):
    ax.set_facecolor('#16213e')
    ax.set_title(titre, color=couleur, fontsize=10)
    ax.tick_params(colors='#aaaaaa', labelsize=8)
    ax.grid(True, color='#2a2a4a', linestyle='--')

style_ax(ax_temp, 'Température (°C)', '#E24B4A')
style_ax(ax_hum,  'Humidité (%)',    '#378ADD')
style_ax(ax_pres, 'Pression (hPa)',   '#639922')
style_ax(ax_pred, 'Prédictions IA (%) - Historique 5h', 'white')

line_t, = ax_temp.plot([], [], color='#E24B4A')
line_h, = ax_hum.plot([], [], color='#378ADD')
line_p, = ax_pres.plot([], [], color='#639922')
lines_preds = [ax_pred.plot([], [], color=COULEURS_P[i], label=CLASSES[i])[0] for i in range(6)]
ax_pred.legend(loc='upper left', fontsize=7, facecolor='#1a1a2e', labelcolor='white')

def update(frame):
    with lock:
        if not temps: return
        x = list(range(len(temps)))
        ho = list(horodates)
        
        line_t.set_data(x, list(temps))
        line_h.set_data(x, list(hums))
        line_p.set_data(x, list(press))
        for i in range(6): lines_preds[i].set_data(list(range(len(preds[i]))), list(preds[i]))

        for ax in [ax_temp, ax_hum, ax_pres, ax_pred]:
            ax.set_xlim(0, max(1, len(x)-1))
            if len(x) > 1:
                step = max(1, len(x)//8)
                ax.set_xticks(x[::step])
                ax.set_xticklabels(ho[::step], rotation=30)

    ax_temp.set_ylim(min(temps)-1, max(temps)+1)
    ax_hum.set_ylim(min(hums)-5, max(hums)+5)
    ax_pres.set_ylim(min(press)-2, max(press)+2)
    ax_pred.set_ylim(-5, 105)

if __name__ == '__main__':
    port = sys.argv[1] if len(sys.argv) > 1 else detecter_port()
    threading.Thread(target=lire_serie, args=(port,), daemon=True).start()
    ani = animation.FuncAnimation(fig, update, interval=INTERVALLE, cache_frame_data=False)
    plt.show()