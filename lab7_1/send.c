#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

// ── GPIO via sysfs ────────────────────────────────────────────────────────────
// Change this to whichever BCM GPIO pin your LED is wired to.
#define LED_GPIO "17"

// ── Morse timing (microseconds) ───────────────────────────────────────────────
// Standard Morse ratios:  dot=1  dash=3  intra-char gap=1  inter-char gap=3
// inter-word gap=7  (all multiples of DOT_US)
#define DOT_US       200000   // 0.2 s  → ~1 char/s at lab-default speed
#define DASH_US      (DOT_US * 3)
#define INTRA_US     DOT_US          // gap between dots/dashes in same letter
#define INTER_US     (DOT_US * 3)    // gap between letters
#define WORD_US      (DOT_US * 7)    // gap between words

// ── Morse table (A-Z, 0-9) ────────────────────────────────────────────────────
static const char *MORSE[] = {
    // A-Z
    ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....",
    "..",   ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.",
    "--.-", ".-.",  "...",  "-",    "..-",  "...-", ".--",  "-..-",
    "-.--", "--..",
    // 0-9
    "-----",".-","..---","...--","....-",".....","-....","--...","---..","----."
};
// Index helpers
static int morse_index(char c) {
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    return -1;
}

// ── GPIO helpers ──────────────────────────────────────────────────────────────
static void gpio_write(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) { perror(path); exit(1); }
    write(fd, val, strlen(val));
    close(fd);
}

static void gpio_setup(void) {
    // Export pin
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {          // ignore EBUSY (already exported)
        write(fd, LED_GPIO, strlen(LED_GPIO));
        close(fd);
        usleep(100000);     // let the kernel create the files
    }
    // Set direction to output
    gpio_write("/sys/class/gpio/gpio" LED_GPIO "/direction", "out");
    // Start with LED off
    gpio_write("/sys/class/gpio/gpio" LED_GPIO "/value", "0");
}

static void led_on(void)  { gpio_write("/sys/class/gpio/gpio" LED_GPIO "/value", "1"); }
static void led_off(void) { gpio_write("/sys/class/gpio/gpio" LED_GPIO "/value", "0"); }

// ── Morse sending ─────────────────────────────────────────────────────────────
static void send_symbol(char sym) {
    led_on();
    usleep(sym == '.' ? DOT_US : DASH_US);
    led_off();
}

static void send_char(char c) {
    int idx = morse_index(c);
    if (idx < 0) return;            // unsupported character – skip silently
    const char *seq = MORSE[idx];
    for (int i = 0; seq[i]; i++) {
        send_symbol(seq[i]);
        if (seq[i + 1]) usleep(INTRA_US);   // gap between symbols
    }
}

static void send_message(const char *msg) {
    int first_char = 1;
    for (int i = 0; msg[i]; i++) {
        char c = msg[i];
        if (c == ' ') {
            usleep(WORD_US);
            first_char = 1;
        } else {
            if (!first_char) usleep(INTER_US);
            send_char(c);
            first_char = 0;
        }
    }
}

// ── Morse pretty-print (mirrors the lab example output) ──────────────────────
static void print_morse(const char *msg) {
    int first = 1;
    for (int i = 0; msg[i]; i++) {
        char c = msg[i];
        if (c == ' ') {
            printf(" /");
            first = 1;
        } else {
            int idx = morse_index(c);
            if (idx < 0) continue;
            if (!first) printf(" ");
            printf("%s", MORSE[idx]);
            first = 0;
        }
    }
    printf("\n");
}

// ── main ──────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <count> \"<message>\"\n", argv[0]);
        fprintf(stderr, "  e.g: %s 4 \"hello ESP32\"\n", argv[0]);
        return 1;
    }

    int count = atoi(argv[1]);
    if (count <= 0) {
        fprintf(stderr, "Error: count must be a positive integer.\n");
        return 1;
    }
    const char *msg = argv[2];

    gpio_setup();

    for (int i = 0; i < count; i++) {
        print_morse(msg);
        send_message(msg);
        if (i + 1 < count) usleep(WORD_US * 2);  // pause between repetitions
    }

    led_off();
    return 0;
}