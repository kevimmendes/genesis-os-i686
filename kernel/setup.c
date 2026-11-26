/*
 * Project Genesis - first-boot setup wizard.
 *
 * Three questions, one confirmation, no disk writes. The wording is repeated
 * for the three languages the wizard offers, because an installer that cannot
 * explain itself in the language you picked is not much of an installer.
 */

#include <drivers/keyboard.h>
#include <kernel/setup.h>
#include <lib/console.h>
#include <lib/string.h>

static struct setup_config config = {
    SETUP_LANGUAGE_PT,
    SETUP_KEYBOARD_US,
    10u,
};

static const char *const language_names[3] = {
    "Portugues",
    "English",
    "Espanol",
};

static const char *const keyboard_names[3] = {
    "US",
    "PT",
    "ES",
};

static const char *const t_invalid[3] = {
    "Escolha invalida.",
    "Invalid choice.",
    "Eleccion invalida.",
};

static const char *const t_keyboard_prompt[3] = {
    "Teclado:",
    "Keyboard layout:",
    "Distribucion de teclado:",
};

static const char *const t_disk_prompt[3] = {
    "Espaco para o sistema em GB (1-20):",
    "Space for the system in GB (1-20):",
    "Espacio para el sistema en GB (1-20):",
};

static const char *const t_range[3] = {
    "Escolha um numero entre 1 e 20.",
    "Choose a number between 1 and 20.",
    "Elige un numero entre 1 y 20.",
};

static const char *const t_summary[3] = {
    "Resumo da instalacao",
    "Installation summary",
    "Resumen de la instalacion",
};

static const char *const t_language_label[3] = {
    "Idioma   : ",
    "Language : ",
    "Idioma   : ",
};

static const char *const t_keyboard_label[3] = {
    "Teclado  : ",
    "Keyboard : ",
    "Teclado  : ",
};

static const char *const t_disk_label[3] = {
    "Disco    : ",
    "Disk     : ",
    "Disco    : ",
};

static const char *const t_confirm[3] = {
    "Instalar com estas opcoes? (s/n): ",
    "Install with these options? (y/n): ",
    "Instalar con estas opciones? (s/n): ",
};

static const char *const t_done[3] = {
    "Preferencias guardadas para esta sessao.\n"
    "O driver de disco (ATA) ainda nao existe nesta versao,\n"
    "por isso nada foi escrito no disco.\n"
    "\nA continuar para o sistema...\n",

    "Preferences stored for this session.\n"
    "The disk driver (ATA) does not exist in this version yet,\n"
    "so nothing was written to the disk.\n"
    "\nContinuing to the system...\n",

    "Preferencias guardadas para esta sesion.\n"
    "El controlador de disco (ATA) todavia no existe en esta version,\n"
    "por eso no se escribio nada en el disco.\n"
    "\nContinuando al sistema...\n",
};

static void print_line(const char *text)
{
    console_puts(text);
    console_puts("\n");
}

static uint32_t read_line(char *buffer, uint32_t capacity)
{
    uint32_t length = 0;

    buffer[0] = '\0';

    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n') {
            console_putc('\n');
            buffer[length] = '\0';
            return length;
        }

        if (c == '\b') {
            if (length > 0) {
                length--;
                console_puts("\b \b");
            }
            continue;
        }

        if (c < 32 || c > 126) {
            continue;
        }

        if (length + 1u >= capacity) {
            continue;
        }

        buffer[length++] = c;
        console_putc(c);
    }
}

static int parse_number(const char *text, uint32_t *value)
{
    uint32_t result = 0;

    if (*text == '\0') {
        return 0;
    }

    for (const char *cursor = text; *cursor != '\0'; cursor++) {
        if (*cursor < '0' || *cursor > '9') {
            return 0;
        }

        result = result * 10u + (uint32_t)(*cursor - '0');
        if (result > 1000u) {
            return 0;
        }
    }

    *value = result;
    return 1;
}

static void choose_language(void)
{
    char line[8];
    uint32_t value;

    for (;;) {
        console_clear();
        console_puts("Project Genesis - setup\n");
        console_puts("=======================\n\n");
        console_puts("Idioma / Language / Idioma:\n");
        console_puts("  1) Portugues\n");
        console_puts("  2) English\n");
        console_puts("  3) Espanol\n\n");
        console_puts("Escolha / Choice: ");

        read_line(line, sizeof(line));

        if (parse_number(line, &value) != 0 && value >= 1u && value <= 3u) {
            config.language = (int)(value - 1u);
            return;
        }

        print_line(t_invalid[config.language]);
    }
}

static void choose_keyboard(void)
{
    char line[8];
    uint32_t value;

    for (;;) {
        console_clear();
        print_line(t_keyboard_prompt[config.language]);
        console_puts("\n  1) US\n  2) PT\n  3) ES\n\n");
        console_puts("> ");

        read_line(line, sizeof(line));

        if (parse_number(line, &value) != 0 && value >= 1u && value <= 3u) {
            config.keyboard = (int)(value - 1u);
            return;
        }

        print_line(t_invalid[config.language]);
    }
}

static void choose_disk(void)
{
    char line[8];
    uint32_t value;

    for (;;) {
        console_clear();
        print_line(t_disk_prompt[config.language]);
        console_puts("\n> ");

        read_line(line, sizeof(line));

        if (parse_number(line, &value) != 0 && value >= 1u && value <= 20u) {
            config.disk_gb = value;
            return;
        }

        print_line(t_range[config.language]);
    }
}

/* Returns 1 when the user confirms, 0 when the wizard should start over. */
static int confirm(void)
{
    char line[8];

    for (;;) {
        console_clear();
        print_line(t_summary[config.language]);
        console_puts("========================\n\n");

        console_puts(t_language_label[config.language]);
        print_line(language_names[config.language]);

        console_puts(t_keyboard_label[config.language]);
        print_line(keyboard_names[config.keyboard]);

        console_puts(t_disk_label[config.language]);
        console_put_dec(config.disk_gb);
        console_puts(" GB\n\n");

        console_puts(t_confirm[config.language]);
        read_line(line, sizeof(line));

        if (line[0] == 's' || line[0] == 'S' || line[0] == 'y' || line[0] == 'Y') {
            return 1;
        }

        if (line[0] == 'n' || line[0] == 'N') {
            return 0;
        }

        print_line(t_invalid[config.language]);
    }
}

int setup_requested(const char *cmdline)
{
    if (cmdline == 0) {
        return 0;
    }

    /* Match the word "setup" between spaces, so "genesis.setup" does not count
     * by accident and both "setup" and "quiet setup" do. */
    const char *cursor = cmdline;

    while (*cursor != '\0') {
        while (*cursor == ' ') {
            cursor++;
        }

        const char *word = cursor;
        while (*cursor != '\0' && *cursor != ' ') {
            cursor++;
        }

        size_t length = (size_t)(cursor - word);
        if (length == 5u && strncmp(word, "setup", 5u) == 0) {
            return 1;
        }
    }

    return 0;
}

const struct setup_config *setup_config_get(void)
{
    return &config;
}

void setup_run(void)
{
    int done = 0;

    while (done == 0) {
        choose_language();
        choose_keyboard();
        choose_disk();
        done = confirm();
    }

    keyboard_set_layout(config.keyboard);

    console_clear();
    console_puts(t_done[config.language]);
}
