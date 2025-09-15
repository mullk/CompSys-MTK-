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

### ISO-8859-1 (latin1) – æøå og andre ud over ASCII (0xA0–0xFF)
#printf "\xE6\xF8\xE5\n"       > test_files/iso_aeoeaa.input    # æøå
#printf "Grüße\n"              > test_files/iso_umlaut.input    # ü
printf "Héllo\n"              > test_files/iso_accent.input    # é
printf "\0xE9\n"               > test_files/emaerke.input #som Hexa é

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
                         -e 's/Unicode text, UTF-8 text.*/UTF-8 Unicode text/' \
                         -e 's/ISO-8859 text.*/ISO-8859 text/' \
                         -e 's/writable, regular file, no read permission/cannot determine (Permission denied)/' \
                         > "${f}.expected"

  # Vi opretter vores ACTUAL fil her.
  # Her bygger vi vores ACTUAL-output ved at køre vores eget program på filen
  # Og gemmer det, programmet skriver (stdout), i "<filnavn>.actual".
  #https://chatgpt.com/share/68c7d7bf-d924-800c-8b5a-8f50c8c8642f NOTE TIL -e 's/Unicode text, UTF-8 text.*/UTF-8 Unicode text/' \


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
