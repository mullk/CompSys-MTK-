#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

// https://chatgpt.com/share/68beb497-6264-800c-88e1-0a33dcceb882
void tjekforempty(FILE *fp){
    //fgetc er en funktion i C, som læser ét enkelt tegn ad gangen fra en fil. 
    int c = fgetc(fp);
    //EOF er end of file
    if (c == EOF) {
        printf("Filen er tom.\n");
    } else {
        printf("Filen har indhold.\n");
    }
}

int main(int argc, char* argv[]) {
    //https://chatgpt.com/share/68beb497-6264-800c-88e1-0a33dcceb882
    printf("%s\n", argv[0]);
    printf("%s\n", argv[1]);
    if (argc < 2) {
        printf("Brug: %s <filnavn>\n", argv[0]);
        return 1;
    }

    FILE *fp;

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("Kunne ikke åbne fil");
        return 1;
    }

    tjekforempty(fp);

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        printf("Linje: %s\n", buffer);
    }


    fclose(fp);
    return 0;

}