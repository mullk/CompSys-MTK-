#!/usr/bin/env bash

# Exit immediately if any command below fails.
# set -e gør/betyder "Stop ved første fejl" – hvis en kommando fejler (exitkode ikke er 0), så stopper scriptet.
set -e

# Bygger vores file.c (Makefile sørger for gcc-flags m.m.)
make

### Samme kommandoer som i terminalen:
echo "Generating a test_files directory.."
mkdir -p test_files
rm -f test_files/*


echo "Generating test files.."

# Vi laver nogle små filer at teste på.
# - 'printf' skriver præcis den tekst/byte-sekvens vi giver den
# - '>' betyder "skriv output til denne fil" Vi giver derefter filnavnet.
#   (Hvis filen allerede findes, overskrives den. Hvis den ikke findes, oprettes den.)
# - '\n' er newline (line feed, LF, 0x0A)
# - '\t' er tab (horizontal tab, HT, 0x09)
# - '\r' er (carriage return CR, 0x0D)
# - '\xNN' er byte med hex-værdi NN (to hex-cifre)
# - "" (tom streng) er en fil uden indhold (0 byte lang)
















### SIMPLE TEST CASES ###
### Givet af opgaven ###
printf "Hello, World!\n"                              > test_files/ascii.input
printf "Hello, World!"                                > test_files/ascii2.input
printf "Hello,\x00World!\n"                           > test_files/data.input
printf ""                                             > test_files/empty.input

### Test cases jeg har tilføjet ###
### ASCII TEST CASES ###

### ASCII (tilladte bytes: 0x07..\x0D, 0x20..0x7E)
printf "Normal ASCII\n"                               > test_files/ascii_normal.input
printf "With space \n"                                > test_files/ascii_space.input
printf "With tab\t\n"                                 > test_files/ascii_tab2.input
printf "With newline\n\n"                             > test_files/ascii_nl2.input
printf "With carriage\rreturn\n"                      > test_files/ascii_cr.input
printf "Mix:\x07\x0A\x0D \x20\x7E\n"                  > test_files/ascii_mix.input

### ASCII (kontroltegn: \x07..\x0D og ESC \x1B)
printf "Line1\r\nLine2\r\n"                           > test_files/ascii_crlf.input
printf "TAB\tSEP\tOK\n"                               > test_files/ascii_tab.input
printf "BEL:\x07 OK\n"                                > test_files/ascii_bel.input
printf "\x1B[31mRED\x1B[0m\n"                         > test_files/ascii_esc.input
printf "Spaces only      \n"                          > test_files/ascii_spaces.input
printf "Unix\nNewlines\n"                             > test_files/ascii_unixnl.input
printf "Carriage\rOnly\r"                             > test_files/ascii_cr_only.input



### ISO TEST CASES ###

### ISO-8859-1 (latin1) – æøå og andre ud over ASCII (0xA0–0xFF)
printf "\xE6\xF8\xE5\n"                               > test_files/iso_aeoeaa.input    # æøå
printf "Héllo\n"                                      > test_files/iso_accent.input    # é
printf "Grüße\n"                                      > test_files/iso_umlaut.input    # ü
printf "Færøerne\n"                                   > test_files/iso_mix.input       # æø

### ISO-8859-1 (latin1) - franske ligaturer (0xE6, 0xC6)
printf "Cœur\n"                                       > test_files/iso_french_ligature.input   # œ
printf "Œuvre\n"                                      > test_files/iso_french_biglig.input     # Œ

### ISO-8859-1 (latin1) - Hollandsk IJ (0xC9, 0xE9)
printf "IJ\n"                                         > test_files/iso_dutch_ij_only.input  #
printf "IJsselmeer\n"                                 > test_files/iso_dutch_ij.input        # IJ

### ISO-8859-2 (latin2) – Ungarsk dobbelt accent (0xA0–0xFF)
printf "Őrült\n"                                      > test_files/iso_hungarian_ouml.input    # Ő
printf "Új\n"                                         > test_files/iso_hungarian_acute.input   # Ú

### ISO-8859-3 (latin3) – Maltese (0xA0–0xFF)
printf "Ħażiż\n"                                      > test_files/iso_maltese.input           # ħ + ż
printf "Łódź\n"                                       > test_files/iso_polish.input            # Ł + ź

### specielle cases
printf "¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿\n"             > test_files/iso_specials.input  # 0xA1..0xBF



### UTF-8 TEST CASES ###

### UTF-8 Unicode (multibyte sekvenser)
### Random sprog og symboler
printf "Hej 😀\n"                                     > test_files/utf8_emoji.input
printf "漢字\n"                                        > test_files/utf8_kanji.input
printf "Привет\n"                                      > test_files/utf8_cyrillic.input
printf "مرحبا\n"                                      > test_files/utf8_arabic.input
printf "Precomposed: Café\nCombining: Cafe\u0301\n"    > test_files/utf8_combining.input
printf "Ελληνικά\n"                                    > test_files/utf8_greek.input
printf "שלום\n"                                        > test_files/utf8_hebrew.input
printf "Math: ∑ ∞ √ ∫ ≤ ≥ ≠\n"                         > test_files/utf8_math.input
printf "Mix: Ångström naïve 中文 😀 Привет مرحبا\n"   > test_files/utf8_mix.input
printf "𐍈 𐌰 𐍂 (gotisk)\n"                             > test_files/utf8_gothic.input
printf "हिन्दी\n"                                        > test_files/utf8_hindi.input
printf "ไทย\n"                                         > test_files/utf8_thai.input
printf "हिन्दी और English और العربية और 中文\n"       > test_files/utf8_multi.input
printf "Combining: A\u030A O\u0308 U\u0308\n"          > test_files/utf8_combining2.input
printf "Zalgo: Z̸͂͋͆͠a̴̒̏͘l̵͝͝g̷͂͘ó̶͝\n"                               > test_files/utf8_zalgo.input
printf "Accents: àáâäæãåāèéêëēėęîïíīįìôöòóœøōõûüùúū\n" > test_files/utf8_accents.input
printf "Cyrillic: АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ\n" > test_files/utf8_cyrillic_full.input




### Andre data typer og cases ###

### Flere DATA typer (forbudte bytes: 0x00, 0x7F, 0x80..0x9F, mm.)

# null byte (0x00) midt i teksten. Dette er forbudt i både ASCII og ISO-8859-1 og invaliderer også en ren UTF-8 sekvens
printf "NUL@\x00mid\n"                                > test_files/data_null_mid.input 

# Starter direkte med en null byte (0x00). Tester at filen stadig registreres som data, selv hvis den begynder med ugyldigt indhold.
printf "\x00starts\n"                                 > test_files/data_null_start.input

# Indeholder DEL-tegnet (0x7F), som ikke er tilladt i ASCII-mængden fra opgaven og heller ikke i ISO-8859-1. Skal derfor klassificeres som data
printf "DEL:\x7F end\n"                               > test_files/data_del_7f.input

# Indeholder en blanding af forbudte bytes: 0x00, 0x7F og 0x80. Tester håndtering af filer med flere forskellige ugyldige tegn samtidigt.
printf "mix:\x00\x7F\x80\n"                           > test_files/data_mix.input

# Indeholder to bytes 0xC0 0xAF, som er en overlong UTF-8 encoding af '/'. Overlong encodings er forbudt i UTF-8, så filen skal ende i data.
printf "\xC0\xAF\n"                                   > test_files/data_overlong.input

# Indeholder tre bytes 0xE0 0x80 0xAF, som er en overlong UTF-8 encoding af '/'. Overlong encodings er forbudt i UTF-8, så filen skal ende i data.
printf "\xE0\x80\xAF\n"                               > test_files/data_overlong3.input

# Indeholder fem bytes 0xFC 0x80 0x80 0x80 0xAF, som er en overlong UTF-8 encoding af '/'. Overlong encodings er forbudt i UTF-8, så filen skal ende i data.
printf "\xFC\x80\x80\x80\x80\xAF\n"                   > test_files/data_overlong7.input

# Indeholder to bytes 0xC1 0xBF, som er en ugyldig UTF-8 sekvens. Skal klassificeres som data.
printf "\xC1\xBF\n"                                   > test_files/data_invalid3.input

# Indeholder fire bytes 0xFC 0x83 0xBF 0xBF, som er en ugyldig UTF-8 sekvens. Skal klassificeres som data.
printf "\xFC\x83\xBF\xBF\n"                           > test_files/data_invalid7.input

# Random bytes fra 0x81..0x9F (disallowed i ISO-8859-1 og ugyldige i UTF-8)
printf "\x81\x8D\x8F\x90\x9D\n"                       > test_files/data_control_bytes.input

# Kombination af gyldigt ASCII og forbudte bytes
printf "ABC\x80DEF\n"                                 > test_files/data_ascii_mixed.input

# Overtrukket sekvens: ugyldig UTF-8 startbyte (0xF5 er > 0xF4 tilladt i UTF-8)
printf "\xF5\x80\x80\x80\n"                           > test_files/data_utf8_invalid_start.input

# Lone continuation byte (0x80 uden en leder)
printf "\x80\n"                                       > test_files/data_lone_cont.input

# Ugyldig 5-byte sekvens (UTF-8 findes kun op til 4 byte)
printf "\xF8\x88\x80\x80\x80\n"                       > test_files/data_5byte.input

# Ugyldig 6-byte sekvens
printf "\xFC\x84\x80\x80\x80\x80\n"                   > test_files/data_6byte.input

# Null bytes blandet med ASCII
printf "X\x00Y\x00Z\n"                                > test_files/data_nulls_mixed.input

# DEL-byte midt i en tekst
printf "HELLO\x7FWORLD\n"                             > test_files/data_with_del.input

# Tilfældigt binært mønster
printf "\x01\xFF\xAA\xBB\xCC\n"                       > test_files/data_random_binary.input

# Gyldigt ISO-tegn efterfulgt af en ugyldig byte
printf "ø\x9F\n"                                      > test_files/data_iso_invalidmix.input

# Lange sekvenser af ugyldige bytes
printf "\x82\x83\x84\x85\x86\x87\n"                   > test_files/data_invalid_run.input

# “High” byte uden parring
printf "\xFE\n"                                       > test_files/data_fe.input

### Empty (skal være 0 bytes)
:                                                     > test_files/empty.input
:                                                     > test_files/empty2.input

### Permission denied-case
# Denne test skal fejle, fordi filen ikke kan læses. Vi laver filen og fjerner læserettighederne.
printf "hemmelighed"                                   > test_files/noread.input
chmod 000 test_files/noread.input

### Det er denne del jeg har tilføjet, alt andet er som det var. Dog nu med kommentar
### Denne note skal slettes inden aflevering

echo "Running the tests.."
exitcode=0  # vi samler en samlet "bestået/ikke-bestået"-status til sidst

















### Det er delen over dette jeg har tilføjet, alt andet er som det var. Dog nu med kommentar
### Denne note skal slettes inden aflevering

# Denne løkke kører én gang per fil, der matcher mønsteret test_files/*.input.
# Skallen (bash) udvider mønsteret til en liste af filnavne, før løkken starter.

for f in test_files/*.input
do
  echo ">>> Testing ${f}.."

      if [[ "$f" == *"noread.input" ]]; then
        echo "NOTE: chmod 000 virker kun på Linux-filsystemer (ext4, XFS osv.)."
        echo "      På WSL/NTFS kan filen stadig læses, så testen vil ikke fejle."
      fi

  # Vi opretter vores EXPECTED fil her.
  # Det er her vi laver vores EXPECTED vs ACTUAL sammenligning.
  # Dette er hvad jeg tror der sker:
  # - Vi laver to output-filer per input-fil: "<filnavn>.expected" og "<filnavn>.actual".
  # - Vi sammenligner de to output-filer med `diff`.
  # - Hvis de er ens, er testen bestået.
  # - Hvis de er forskellige, er testen ikke bestået.

  # Vi laver "forventet" output sådan her:
  # 1) Kør systemets 'file' på filen. 'file' gætter filtype ud fra indholdet.
  # 2) Send (pipe) output videre til 'sed', som klipper pynt fra, så teksten matcher vores simple kategorier.
  #    - '|' (pipe) betyder: "send venstres output ind i højres input".
  #    - 'sed' er en "stream editor" (tror: find/erstatt på tekstlinjer).
  #    - '-e' giver en regel. Vi kan give flere '-e' – de køres i rækkefølge.
  #    - 's/A/B/' betyder: "erstat det der matcher A, med B".
  #    - '.*' i A betyder "alt efter dette punkt" (0 eller flere vilkårlige tegn).
  # 3) Gem den normaliserede linje som "<filnavn>.expected".

  file    "$f" | sed     -e 's/ASCII text.*/ASCII text/' \
                         -e 's/UTF-8 Unicode text.*/UTF-8 Unicode text/' \
                         -e 's/ISO-8859 text.*/ISO-8859 text/' \
                         -e 's/writable, regular file, no read permission/cannot determine (Permission denied)/' \
                         > "${f}.expected"

  # Vi opretter vores ACTUAL fil her.
  # Her bygger vi vores ACTUAL-output ved at køre vores eget program på filen
  # Og gemmer det, programmet skriver (stdout), i "<filnavn>.actual".


  ./file  "${f}" > "${f}.actual"

  # Nu sammenligner vi de to output-filer:
  # 'diff' sammenligner to filer:
  #   - Exitkode 0 = filer er ens
  #   - Exitkode alt andet end 0 = der er forskel
  # '-u' viser forskelle i et pænt "unified" format (samme stil som git)
  #
  # '!' foran kommandoen betyder "negér exitkoden".
  # Så: 'if ! diff ...; then' læses som "hvis der VAR forskel, så gå i 'then'-grenen".
  
  if ! diff -u "${f}.expected" "${f}.actual"
  then
    echo ">>> Failed :-("
    exitcode=1
  else
    echo ">>> Success :-)"
  fi # fi er slutningen på en 'if' -blok (i stedet for end if)
done

# Scriptets slutresultat:
#  - 0 hvis alle tests lykkedes
#  - 1 hvis mindst én test fejlede

exit $exitcode
