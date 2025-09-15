#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

    /* Hjælpefunktion fra A0-kravet: skriv fejlformat til STDOUT */
static int print_error(const char *path, int errnum) {
    return fprintf(stdout, "%s: cannot determine (%s)\n", path, strerror(errnum));
}

bool checkforempty(unsigned bytesRead){
    bool x = true;
    if (bytesRead < 1){
        x  = false;
        return x;
    }
    else return x;
}

static inline bool is_ascii_allowed(unsigned char b) {
    if (b >= 0x20 && b <= 0x7E) return true;  // synlige ASCII
    if (b == 0x1B) return true;               // ESC
    if (b >= 0x07 && b <= 0x0D) return true;  // BEL, BS, TAB, LF, VT, FF, CR
    return false;
}

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
    return result;  // ASCII er 0x00 til 0x7F (0–127)
}



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

    int utf8_seq_len(unsigned char b0) {
        if ((b0 & 0x80) == 0x00) return 1;       // ASCII (0xxxxxxx)
            else if ((b0 & 0xE0) == 0xC0) return 2;  // 2-byte UTF-8 (110xxxxx)
            else if ((b0 & 0xF0) == 0xE0) return 3;  // 3-byte UTF-8 (1110xxxx)
            else if ((b0 & 0xF8) == 0xF0) return 4;  // 4-byte UTF-8 (11110xxx)
            else return -1;                           // ugyldig startbyte
        }

    bool is_continuation(unsigned char c) {
    // 0xC0 = 1100 0000 dermed er det en mask, som sammenlignes bliver 10xx xxxx og 
        return (c & 0xC0) == 0x80; // og 0x80 er 1000 0000, hvilket tjekker om seq[x] er kontinueret med 10
    }

    bool is_valid_UTF8(unsigned char seq[] , int len){
        if (len < 1)
    return false;

    unsigned char b0 = seq[0];
    //https://chatgpt.com/share/68c00ec5-50ac-800c-8351-8825e458c3a6 BIT WISE... 
    //DER DER IKKE NOGLE ISO, SOM VIL FALDE ELLER KRÆVER DET ANDET LOADNING
    if((b0 & 0x80)==0x00){
        //Vi tjekker om vi kun anvender 1 byte
        return len == 1;
    }

    // Hvis der bruges 2 byte så er bit mønsteret for 1 byte være: 110xxxxx og byte 2 vil være 10xxxxxx
    // Vi vil gerne have de tre første byte 
    // 0xE0 = 1110 0000 så der laves en bitwise and på b0, så vi udvælger de bit 7,6 og 5 i byte 1. 4,3,2,1,0 sættes til 0.
    // 5 bliver til 1, da den har 0 på pladsen som tjkkes mod 0 i bitwise, så resultatet bliver 1    
    //dernæst tjekkes de efter med tallet referenceværdien, som er 0xC0 = 11000000.
    if ((b0 & 0xE0) == 0xC0){
        //printf("%c\n", 'C');
        if (len != 2) return false;
        return is_continuation(seq[1]);
    }
    else if ((b0 & 0xF0) == 0xE0) {
        //0xF0 = 11110000
        //0xE0 = 11100000
        // 3-byte sekvens (1110xxxx 10xxxxxx 10xxxxxx)
        //printf("%c\n", 'C');
        if (len != 3) return false;
        return is_continuation(seq[1]) && is_continuation(seq[2]);
    } else if ((b0 & 0xF8) == 0xF0) {
        // 4-byte sekvens (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        //printf("%c\n", 'C');
        if (len != 4) return false;
        return is_continuation(seq[1]) && is_continuation(seq[2]) && is_continuation(seq[3]);
    } else
    //kna ikke kommme uden for unicode https://chatgpt.com/share/68bfd793-6508-800c-ad3b-f5b9f15ef8a2
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

    const char *path = argv[1];

    FILE *fp;

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        if(!fp){
            fprintf(stdout, "INFO: KUNNE IKKE ÅBNE FIL'%s'. Detteer forventet i denne test.\n", path);
        }
        perror("Kunne ikke åbne fil");
        print_error(path, errno);
        return EXIT_SUCCESS;
    }

    /*
    LÆSER FIL I BUFFER
    */
     //https://chatgpt.com/share/68bec5dc-b65c-800c-bbc3-68dfeb1d1dea
     unsigned char buf[1024];
    //UNSIGNED CHAR https://chatgpt.com/share/68bf16af-b390-800c-9be0-8c6b85c8b695
    //Husk retunere kun det antal læste byte
    size_t bytesRead = fread(buf, 1, sizeof(buf), fp);

    fclose(fp);

    if (checkforempty(bytesRead) == false){
        fprintf(stdout, "%s: empty\n", path);
        return EXIT_SUCCESS;
    }

    if (is_ascii(buf, bytesRead) == true){
        fprintf(stdout, "%s: ASCII text\n", path);
        return EXIT_SUCCESS;
    }

    if (is_iso8859(buf, bytesRead)) {
        fprintf(stdout, "%s: ISO-8859 text\n", path);
        return EXIT_SUCCESS;
    }

    bool utf8_valid = true;
    for (size_t i = 0; i < bytesRead; ) {
        int len = utf8_seq_len(buf[i]);
        if (len < 1 || i + len > bytesRead || !is_valid_UTF8(&buf[i], len)) {
            utf8_valid = false;
            break;
        }
        i += len; // spring frem til næste sekvens
    }
    if (utf8_valid) {
        // ekstra filter: nulbyte (0x00) og DEL (0x7F) er ikke tilladt i denne opgave
        for (size_t j = 0; j < bytesRead; j++) {
            if (buf[j] == 0x00 || buf[j] == 0x7F) {
                utf8_valid = false;
                break;
            }
        }
    }
    if (utf8_valid) {
        fprintf(stdout, "%s: Unicode text, UTF-8 text\n", path);
        return EXIT_SUCCESS;
    }

    fprintf(stdout, "%s: data\n", path);

    return EXIT_SUCCESS;
}
