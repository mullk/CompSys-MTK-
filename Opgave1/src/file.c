#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

    /* Hjælpefunktion fra A0-kravet: skriv fejlformat til STDOUT */
static int print_error(const char *path, int errnum) {
    return fprintf(stdout, "%s: cannot determine (%s)\n", path, strerror(errnum));
}

    /* 
    Tjek om en enkelt byte er "tilladt" i A0-ASCII:
    - 0x20..0x7E  = de synlige ASCII-tegn (mellemrum, bogstaver, tal, tegnsætning)
    - 0x1B        = ESC (escape)
    - 0x07..0x0D  = en lille gruppe kontroltegn (BEL, BS, TAB, LF, VT, FF, CR)
    Alt andet er IKKE tilladt i denne opgaves "ASCII text".
    (Det inkluderer bl.a. 0x00 (NUL), 0x0E..0x1A, 0x1C..0x1F, 0x7F (DEL), og 0x80..0xFF)
    */

static inline bool is_ascii_allowed(unsigned char b) {
    if (b >= 0x20 && b <= 0x7E) return true;  // synlige ASCII
    if (b == 0x1B) return true;               // ESC
    if (b >= 0x07 && b <= 0x0D) return true;  // BEL, BS, TAB, LF, VT, FF, CR
    return false;
}

    /* 
    Er filen tom – uden at "spise" første byte?
    Vi prøver at læse 1 byte. Hvis der ikke er nogen (EOF), er filen tom.
    Hvis der ER en byte, lægger vi den tilbage igen med ungetc, så efterfølgende
    læsning starter fra begyndelsen som intet var sket. 
    */

static bool tjekforempty(FILE *fp) {
    int c = fgetc(fp);               // prøv at læse 1 byte
    if (c == EOF) {
        return true;                 // tom fil
    }

    /* 
    Der var mindst én byte. Læg den tilbage, så næste læsning ser filen "urørt".
    Hvis ungetc mod forventning fejler, fortsætter vi bare (vi ser det så som "ikke tom"). 
    */
    
    if (ungetc(c, fp) == EOF) {   
        return false;
    }
    return false;                    // ikke tom
}

    /* 
    Bestem længden af en UTF-8 sekvens ud fra dens første byte:
    - 0xxxxxxx          = 1 byte (almindelig ASCII)
    - 110xxxxx          = 2 byte
    - 1110xxxx          = 3 byte
    - 11110xxx          = 4 byte
    Hvis første byte ikke matcher et gyldigt mønster, returnerer vi -1.
    */
    int utf8_seq_len(unsigned char b0) {
        if ((b0 & 0x80) == 0x00) return 1;       // ASCII (0xxxxxxx)
        else if ((b0 & 0xE0) == 0xC0) return 2;  // 2-byte sekvens
        else if ((b0 & 0xF0) == 0xE0) return 3;  // 3-byte sekvens
        else if ((b0 & 0xF8) == 0xF0) return 4;  // 4-byte sekvens
        else return -1;                          // ugyldig startbyte
    }

    /* 
    Tjek om en byte er en "continuation byte" i UTF-8.
    Disse skal altid starte med bitmønstret 10xxxxxx.
    */
    bool is_continuation(unsigned char c) {
        return (c & 0xC0) == 0x80;
    }

    /* 
    Valider en enkelt UTF-8 sekvens givet dens bytes:
    - ASCII: præcis 1 byte i intervallet 0xxxxxxx
    - 2-byte: start 110xxxxx efterfulgt af 1 continuation (10xxxxxx)
    - 3-byte: start 1110xxxx efterfulgt af 2 continuations
    - 4-byte: start 11110xxx efterfulgt af 3 continuations
    Hvis længden ikke passer eller der mangler continuation bytes,
    er sekvensen ugyldig.
    */


    bool is_valid_UTF8(unsigned char seq[], int len) {
        // Hvis længden er mindre end 1, kan det ikke være en gyldig sekvens
        if (len < 1) return false;

        // Første byte bruges til at afgøre hvilken type sekvens vi har
        unsigned char b0 = seq[0];

        // Hvis første byte er 0xxxxxxx, er det ASCII → præcis 1 byte
        if ((b0 & 0x80) == 0x00) {
            return len == 1; 
        } 
        // Hvis første byte er 110xxxxx, skal vi have 2 byte i alt:
        // én startbyte + én continuation (10xxxxxx)
        else if ((b0 & 0xE0) == 0xC0) {
            return len == 2 && is_continuation(seq[1]);
        } 
        // Hvis første byte er 1110xxxx, skal vi have 3 byte i alt:
        // én startbyte + to continuation bytes
        else if ((b0 & 0xF0) == 0xE0) {
            return len == 3 && is_continuation(seq[1]) && is_continuation(seq[2]);
        } 
        // Hvis første byte er 11110xxx, skal vi have 4 byte i alt:
        // én startbyte + tre continuation bytes
        else if ((b0 & 0xF8) == 0xF0) {
            return len == 4 
                && is_continuation(seq[1]) 
                && is_continuation(seq[2]) 
                && is_continuation(seq[3]);
        }

        // Hvis ingen af mønstrene passer, er sekvensen ugyldig
        return false;
    }

    /* 
    Tjek om en fil kan klassificeres som ISO-8859-1 (latin1).
    Vi gennemgår alle bytes i bufferen én for én:
    - Hvis byte er "tilladt" A0-ASCII (via is_ascii_allowed), er den ok.
    - Hvis byte er i intervallet 0xA0..0xFF, er det også ok (latin1-udvidelse).
    - Alt andet → ikke ISO-8859-1.
    */
    static bool is_iso8859(unsigned char *buf, size_t len) {
        // Gå gennem alle bytes i bufferen
        for (size_t i = 0; i < len; i++) {
            unsigned char b = buf[i];

            // Hvis byte er et af de tilladte ASCII-tegn → fortsæt
            if (is_ascii_allowed(b)) continue;

            // Hvis byte er 0xA0 eller højere → det er et gyldigt latin1-tegn
            if (b >= 0xA0) continue;

            // Hvis vi når hertil, har vi fundet en ugyldig byte
            return false;
        }

        // Hvis vi når hele vejen igennem uden fejl, er alt gyldigt ISO-8859-1
        return true;
    }


