#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/// @brief Hjælpefunktion fra A0-kravet: skriv fejlformat til STDOUT
/// @param path er den sti, som er leveret via argv.
/// @param errnum er en heltalsværdi som repræsentere en systemsfejlkode
/// @return retunerer en string med beskrivelse af path og fejlkode
static int print_error(const char *path, int errnum) {
    return fprintf(stdout, "%s: cannot determine (%s)\n", path, strerror(errnum));
}

/// @brief Tjekker om der er læst mere end 1 byte. Derfor tjekker for tomme filer
/// @param bytesRead antal læste byte
/// @return sand eller falsk relativt til om læst mere end en byte
bool checkforempty(unsigned bytesRead){
    bool x = true;
    if (bytesRead < 1){
        x  = false;
        return x;
    }
    else return x;
}

/// @brief Hjælpefunktion til is_ascii
///        angiver om en given char overholder den udleverede standart fra A0 
///        {0x07, 0x08, . . . 0x0D} ∪ {0x1B} ∪ {0x20, 0x21, . . . , 0x7E}
/// @param b en byte sekvens som tjekkes
/// @return sand eller falsk relativt til om sekvensen lægger inden for opgavens intervaller
static inline bool is_ascii_allowed(unsigned char b) {
    if (b >= 0x20 && b <= 0x7E) return true;  // synlige ASCII
    if (b == 0x1B) return true;               // ESC
    if (b >= 0x07 && b <= 0x0D) return true;  // BEL, BS, TAB, LF, VT, FF, CR
    return false;
}

/// @brief tjekker om hele den læste byte sekvens holder sig inden for det tilladte Ascii interval.
/// @param buf peger på det første element i det array, som er indlæst via fread
/// @param length angiver hvor mange byte der er læst
/// @return sand eller falsk relativt til om alle char er inden for det givne Ascii interval.
bool is_ascii(unsigned char *buf, size_t length) {
    bool result = true;
    size_t k = 0;
    while (k < length)
    {
        if ((is_ascii_allowed(buf[k])) == true){
        k += 1;
        }
        else{
            result = false;
            return result;
        }

    }
    return result;
}


