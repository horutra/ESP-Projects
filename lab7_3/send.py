from gpiozero import LED
from time import sleep
import sys

led = LED(18)

MORSE = {
    'A': '.-',    'B': '-...',  'C': '-.-.',
    'D': '-..',   'E': '.',     'F': '..-.',
    'G': '--.',   'H': '....',  'I': '..',
    'J': '.---',  'K': '-.-',   'L': '.-..',
    'M': '--',    'N': '-.',    'O': '---',
    'P': '.--.',  'Q': '--.-',  'R': '.-.',
    'S': '...',   'T': '-',
    'U': '..-',   'V': '...-',  'W': '.--',
    'X': '-..-',  'Y': '-.--',  'Z': '--..',
    '0': '-----', '1': '.----', '2': '..---',
    '3': '...--', '4': '....-', '5': '.....',
    '6': '-....', '7': '--...', '8': '---..',
    '9': '----.'
}

DOT = .075
DASH = .275

def send_symbol(symbol):
    led.on()

    if symbol == '.':
        sleep(DOT)
    else:
        sleep(DASH)

    led.off()
    sleep(DOT)

def send_message(msg):
    for ch in msg.upper():
        if ch == ' ':
            sleep(1)
            continue

        if ch in MORSE:
            for symbol in MORSE[ch]:
                send_symbol(symbol)

            sleep(0.75)

if len(sys.argv) != 3:
    print("Usage: python3 send.py <count> \"message\"")
    sys.exit(1)

count = int(sys.argv[1])
message = sys.argv[2]

for _ in range(count):
    send_message(message)
    sleep(1)