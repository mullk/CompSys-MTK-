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

printf "Hello, World!\n" > test_files/ascii.input
printf "Hello, World!" > test_files/ascii2.input
printf "Hello,\x00World!\n" > test_files/data.input
printf "" > test_files/empty.input

### TODO: Generate more test files ###
### dont mind if i do :)))

### Det er denne del jeg har tilføjet, alt andet er som det var. Dog nu med kommentar
### Denne note skal slettes inden aflevering

### ASCII (tilladte bytes: 0x07..\x0D, 0x20..0x7E)
printf "Normal ASCII\n"       > test_files/ascii_normal.input
printf "With space \n"        > test_files/ascii_space.input
printf "With tab\t\n"         > test_files/ascii_tab2.input
printf "With newline\n\n"     > test_files/ascii_nl2.input
printf "With carriage\rreturn\n" > test_files/ascii_cr.input
printf "Mix:\x07\x0A\x0D \x20\x7E\n" > test_files/ascii_mix.input

### ASCII (kontroltegn: \x07..\x0D og ESC \x1B)
printf "Line1\r\nLine2\r\n"   > test_files/ascii_crlf.input
printf "TAB\tSEP\tOK\n"       > test_files/ascii_tab.input
printf "BEL:\x07 OK\n"        > test_files/ascii_bel.input
printf "\x1B[31mRED\x1B[0m\n" > test_files/ascii_esc.input
printf "Spaces only      \n"  > test_files/ascii_spaces.input
printf "Unix\nNewlines\n"     > test_files/ascii_unixnl.input
printf "Carriage\rOnly\r"     > test_files/ascii_cr_only.input

### ISO-8859-1 (latin1) – æøå og andre ud over ASCII (0xA0–0xFF)
printf "\xE6\xF8\xE5\n"       > test_files/iso_aeoeaa.input    # æøå
printf "Héllo\n"              > test_files/iso_accent.input    # é
printf "Grüße\n"              > test_files/iso_umlaut.input    # ü
printf "Færøerne\n"           > test_files/iso_mix.input       # æø

### UTF-8 Unicode (multibyte sekvenser)
printf "Hej 😀\n"              > test_files/utf8_emoji.input
printf "漢字\n"               > test_files/utf8_kanji.input
printf "Привет\n"             > test_files/utf8_cyrillic.input
printf "مرحبا\n"              > test_files/utf8_arabic.input

### Flere DATA typer (forbudte bytes: 0x00, 0x7F, 0x80..0x9F, mm.)
printf "NUL@\x00mid\n"        > test_files/data_null_mid.input
printf "\x00starts\n"         > test_files/data_null_start.input
printf "DEL:\x7F end\n"       > test_files/data_del_7f.input
printf "mix:\x00\x7F\x80\n"   > test_files/data_mix.input

### Empty (skal være 0 bytes)
: > test_files/empty.input
: > test_files/empty2.input

### Permission denied-case
printf "hemmelighed" > test_files/noread.input
chmod 000 test_files/noread.input

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

  file    "$f" | sed -e 's/ASCII text.*/ASCII text/' \
                         -e 's/UTF-8 Unicode text.*/UTF-8 Unicode text/' \
                         -e 's/Unicode text, UTF-8 text.*/UTF-8 Unicode text/' \
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