/// @brief tjekker om hele den læste byte sekvens holder sig inden for det tilladte ISO interval.
/// Her medtages jf. A0 intervallet fra is_ascii_allowed samt fra 160-255.
/// @param buf peger på det første element i det array, som er indlæst via fread
/// @param length angiver hvor mange byte der er læst
/// @return sand eller falsk relativt til om alle char er inden for det givne ISO interval.
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

    /// @brief Hjælpefunktion til is_valid_UTF8
    /// Her anvendes bit-wise and tjek, som linkes til i is_valid_UTF8
    /// Link: https://chatgpt.com/share/68c91e1f-08f0-800c-88af-540044f49de7
    /// @param b0 angive bytesekvensen
    /// @return talværdi relativt til de 8 byte, som UTF opbygges af. Dermed ved man om det er en
    /// sekvens med længde 1, 2, 3 eller 4.
    int utf8_seq_len(unsigned char b0) {
        if ((b0 & 0x80) == 0x00) return 1;       // ASCII (0xxxxxxx)
        else if ((b0 & 0xE0) == 0xC0) return 2;  // 2-byte UTF-8 (110xxxxx)
        else if ((b0 & 0xF0) == 0xE0) return 3;  // 3-byte UTF-8 (1110xxxx)
        else if ((b0 & 0xF8) == 0xF0) return 4;  // 4-byte UTF-8 (11110xxx)
        else return -1;                           // ugyldig startbyte
        }

    /// @brief Hjælpefunktion til is_valid_UTF8.
    /// Her tjekkes om UTF-8 mønster for byte 2, 3 og 4 følges.
    /// Her anvendes bit-wise and tjek, som linkes til i is_valid_UTF8
    /// @param c angive bytesekvensen
    /// @return bool om mønsteret overholdes eller
    bool is_continuation(unsigned char c) {
        return (c & 0xC0) == 0x80;
    }

    /// @brief Tjekker om en char er UTF-8
    /// Funktionen samt dens hjælpefunktion er udviklet gennem samtale med chatGpt:
    /// Link: https://chatgpt.com/share/68bf1b3e-4e8c-800c-ab71-9cb42d62db49
    /// I funktionen anvendes bit-wise tjek.
    /// Link: https://chatgpt.com/share/68c92229-d71c-800c-951d-f3291f6de704
    /// @param seq modtager en given bytesekvens relativt.
    /// @param len længden af bytesekvens relativt til UTF-8.
    /// @return sand eller falsk relativt til om det er UTF-8
    bool is_valid_UTF8(unsigned char seq[] , int len){
    if (len < 1)
    return false;
    unsigned char b0 = seq[0];

    // ekstra filter: nulbyte (0x00) og DEL (0x7F) er ikke tilladt i denne opgave
    if (b0 == 0x00 || b0 == 0x7F)
    return false;

    if((b0 & 0x80)==0x00){
        // 1-byte sekvens (0xxxxxxx)
        return len == 1;
    }

    if ((b0 & 0xE0) == 0xC0){
        // 2-byte sekvens (110xxxxx)
        if (len != 2) return false;
        return is_continuation(seq[1]);
    }
    else if ((b0 & 0xF0) == 0xE0) {
        // 3-byte sekvens (1110xxxx 10xxxxxx 10xxxxxx)
        if (len != 3) return false;
        return is_continuation(seq[1]) && is_continuation(seq[2]);
    } else if ((b0 & 0xF8) == 0xF0 && b0 <= 0xF4) {
        // 4-byte sekvens (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if (len != 4) return false;
        return is_continuation(seq[1]) && is_continuation(seq[2]) && is_continuation(seq[3]);
    } else
        return false;

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

    //Opretter en pointer til argumentet, som med gives.
    const char *path = argv[1];

    FILE *fp;

    // argv[1] filnavn, "rb" læse som bite. I Windows er "b" vigtig for binære filer,
    // da newline (\n) kan blive konverteret til \r\n. På Linux har "b" ingen effekt.
    // https://chatgpt.com/share/68c92229-d71c-800c-951d-f3291f6de704
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        if(!fp){
            fprintf(stdout, "INFO: KUNNE IKKE ÅBNE FIL'%s'. Detteer forventet i denne test.\n", path);
        }
        perror("Kunne ikke åbne fil");
        print_error(path, errno);
        return EXIT_SUCCESS;
    }

    /* LÆSER FIL TIL BUFFER */
    unsigned char buf[1024];
    //fread læser filens indhold og gennem det i buf. 1 angiver størrelsen på hvert element
    //sizeof(buf) angiver antal elementer der skal læses. fp angiver en pointer til den åbne fil.
    //Desuden returnere den antal læste byte, som gemmes i bytesRead.
    size_t bytesRead = fread(buf, 1, sizeof(buf), fp);

    //filen lukkes, da vi har læst dataen.
    fclose(fp);

    /*TJEKKER FOR TOM FIL.*/
    if (checkforempty(bytesRead) == false){
        fprintf(stdout, "%s: empty\n", path);
        return EXIT_SUCCESS;
    }

    /*TJEKKER FOR KUN ASCII*/
    if (is_ascii(buf, bytesRead) == true){
        fprintf(stdout, "%s: ASCII text\n", path);
        return EXIT_SUCCESS;
    }

    /*Tjekker for ISO*/
    if (is_iso8859(buf, bytesRead)) {
        fprintf(stdout, "%s: ISO-8859 text\n", path);
        return EXIT_SUCCESS;
    }

    /*Tjekker for UTF8*/
    bool utf8_valid = true;
    for (size_t i = 0; i < bytesRead; ) {
        int len = utf8_seq_len(buf[i]);
        // Returnere længende af bytesekvens relativ til UTF8
        // &buf[i] sender adressen på den sekevens, som vi tjekker.
        if (len < 1 || i + len > bytesRead || !is_valid_UTF8(&buf[i], len)) {
            utf8_valid = false;
            break;
        }
        // Grundet len ved vi, hvor mange sekvenser vi har tjekket. Derfor opdateres i, så vi
        // der springes frem til næste bytesekvens
        i += len;
    }
    if (utf8_valid) {
        fprintf(stdout, "%s: Unicode text, UTF-8 text\n", path);
        return EXIT_SUCCESS;
    }
    /*Hvis det ikke er Ascii, ISO eller UTF-8*/
    fprintf(stdout, "%s: data\n", path);

    return EXIT_SUCCESS;
}