int main(int argc, char *argv[]) {

    /* 
    (1) Tjek korrekt brug:
    Programmet skal have PRÆCIS 1 argument: stien til filen.
    Hvis det ikke er tilfældet, fejlskriv "Usage: file path" til standard-fejl (stderr) og slut med kode 1.
    */
    
    if (argc != 2) {
        fprintf(stderr, "Usage: file path\n");
        return EXIT_FAILURE; // = 1
    }

    const char *path = argv[1]; // filstien vi skal undersøge

    // (2) Åbn filen i BINÆR tilstand ("rb"), så vi læser bytes som de er.
    FILE *fp = fopen(path, "rb");
    if (!fp) {
            fprintf(stdout,
                "INFO: Kunne ikke åbne fil '%s'. Dette er forventet i denne test.\n",
                path);

    // Kan ikke åbne? Så skriver vi fejlen til STDOUT og exit 0.
        perror(path);
        print_error(path, errno);
        return EXIT_SUCCESS; // = 0
    }

 
    /* 
    (3) Er filen tom?
    Svar og afslut, hvis ja. Ellers læser vi videre. 
    */

    if (tjekforempty(fp)) {
        fprintf(stdout, "%s: empty\n", path);
        fclose(fp);
        return EXIT_SUCCESS;
    }


    rewind(fp);  // flyt filposition tilbage til start, så vi kan læse hele indholdet fordi tjekforempty kan have læst 1 byte
    unsigned char buf[4096];  // buffer til at holde op til 4096 bytes
    size_t n = fread(buf, 1, sizeof(buf), fp);  // læs filens bytes ind i buf

    /* 
    (4) Tjek for ASCII:
    Vi går alle bytes igennem og ser, om de er "tilladt" A0-ASCII
    (dvs. synlige tegn, mellemrum, ESC og få kontroltegn).
    Hvis ALT er tilladt, så er filen ren ASCII.
    */
    bool ascii_ok = true;
    for (size_t j = 0; j < n; j++) {
        if (!is_ascii_allowed(buf[j])) { ascii_ok = false; break; }
    }
    if (ascii_ok) {
        fprintf(stdout, "%s: ASCII text\n", path);
        fclose(fp);
        return EXIT_SUCCESS;
    }

    /* 
    (5) Tjek for UTF-8:
    Vi scanner filen byte for byte og afgør længden på hver UTF-8 sekvens.
    - Hvis en startbyte er ugyldig, eller der mangler continuation bytes,
      så er det ikke gyldigt UTF-8.
    - Hvis hele bufferen kan opdeles i gyldige sekvenser, er den gyldig.
    */
    bool utf8_valid = true;
    for (size_t i = 0; i < n; ) {
        int len = utf8_seq_len(buf[i]);
        if (len < 1 || i + len > n || !is_valid_UTF8(&buf[i], len)) {
            utf8_valid = false;
            break;
        }
        i += len; // spring frem til næste sekvens
    }
    if (utf8_valid) {
        // ekstra filter: nulbyte (0x00) og DEL (0x7F) er ikke tilladt i denne opgave
        for (size_t j = 0; j < n; j++) {
            if (buf[j] == 0x00 || buf[j] == 0x7F) {
                utf8_valid = false;
                break;
            }
        }
    }
    if (utf8_valid) {
        fprintf(stdout, "%s: UTF-8 Unicode text\n", path);
        fclose(fp);
        return EXIT_SUCCESS;
    }

    /* 
    (6) Tjek for ISO-8859-1 (latin1):
    Hvis alle bytes enten er gyldig ASCII eller >= 0xA0, 
    så er filen ISO-8859-1 tekst.
    */
    if (is_iso8859(buf, n)) {
        fprintf(stdout, "%s: ISO-8859 text\n", path);
        fclose(fp);
        return EXIT_SUCCESS;
    }

    /* 
    (7) Alt andet falder igennem som "data":
    - Hvis der er kontroltegn udenfor ASCII
    - eller blandede/ugyldige sekvenser
    */
    fprintf(stdout, "%s: data\n", path);
    fclose(fp);
    return EXIT_SUCCESS;
}