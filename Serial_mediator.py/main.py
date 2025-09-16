import requests
import time
import serial
from datetime import datetime

ser = serial.Serial("COM3", 9600, timeout=1)
time.sleep(2)
url = "http://127.0.0.1/Z-planning/api/"

matrice_led = [
    ["1A1", "00"], ["2A1", "01"], ["3A1", "02"], ["4A1", "03"], ["5A1", "04"], ["6A1", "05"], ["7A1", "06"], ["8A1", "07"],
    ["6A2", "10"], ["7A2", "11"], ["8A2", "12"], ["9A2", "13"], ["10A2", "14"], ["11A2", "15"], ["12A2", "16"], ["13A2", "17"],
    ["14A2", "20"], ["15A2", "21"], ["16A2", "22"], ["17A2", "23"], ["18A2", "24"], ["19A2", "25"], ["20A2", "26"], ["21A2", "27"],
    ["9A1", "30"], ["10A1", "31"], ["11A1", "32"], ["12A1", "33"], ["22A2", "34"], ["23A2", "35"], ["24A2", "36"], ["25A2", "37"],
    ["13A1", "40"], ["14A1", "41"], ["15A1", "42"], ["16A1", "43"], ["17A1", "44"], ["18A1", "45"], ["19A1", "46"], ["20A1", "47"],
    ["1A2", "50"], ["2A2", "51"], ["3A2", "52"], ["4A2", "53"], ["5A2", "54"],
    ["26A2", "60"], ["2A2", "61"], ["28A2", "62"], ["29A2", "63"], ["30A2", "64"],
]

def postazioni(id_badge: str) -> None:
    oggi = datetime.today()
    data_formattata = oggi.strftime("%Y-%m-%d")
    print(data_formattata)
    data = {
        "id_badge": id_badge,
        "data": data_formattata
    }
    try:
        response = requests.get(url+"getPrenotazioniPostazioniDateFromIdBadge.php", json = data)
        response.raise_for_status()
        risposta = response.json()
        print(risposta)

        if risposta['errore'] == "errore":
            msg_to_display = "D:ERRORE/badge non valido :("
            ser.write(msg_to_display.encode("utf-8"))
            print(msg_to_display)
            return

        msg_to_display = ""
        msg_to_display = "benvenuto " + risposta['username']
        if risposta['postazioni'] == "NONE":
            msg_to_display += "/    non hai nessuna/   oggi"
        else:
            msg_to_display += "/oggi hai prenotato/"
            postazioni = risposta['postazioni'].split(';')
            postazioni.pop()
            print("postazioni:  ", postazioni)

            for postazione in postazioni:
                msg_to_display += postazione + "  "

        msg_to_display = 'D:' + msg_to_display + '\n'
        print(msg_to_display)
        ser.write(msg_to_display.encode("utf-8"))
        ser.write("SI\n".encode("utf-8"))
        print(msg_to_display)
        time.sleep(0.2)

    except requests.RequestException as e:
        print("Errore nella richiesta:", e)
    return

def parcheggio(id_badge: str) -> None:
    oggi = datetime.today()
    data_formattata = oggi.strftime("%Y-%m-%d")
    data = {
        "id_badge" : id_badge,
        "data" : data_formattata
    }
    try:
        response = requests.get(url+"getPrenotazioniParcheggioDateFromIdBadge.php", json = data)
        response.raise_for_status()
        risposta = response.json()
        print(risposta)

        if risposta['errore'] == "errore":
            return

        if risposta['msg'] == "errore":
            return

        msg = ""
        if risposta['msg'] == "OK":
            msg = "SP"

            ser.write(msg.encode("utf-8"))

    except requests.RequestException as e:
        print("Errore nella richiesta:", e)
    return

def postazioniLibere() -> None:


    oggi = datetime.today()
    data_formattata = oggi.strftime("%Y-%m-%d")
    data = {
        "data":data_formattata
    }
    try:
        response = requests.get(url+"getPostazioniLibere.php", json = data)
        response.raise_for_status()
        risposta = response.json()
        print(risposta)

        if risposta['errore'] == "errore":
            return
        postazioni = risposta['postazioni'].split(';')
        print(postazioni)

        msg = "M"
        i = 0
        index_matrice = 0
        for postazione in postazioni:
            for coordinate in matrice_led:
                if coordinate[0] == postazione:
                    msg+=coordinate[1]


        print(msg)
        ser.write(msg("utf-8"))

    except requests.RequestException as e:
        print("Errore nella richiesta:", e)
    return

def cmdDetector(msg: str) -> None: #gli viene passata la stringa ricevuta in seriale, riconosce e esegue il comando
    cmd = msg.split(':')
    print(msg)
    if cmd[0] == "POS":
        postazioni(cmd[1])
        return
    if cmd[0] == "PAR":
        parcheggio(cmd[1])
        return
    return



#postazioniLibere()
time.sleep(1)

while True:

    if ser.in_waiting > 0:  # controlla se ci sono dati disponibili
        serial_msg = ser.readline().decode("utf-8").rstrip()
        cmdDetector(serial_msg)