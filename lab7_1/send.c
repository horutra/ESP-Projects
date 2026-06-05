```c
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define CHIPNAME "gpiochip0"
#define GPIO_LINE 18

#define DOT_TIME 100000      // 100 ms
#define DASH_TIME 300000     // 300 ms
#define SYMBOL_GAP 100000
#define LETTER_GAP 300000
#define WORD_GAP 700000

struct Morse {
    char c;
    const char *code;
};

struct Morse morseTable[] = {
    {'A', ".-"},    {'B', "-..."}, {'C', "-.-."},
    {'D', "-.."},   {'E', "."},    {'F', "..-."},
    {'G', "--."},   {'H', "...."}, {'I', ".."},
    {'J', ".---"},  {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},    {'N', "-."},   {'O', "---"},
    {'P', ".--."},  {'Q', "--.-"}, {'R', ".-."},
    {'S', "..."},   {'T', "-"},    {'U', "..-"},
    {'V', "...-"},  {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"},  {'Z', "--.."},

    {'0', "-----"}, {'1', ".----"}, {'2', "..---"},
    {'3', "...--"}, {'4', "....-"}, {'5', "....."},
    {'6', "-...."}, {'7', "--..."}, {'8', "---.."},
    {'9', "----."}
};

const char* getMorse(char c)
{
    c = toupper(c);

    for (unsigned int i = 0; i < sizeof(morseTable)/sizeof(morseTable[0]); i++)
    {
        if (morseTable[i].c == c)
            return morseTable[i].code;
    }

    return NULL;
}

void led_on(struct gpiod_line *line)
{
    gpiod_line_set_value(line, 1);
}

void led_off(struct gpiod_line *line)
{
    gpiod_line_set_value(line, 0);
}

void sendSymbol(struct gpiod_line *line, char symbol)
{
    led_on(line);

    if (symbol == '.')
        usleep(DOT_TIME);
    else
        usleep(DASH_TIME);

    led_off(line);
    usleep(SYMBOL_GAP);
}

void sendMessage(struct gpiod_line *line, const char *msg)
{
    while (*msg)
    {
        if (*msg == ' ')
        {
            usleep(WORD_GAP);
        }
        else
        {
            const char *code = getMorse(*msg);

            if (code)
            {
                while (*code)
                {
                    sendSymbol(line, *code);
                    code++;
                }

                usleep(LETTER_GAP);
            }
        }

        msg++;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <count> \"message\"\n", argv[0]);
        return 1;
    }

    int count = atoi(argv[1]);
    char *message = argv[2];

    struct gpiod_chip *chip;
    struct gpiod_line *line;

    chip = gpiod_chip_open_by_name(CHIPNAME);
    line = gpiod_chip_get_line(chip, GPIO_LINE);

    gpiod_line_request_output(line, "morse", 0);

    for (int i = 0; i < count; i++)
    {
        sendMessage(line, message);
        usleep(1000000);
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
```
