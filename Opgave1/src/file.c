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

    /* 
    (4) Læs hele filen byte for byte og tjek, om alt er tilladt A0-ASCII.
    fgetc(fp) giver næste byte som int (så det kan rumme EOF = -1).
    Vi caster til 'unsigned char' før check, så 0x80..0xFF ikke bliver negative. 
    */

    int ch;
    bool all_ascii = true;
    while ((ch = fgetc(fp)) != EOF) {          // læs til filens slutning (EOF)
        unsigned char b = (unsigned char) ch;  // gør klar til byte-sammenligning
        if (!is_ascii_allowed(b)) {            // fandt en "ikke-tilladt" byte?
            all_ascii = false;
            break;                             // Hvis vi når hertil er det ikke rent ASCII derfor stop loop
        }
    }

    // Hvis der skete en læsefejl, giv os en fejlmeddelelse og afslut.
    
    if (ferror(fp)) {
        print_error(path, errno);
        fclose(fp);
        return EXIT_SUCCESS;
    }

    // (5) Træk konklusion og skriv præcis én linje til STDOUT.

    if (all_ascii) {
        fprintf(stdout, "%s: ASCII text\n", path);
    } else {
        fprintf(stdout, "%s: data\n", path);
    }

    // Det er åbenbart vigtigt at lukke filen og afslut med succes.

    fclose(fp);
    return EXIT_SUCCESS;
}

