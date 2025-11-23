#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#ifdef __APPLE__
#include "./endian.h"
#else
#include <endian.h>
#endif

#include "peer.h"
#include "common.h"

#include <math.h>

// Globale værdier

NetworkAddress_t *my_address = NULL;
NetworkAddress_t **network = NULL;
uint32_t peer_count = 0;

pthread_mutex_t network_mutex = PTHREAD_MUTEX_INITIALIZER;
bool run = true;

void respond(int fd, uint32_t status, char* data, size_t bytes);


void compute_signature(const char *password, const char *salt, hashdata_t out)
{
    // Buffer der indeholder:  [password | salt]
    // Bemærk: PASSWORD_LEN og SALT_LEN er faste størrelser.
    char buf[PASSWORD_LEN + SALT_LEN];

    // Kopiér hele adgangskoden ind først.
    // Her går vi ud fra, at adgangskoden altid fylder PASSWORD_LEN bytes
    // (A3 specifikationen kræver faste størrelser).
    memcpy(buf, password, PASSWORD_LEN);

    // Læg saltet lige efter password i bufferen.
    memcpy(buf + PASSWORD_LEN, salt, SALT_LEN);

    // Hash hele (password || salt) i ét SHA-256 kald.
    // Resultatet skrives ud i 'out' (32 bytes).
    get_data_sha(buf, out, PASSWORD_LEN + SALT_LEN, SHA256_HASH_SIZE);
}








// Udskriver en liste over alle kendte peers i netværket.
// Funktionen låser netværks-listen for at undgå race conditions,
// så ingen anden tråd ændrer peer_count eller network[] mens der printes.
void print_network()
{
    // Sikrer eksklusiv adgang til network[] og peer_count
    pthread_mutex_lock(&network_mutex);

    // Print header med antal kendte peers
    printf("\n--- NETWORK (%u peers) ---\n", peer_count);

    // Gennemløber alle kendte peers og viser IP og port
    for (uint32_t i = 0; i < peer_count; i++) {
        printf("%u) %s:%u\n", i,
               network[i]->ip,
               network[i]->port);
    }

    printf("--------------------------\n");

    // Frigiv låsen så andre tråde kan opdatere netværkslisten igen
    pthread_mutex_unlock(&network_mutex);
}





// Finder en peer i min lokale netværksliste.
// Jeg leder efter en IP + port som matcher en af dem jeg allerede har gemt.
// Hvis jeg finder den -> returnerer dens index i network[].
// Hvis ikke -> return -1 (det bruger jeg som "peer findes ikke").
int find_peer(const char *ip, uint32_t port)
{
    // Går igennem alle peers jeg kender
    for (uint32_t i = 0; i < peer_count; i++) {

        // Tjekker om både IP og port matcher nøjagtigt
        // (IP er en streng, derfor strcmp; port er bare et tal)
        if (strcmp(network[i]->ip, ip) == 0 &&
            network[i]->port == port)
        {
            return i;   // Fandt peer -> giver index tilbage
        }
    }

    // Kommer jeg hertil, så fandt jeg ingen match
    return -1;
}






// Tjekker om afsenderens signature faktisk matcher det,
// jeg har gemt for den peer i min netværksliste.
// Hele pointen er at sikre, at en peer ikke bare kan udgive sig
// for at være en anden ved at sende samme IP/port.
// Hvis signaturen ikke passer -> så er der noget suspekt.
bool validate_signature(RequestHeader_t *hdr)
{
    // Først finder jeg peerens index i min lokale network[] liste.
    // Hvis den ikke er der -> så giver det ingen mening at tjekke signatur.
    int idx = find_peer(hdr->ip, hdr->port);
    if (idx < 0) return false;  // kender ikke peer -> validation fail

    // Nu sammenligner jeg den signatur, peer sender med den jeg har gemt.
    // Da begge er SHA256-hashes, kan jeg bare lave en direkte byte-compare.
    // Hvis alle 32 bytes er ens -> valid signature.
    return memcmp(hdr->signature,
                  network[idx]->signature,
                  SHA256_HASH_SIZE) == 0;
}












// Tilføjer en peer til mit lokale netværk.
// Funktionens job er basically:
//  1) Tjek om peer allerede findes (samme IP + port)
//       -> i så fald bare opdater signature + salt
//  2) Hvis peer er ny -> allokér plads og tilføj den til network[]
// Returnerer:
//      0 = peer eksisterede allerede
//      1 = peer var ny og blev tilføjet
int add_peer_to_network(const char *ip, uint32_t port,
                               const hashdata_t signature,
                               const char *salt)
{
    // Låser netværks-listen så der ikke er race-cond
    // (servertråde kan ellers opdatere listen samtidig)
    pthread_mutex_lock(&network_mutex);

    // Først tjekker jeg om vi allerede kender peer'en.
    // Det gør jeg ved at lede efter en entry med samme IP + port.
    for (uint32_t i = 0; i < peer_count; i++) {

        if (strcmp(network[i]->ip, ip) == 0 &&
            network[i]->port == port) {

            // Hvis vi allerede kender peer'en, så skal vi IKKE ignorere den.
            // A3-protokollen kræver at vi opdaterer salt + signature ved re-announce.
            memcpy(network[i]->signature, signature, SHA256_HASH_SIZE);
            memcpy(network[i]->salt, salt, SALT_LEN);

            // Frigiver låsen og melder “ikke ny”
            pthread_mutex_unlock(&network_mutex);
            return 0;
        }
    }

    // Hvis vi når hertil, så er det en NY peer
    // -> allokér en ekstra plads i network-listen
    network = realloc(network, (peer_count + 1) * sizeof(NetworkAddress_t *));
    network[peer_count] = malloc(sizeof(NetworkAddress_t));

    // Gemmer alle peerens oplysninger
    memset(network[peer_count]->ip, 0, IP_LEN);
    memcpy(network[peer_count]->ip, ip, IP_LEN);

    network[peer_count]->port = port;

    memcpy(network[peer_count]->signature, signature, SHA256_HASH_SIZE);
    memcpy(network[peer_count]->salt, salt, SALT_LEN);

    // Nu har vi én peer mere
    peer_count++;

    // Done -> lås op
    pthread_mutex_unlock(&network_mutex);

    return 1;   // peer var ny
}



// Læser hele request-headeren fra socket'en.
// Headeren har en fast størrelse (RequestHeader_t), så det er ret simpelt:
//  1) læs præcis sizeof(RequestHeader_t) bytes
//  2) konverter de felter der er i network byte-order -> host byte-order
// Hvis der ikke kommer nok bytes (client disconnect osv.) -> return false.
bool recv_request_header(int fd, RequestHeader_t *hdr)
{
    // compsys_helper_readn læser PRÆCIS det antal bytes vi beder om,
    // med mindre forbindelsen dør eller der sker fejl.
    ssize_t n = compsys_helper_readn(fd, hdr, sizeof(RequestHeader_t));
    if (n <= 0) return false;   // fik ikke en komplet header

    // De her tre felter bliver sendt som big-endian (netværksformat).
    // Så jeg skal konvertere dem til min maskines format,
    // ellers får jeg weird tal når jeg læser port/command/length.
    hdr->port    = be32toh(hdr->port);
    hdr->command = be32toh(hdr->command);
    hdr->length  = be32toh(hdr->length);

    return true;   // header blev læst korrekt
}







// Modtager selve "body"-delen af requesten.
// Headeren har allerede fortalt mig hvor mange bytes body fylder,
// så her skal jeg bare læse præcis 'length' bytes fra socket'en.
// Hvis length = 0, så er der ingen body (REGISTER har fx ingen body).
char *recv_body(int fd, uint32_t length)
{
    // Hvis der slet ikke er noget body, så kan jeg bare returnere NULL.
    // (Caller skal selv håndtere "ingen body".)
    if (length == 0) return NULL;

    // Allokerer en buffer til body-dataen.
    char *buf = malloc(length);

    // compsys_helper_readn prøver at læse PRÆCIS 'length' bytes.
    // Hvis den får < 0 eller 0 -> connection tabt, incomplete read osv.
    if (compsys_helper_readn(fd, buf, length) <= 0) {
        free(buf);     // husk at free hvis det fejler
        return NULL;   // signalér at body ikke blev læst korrekt
    }

    // Alt gik godt -> returnér bufferen med data
    return buf;
}




// Håndterer INFORM-beskeder.
// INFORM betyder: “der er kommet en ny peer i netværket – opdater din liste”.
// Det er basically det mekanisme, der holder hele netværket synkroniseret.
// INFORM skal ALDRIG give et reply når alt er OK (A3 regel).
void handle_inform(int fd, RequestHeader_t *hdr, const char *body)
{
    printf("[DEBUG] INFORM received from %s:%u  (length=%u)\n",
           hdr->ip, hdr->port, hdr->length);

    // 1) Tjek om afsenderen overhovedet findes i min netværksliste.
    // Hvis jeg ikke kender dem -> så er det mistænkeligt (peer missing).
    int idx = find_peer(hdr->ip, hdr->port);
    if (idx < 0) {
        respond(fd, STATUS_PEER_MISSING, NULL, 0);
        return;
    }

    // 2) Tjek om signaturen fra afsenderen matcher.
    // Hvis ikke -> de forsøger at udgive sig for en anden -> reject.
    if (!validate_signature(hdr)) {
        respond(fd, STATUS_BAD_PASSWORD, NULL, 0);
        return;
    }

    // 3) Body skal være præcis én peer-entry på 68 bytes.
    // Hvis body mangler eller har forkert størrelse -> MALFORMED.
    if (body == NULL) {
        respond(fd, STATUS_MALFORMED, NULL, 0);
        return;
    }
    if (hdr->length != PEER_ADDR_LEN) {
        respond(fd, STATUS_MALFORMED, NULL, 0);
        return;
    }

    // Body-format:
    //   ip (16)
    //   port (4)
    //   signature (32)
    //   salt (16)
    const char *ip = body;

    uint32_t port;
    memcpy(&port, body + IP_LEN, 4);
    port = be32toh(port); // konverter port til host byte-order

    printf("[DEBUG] INFORM body says: new peer = %s:%u\n", ip, port);

    // henter signature og salt for den nye peer
    const hashdata_t *sig = (const hashdata_t *)(body + IP_LEN + 4);
    const char *salt = body + IP_LEN + 4 + SHA256_HASH_SIZE;

    // 4) Tilføj peer til din liste (duplicate-safe).
    // add_peer_to_network returnerer 0 hvis den allerede eksisterer.
    if (!add_peer_to_network(ip, port, *sig, salt)) {
        // Hvis det var duplicate -> det er fint.
        // (INFORM skal ikke give nogen OK-besked tilbage)
        return;
    }

    // Husk: INFORM har ALDRIG noget reply på success.
    // Det er bare “opdater din liste og hold kæft bagefter”.
}



// En lille hjælperfunktion til at åbne en client-forbindelse,
// men uden at være alt for striks omkring input-validering.
// Jeg bruger denne i send_register(), fordi scanf nogle gange giver
// underlige ting tilbage (newline, null-bytes osv).
// Ideen her er at "rense" port-strengen og sikre at den faktisk er et tal.
int open_clientfd_nocheck(const char *ip, const char *port_str) {
    char *end = NULL;

    // strtol forsøger at lave port_str om til et tal.
    // Hvis hele strengen ikke kan tolkes som et tal,
    // så kommer 'end' til at pege tilbage på start -> fejl.
    long p = strtol(port_str, &end, 10);

    // Tjekker om konverteringen fejlede, eller porten er uden for range.
    if (end == port_str || p < 1 || p > 65535) {
        printf("Invalid port '%s'\n", port_str);
        return -1;   // returnér fejl så caller ved det ikke gik
    }

    // Laver en korrekt formateret portstreng (uden mærkelige bytes).
    char portbuf[16];
    snprintf(portbuf, sizeof(portbuf), "%ld", p);

    // Kopierer IP'en til en buffer (igen for at være sikker på at formatet er ok).
    char ip_copy[IP_LEN];
    snprintf(ip_copy, sizeof(ip_copy), "%s", ip);

    // compsys_helper_open_clientfd åbner selve socket-forbindelsen.
    return compsys_helper_open_clientfd(ip_copy, portbuf);
}











// Sender en REGISTER-request til en peer jeg vil forbinde mig til.
// Det her er basically: “Hej, må jeg være med i jeres netværk?”
// REGISTER giver altid et svar tilbage med en komplet peerlist,
// medmindre password/signature er forkert.
void send_register()
{
    char peer_ip[IP_LEN];
    char peer_port_str[16];

    // Nulstiller portbufferen (scanf efterlader mærkelige ting nogle gange)
    memset(peer_port_str, 0, sizeof(peer_port_str));

    // Spørger brugeren hvem vi vil registrere os hos
    printf("Enter peer IP to connect to: ");
    scanf(" %15s", peer_ip);

    printf("Enter peer port to connect to: ");
    scanf(" %15s", peer_port_str);

    // Fjerner evt. newline fra scanf
    peer_port_str[strcspn(peer_port_str, "\r\n")] = 0;
    printf("[DEBUG] peer_port_str entered = '%s'\n", peer_port_str);

    // Debug -> viser raw bytes fra port_str (nok mest til fejlfinding)
    printf("DBG bytes:");
    for (int i = 0; i < 16; i++) printf(" %02X", (unsigned char)peer_port_str[i]);
    printf("\n");

    // Jeg bruger min egen port-parser pga scanf bug med skjulte bytes
    int fd = open_clientfd_nocheck(peer_ip, peer_port_str);
    if (fd < 0) {
        printf("Cannot connect.\n");
        return;
    }


    // Bygger selve REGISTER-headeren
    uint32_t msg_len = REQUEST_HEADER_LEN;
    char msg[msg_len];

    // Afsenderens IP (mig selv)
    memset(msg, 0, IP_LEN);
    memcpy(msg, my_address->ip, IP_LEN);

    // Afsenderens port
    uint32_t netp = htobe32(my_address->port);
    memcpy(msg + IP_LEN, &netp, 4);

    // Afsenderens signature (beviser at vi kender password)
    memcpy(msg + IP_LEN + 4, my_address->signature, SHA256_HASH_SIZE);

    // Kommando = REGISTER
    uint32_t cmd = htobe32(COMMAND_REGISTER);
    memcpy(msg + IP_LEN + 4 + SHA256_HASH_SIZE, &cmd, 4);

    // REGISTER har altid length = 0
    uint32_t zero = 0;
    memcpy(msg + IP_LEN + 4 + SHA256_HASH_SIZE + 4, &zero, 4);

    // Sender REGISTER-pakken
    compsys_helper_writen(fd, msg, msg_len);


    // Modtag svar (peerlist + status)
    ReplyHeader_t reply;
    compsys_helper_readn(fd, &reply, sizeof(reply));

    uint32_t status = be32toh(reply.status);

    // Tjek hvad serveren siger om vores registration
    if (status == STATUS_PEER_EXISTS) {
        printf("You were already registered on that peer.\n");

    } else if (status == STATUS_BAD_PASSWORD) {
        printf("Password mismatch! Registration rejected.\n");
        close(fd);
        return;

    } else if (status != STATUS_OK) {
        printf("Register failed -> status %u\n", status);
        close(fd);
        return;
    }

    // Hvis status er OK -> modtager en peerlist
    reply.length = be32toh(reply.length);

    char *payload = NULL;
    if (reply.length > 0) {
        payload = malloc(reply.length);
        compsys_helper_readn(fd, payload, reply.length);
    }

    // Parser peerlisten: den består af N entries på 68 bytes
    uint32_t entries = reply.length / PEER_ADDR_LEN;
    for (uint32_t i = 0; i < entries; i++) {
        uint32_t pos = i * PEER_ADDR_LEN;

        char ipbuf[IP_LEN];
        memcpy(ipbuf, payload + pos, IP_LEN);

        uint32_t port;
        memcpy(&port, payload + pos + IP_LEN, 4);
        port = be32toh(port);

        hashdata_t sig;
        memcpy(sig, payload + pos + IP_LEN + 4, SHA256_HASH_SIZE);

        char salt[SALT_LEN];
        memcpy(salt, payload + pos + IP_LEN + 4 + SHA256_HASH_SIZE, SALT_LEN);

        // Tilføjer peer til min lokale netværksliste
        add_peer_to_network(ipbuf, port, sig, salt);
    }

    if (payload) free(payload);
    close(fd);

    // Print hele netværket så jeg kan se resultatet af REGISTER
    print_network();
}







// Modtager en fil i blokke.
// Dette er “modsætningen” til handle_retrieve(), så de to SKAL være enige om formatet.
// Flowet er basically:
//   1) læs første reply-header (skal være block 0)
//   2) læg payload fra block 0 i buffer og tjek block_hash
//   3) læs alle efterfølgende blokke én efter én
//   4) verificér block_hash for hver blok
//   5) til sidst -> verificér total_hash (hele filen)
//   6) skriv filen til disk
static int receive_file_blocks(int fd, const char *outname)
{
    ReplyHeader_t reply;

    // Læs første header — den MÅ være block 0
    if (compsys_helper_readn(fd, &reply, sizeof(reply)) <= 0) {
        printf("Error: unable to read reply header.\n");
        return -1;
    }

    uint32_t status      = be32toh(reply.status);
    uint32_t block_no    = be32toh(reply.this_block);
    uint32_t block_count = be32toh(reply.block_count);
    uint32_t payload_len = be32toh(reply.length);

    // Total-hash fra den første blok -> bruges senere til slutverifikation
    hashdata_t expected_total_hash;
    memcpy(expected_total_hash, reply.total_hash, SHA256_HASH_SIZE);

    if (status != STATUS_OK) {
        printf("Retrieve failed -> status = %u\n", status);
        return -1;
    }

    // Første blok SKAL være block 0 ifølge protokollen
    if (block_no != 0) {
        printf("Protocol error: first block was not block 0.\n");
        return -1;
    }

    // Alloker plads til alle blokke (maks størrelse)
    uint32_t block_data_max = MAX_MSG_LEN - REPLY_HEADER_LEN;
    uint32_t max_total = block_count * block_data_max;

    char *filebuf = malloc(max_total);
    if (!filebuf) {
        printf("Memory allocation failed.\n");
        return -1;
    }

    // --- Læs payload for block 0 ---
    if (payload_len > 0) {
        if (compsys_helper_readn(fd, filebuf, payload_len) <= 0) {
            printf("Error reading block payload.\n");
            free(filebuf);
            return -1;
        }
    }

    uint32_t offset = payload_len;

    // Verificér block_hash for block 0
    {
        hashdata_t local_hash;
        get_data_sha(filebuf, local_hash, payload_len, SHA256_HASH_SIZE);

        if (memcmp(local_hash, reply.block_hash, SHA256_HASH_SIZE) != 0) {
            printf("Error: block 0 hash mismatch! File corrupted.\n");
            free(filebuf);
            return -1;
        }
    }

    // --- Læs resten af blokkene en efter en ---
    for (uint32_t b = 1; b < block_count; b++) {

        // Læs header for blok b
        if (compsys_helper_readn(fd, &reply, sizeof(reply)) <= 0) {
            printf("Error reading header for block %u\n", b);
            free(filebuf);
            return -1;
        }

        uint32_t b_status = be32toh(reply.status);
        uint32_t b_block  = be32toh(reply.this_block);
        uint32_t b_len    = be32toh(reply.length);

        if (b_status != STATUS_OK) {
            printf("Error: block %u returned status %u\n", b, b_status);
            free(filebuf);
            return -1;
        }

        if (b_block != b) {
            printf("Protocol mismatch: expected block %u but got %u\n", b, b_block);
            free(filebuf);
            return -1;
        }

        // Læs payload for denne blok
        if (b_len > 0) {
            if (compsys_helper_readn(fd, filebuf + offset, b_len) <= 0) {
                printf("Error reading payload for block %u\n", b);
                free(filebuf);
                return -1;
            }
        }

        // --- block_hash check ---
        {
            hashdata_t local_hash;
            get_data_sha(filebuf + offset, local_hash, b_len, SHA256_HASH_SIZE);

            if (memcmp(local_hash, reply.block_hash, SHA256_HASH_SIZE) != 0) {
                printf("Error: block %u hash mismatch! File corrupted.\n", b);
                free(filebuf);
                return -1;
            }
        }

        offset += b_len;
    }

    // ---- Total hash check (hele filen) ----
    {
        hashdata_t computed_total;
        get_data_sha(filebuf, computed_total, offset, SHA256_HASH_SIZE);

        // Hvis de ikke matcher -> filen er korrupt
        if (memcmp(computed_total, expected_total_hash, SHA256_HASH_SIZE) != 0) {
            printf("Error: total file hash mismatch! Transfer corrupted.\n");
            free(filebuf);
            return -1;
        }
    }

    // --- Skriv hele filens indhold til output-fil ---
    FILE *out = fopen(outname, "wb");
    if (!out) {
        printf("Error: cannot write output file.\n");
        free(filebuf);
        return -1;
    }

    fwrite(filebuf, 1, offset, out);
    fclose(out);

    printf("File \"%s\" saved securely (%u bytes).\n", outname, offset);

    free(filebuf);
    return offset;
}










// Sender en RETRIEVE-request til en peer og henter en fil tilbage.
// Det her er basically klient-siden af filoverførslen.
// Jeg vælger en peer, bygger en RETRIEVE-header med filnavnet som body,
// og så bruger jeg receive_file_blocks() til at hente filen ned.
void send_retrieve()
{
    // Kan ikke hente noget hvis netværkslisten er tom.
    if (peer_count == 0) {
        printf("No peers to retrieve from.\n");
        return;
    }

    // Læs filnavnet fra brugeren.
    char filename[PATH_LEN];
    printf("Enter filename: ");
    scanf(" %255s", filename);

    // Vis listen over kendte peers, så brugeren kan vælge én.
    print_network();
    printf("Select peer index: ");
    int idx;
    scanf(" %d", &idx);

    // Simpel bounds-check på valg af peer.
    if (idx < 0 || (uint32_t)idx >= peer_count) {
        printf("Invalid index.\n");
        return;
    }

    // Åbn forbindelse til den valgte peer.
    char portbuf[16];
    sprintf(portbuf, "%u", network[idx]->port);

    int fd = compsys_helper_open_clientfd(network[idx]->ip, portbuf);
    if (fd < 0) {
        printf("Cannot connect.\n");
        return;
    }

    // Byg RETRIEVE-requesten

    // Body’en er bare filnavnet + null-byte
    uint32_t body_len = strlen(filename) + 1;
    uint32_t msg_len = REQUEST_HEADER_LEN + body_len;

    char *msg = malloc(msg_len);
    memset(msg, 0, msg_len);

    // afsenderens IP
    memcpy(msg, my_address->ip, IP_LEN);

    // afsenderens port i big-endian
    uint32_t netp = htobe32(my_address->port);
    memcpy(msg + IP_LEN, &netp, 4);

    // afsenderens signature (til password-check)
    memcpy(msg + IP_LEN + 4, my_address->signature, SHA256_HASH_SIZE);

    // kommando = RETRIEVE
    uint32_t cmd = htobe32(COMMAND_RETREIVE);
    memcpy(msg + IP_LEN + 4 + SHA256_HASH_SIZE, &cmd, 4);

    // længden på body
    uint32_t netlen = htobe32(body_len);
    memcpy(msg + IP_LEN + 4 + SHA256_HASH_SIZE + 4, &netlen, 4);

    // kopier filnavnet ind som body
    memcpy(msg + REQUEST_HEADER_LEN, filename, body_len);

    // send hele requesten
    compsys_helper_writen(fd, msg, msg_len);
    free(msg);


    // Modtag filen i blokke
    int bytes = receive_file_blocks(fd, filename);
    close(fd);

    // hvis alt gik godt -> sig det
    if (bytes >= 0)
        printf("File \"%s\" saved (%d bytes).\n", filename, bytes);
}





// Version af send_retrieve() hvor jeg ikke selv vælger peer,
// men i stedet vælger en tilfældig peer fra netværkslisten.
// God til at teste at alle peers håndterer RETRIEVE korrekt,
// og at netværket er synkroniseret (INFORM virker).
void send_random_retrieve()
{
    // Først: kan jeg overhovedet hente noget?
    if (peer_count == 0) {
        printf("No peers to retrieve from.\n");
        return;
    }

    // Spørger brugeren om hvilken fil de vil hente.
    char filename[PATH_LEN];
    printf("Enter filename: ");
    scanf(" %255s", filename);

    // Vælg en tilfældig peer fra listen.
    // rand() % peer_count giver et index i range [0, peer_count-1].
    uint32_t idx = rand() % peer_count;

    printf("Retrieving from random peer %u (%s:%u)\n",
           idx, network[idx]->ip, network[idx]->port);

    // Åbn forbindelse til den tilfældige peer.
    char portbuf[16];
    sprintf(portbuf, "%u", network[idx]->port);

    int fd = compsys_helper_open_clientfd(network[idx]->ip, portbuf);
    if (fd < 0) {
        printf("Cannot connect to chosen peer.\n");
        return;
    }

    // Byg RETRIEVE-requesten
    uint32_t body_len = strlen(filename) + 1;
    uint32_t msg_len = REQUEST_HEADER_LEN + body_len;

    char *msg = malloc(msg_len);
    memset(msg, 0, msg_len);

    // afsenderens IP (mig selv)
    memcpy(msg, my_address->ip, IP_LEN);

    // afsenderens port
    uint32_t netp = htobe32(my_address->port);
    memcpy(msg + IP_LEN, &netp, 4);

    // min signature (password-bevis)
    memcpy(msg + IP_LEN + 4, my_address->signature, SHA256_HASH_SIZE);

    // kommando = RETRIEVE
    uint32_t cmd = htobe32(COMMAND_RETREIVE);
    memcpy(msg + IP_LEN + 4 + SHA256_HASH_SIZE, &cmd, 4);

    // body-length (filnavn + nullbyte)
    uint32_t netlen = htobe32(body_len);
    memcpy(msg + IP_LEN + 4 + SHA256_HASH_SIZE + 4, &netlen, 4);

    // placerer selve filnavnet i body
    memcpy(msg + REQUEST_HEADER_LEN, filename, body_len);

    // send requesten
    compsys_helper_writen(fd, msg, msg_len);
    free(msg);

    // Modtag filen i blokke
    int bytes = receive_file_blocks(fd, filename);
    close(fd);

    if (bytes >= 0)
        printf("File \"%s\" saved (%d bytes).\n", filename, bytes);
}

// Dette er “klient”-delen af min peer, dvs. den tråd der snakker med brugeren.
// Her vælger jeg manuelt hvilke kommandoer jeg vil sende til netværket.
// Den kører i en løkke indtil brugeren vælger quit.
void *client_thread()
{
    while (true) {
        // Menuen brugeren kan vælge fra
        printf("\nCommands:\n");
        printf("1) register\n");
        printf("2) retrieve (random peer)\n");
        printf("3) retrieve (choose peer)\n");
        printf("4) quit\n");
        printf("5) list\n");
        printf("> ");

        int choice;

        // scanf er meget sensitiv -> hvis input fails, så rydder jeg stdin.
        if (scanf(" %d", &choice) != 1) {
            // Rydder buffer for at undgå infinite loops
            while (getchar() != '\n');
            continue;
        }

        // Brugeren vælger hvilken netværkskommando der skal køres.
        if (choice == 1)
            send_register();           // registrer mig hos en peer

        else if (choice == 2)
            send_random_retrieve();    // hent fil fra en tilfældig peer

        else if (choice == 3)
            send_retrieve();           // hent fil fra specifik peer

        else if (choice == 4) {
            run = false;               // stopper både server og klient
            break;
        }

        else if (choice == 5)
            print_network();           // vis hele peer-listen
    }

    return NULL;
}

bool is_equal(hashdata_t first, hashdata_t second){
    for(int i = 0; i < SHA256_HASH_SIZE; i++){
        if(first[i] != second[i]){
            return false;
        }
    }
    return true;
}

void add_response_header(char* message, size_t block_length, uint32_t status, uint32_t current_block, uint32_t blocks, hashdata_t* total_hash, hashdata_t* block_hash){
    uint32_t mess_length = htobe32(REPLY_HEADER_LEN + block_length);
    uint32_t mess_status = htobe32(status);
    uint32_t net_current_block = htobe32(current_block);
    uint32_t max_blocks = htobe32(blocks);

    memcpy(message, &mess_length, sizeof(uint32_t));
    memcpy(message + (sizeof(uint32_t) * 1), &mess_status, sizeof(uint32_t));
    memcpy(message + (sizeof(uint32_t) * 2), &net_current_block, sizeof(uint32_t));
    memcpy(message + (sizeof(uint32_t) * 3), &max_blocks, sizeof(uint32_t));
    if(block_hash == 0){
        memset(message + (sizeof(uint32_t) * 4), 0, SHA256_HASH_SIZE);
    }else{
        memcpy(message + (sizeof(uint32_t) * 4), block_hash, SHA256_HASH_SIZE);
    }
    if(total_hash == 0){
        memset(message + (sizeof(uint32_t) * 4) + SHA256_HASH_SIZE, 0, SHA256_HASH_SIZE);
    }else{
        memcpy(message + (sizeof(uint32_t) * 4) + SHA256_HASH_SIZE, total_hash, SHA256_HASH_SIZE);
    }
        
}

void respond(int fd, uint32_t status, char* data, size_t bytes){
    uint32_t blocks = ceil((float) bytes / (float) MAX_MSG_LEN);
    hashdata_t total_hash;
    get_data_sha(data, total_hash, bytes, SHA256_HASH_SIZE);

    if(bytes == 0){
        char message[REPLY_HEADER_LEN];
        add_response_header(message, 0, status, 0, 1, 0, 0);
        ssize_t writen = compsys_helper_writen(fd, message, REPLY_HEADER_LEN);
        if(writen < 0){
            printf("ERROR: trying to write %s\n", strerror(errno));
        }
        return;
    }
    size_t bytes_send = 0;
    for(uint32_t i = 0; i < blocks; i++){
        size_t this_length = MAX_MSG_LEN - REPLY_HEADER_LEN;
        if(i == blocks - 1){
            this_length = bytes - bytes_send;
        }

        char message[REPLY_HEADER_LEN + this_length];

        hashdata_t block_hash;
        get_data_sha(&data[bytes_send], block_hash, this_length, SHA256_HASH_SIZE);

        add_response_header(message, this_length, status, i, blocks, &total_hash, &block_hash);

        memcpy(message + REPLY_HEADER_LEN, data + bytes_send, this_length);

        ssize_t writen = compsys_helper_writen(fd, message, this_length + REPLY_HEADER_LEN);
        if(writen < 0){
            printf("ERROR: trying to write %s\n", strerror(errno));
            return;
        }
        assert(writen == (ssize_t)(this_length + REPLY_HEADER_LEN));
        bytes_send += this_length;
    }
}

void send_to_client(char* message, size_t message_len, char ip[IP_LEN], uint32_t port){
    char c_port[5];
    sprintf(c_port, "%u", port);
    c_port[5] = '\0';

    int fd = compsys_helper_open_clientfd(ip, c_port);
    if(fd >= 0){
        ssize_t write = compsys_helper_writen(fd, message, message_len);
        if(write < 0){
            printf("ERROR: trying to write to %s:%u\n", ip, port);
        }
    }else{
        if(fd == -1){
            printf("ERROR: trying to open %s:%u %s\n", ip, port, strerror(errno));
        }else if(fd == -2){

        }else{
            printf("Unknown error trying to open %s:%u\n", ip, port);
        }
    }
}

void add_request_header(char* message, uint32_t payload_length){
        memcpy(message, my_address->ip, IP_LEN);
        memcpy(message + IP_LEN, &my_address->port, PORT_LEN);
        memcpy(message + (IP_LEN + PORT_LEN), my_address->signature, SHA256_HASH_SIZE);

        uint32_t cmd = htobe32(COMMAND_INFORM);
        memcpy(message + (IP_LEN + PORT_LEN + SHA256_HASH_SIZE), &cmd, sizeof(uint32_t));
        uint32_t addr_len = htobe32(payload_length);
        memcpy(message + (IP_LEN + PORT_LEN + SHA256_HASH_SIZE + sizeof(uint32_t)), &addr_len, sizeof(uint32_t));
}

void send_inform(){
    for(uint32_t i = 0; i < peer_count - 1; i++){
        size_t message_len = REQUEST_HEADER_LEN + PEER_ADDR_LEN;
        char message[message_len];
        add_request_header(&message[0], PEER_ADDR_LEN);

        memcpy(message + REQUEST_HEADER_LEN, network[peer_count - 1]->ip, IP_LEN);
        uint32_t peer_port = htobe32(network[peer_count - 1]->port);
        memcpy(message + (REQUEST_HEADER_LEN + IP_LEN), &peer_port, PORT_LEN);
        memcpy(message + (REQUEST_HEADER_LEN + IP_LEN + PORT_LEN), my_address->signature, SHA256_HASH_SIZE);
        memcpy(message + (REQUEST_HEADER_LEN + IP_LEN + PORT_LEN + SHA256_HASH_SIZE), my_address->salt, SALT_LEN);
        
        send_to_client(message, message_len, network[i]->ip, network[i]->port);
    }
}

bool is_unregistrered(RequestHeader_t* data){
    for(uint32_t i = 0; i < peer_count; i++){
        if(network[i]->ip == data->ip) return false;
    }
    return true;
}

void register_peer(int fd, RequestHeader_t* header){
    if(is_unregistrered(header)){
        char data[PEER_ADDR_LEN * (peer_count + 1)];
        
        memcpy(data, my_address->ip, IP_LEN);
        uint32_t peer_port = htobe32(my_address->port);
        memcpy(data + (IP_LEN), &peer_port, PORT_LEN);
        memcpy(data + (IP_LEN + PORT_LEN), my_address->signature, SHA256_HASH_SIZE);
        memcpy(data + (IP_LEN + PORT_LEN + SHA256_HASH_SIZE), my_address->salt, SALT_LEN);

        for(uint32_t i = 0; i < peer_count; i++){
            memcpy(data + (PEER_ADDR_LEN * (i + 1)), network[i]->ip, IP_LEN);
            uint32_t peer_port = htobe32(network[peer_count - 1]->port);
            memcpy(data + (PEER_ADDR_LEN * (i + 1) + IP_LEN), &peer_port, PORT_LEN);
            memcpy(data + (PEER_ADDR_LEN * (i + 1) + IP_LEN + PORT_LEN), my_address->signature, SHA256_HASH_SIZE);
            memcpy(data + (PEER_ADDR_LEN * (i + 1) + IP_LEN + PORT_LEN + SHA256_HASH_SIZE), my_address->salt, SALT_LEN);
        }
        

        respond(fd, STATUS_OK, data, PEER_ADDR_LEN * (peer_count + 1));
    }else{
        respond(fd, STATUS_PEER_EXISTS, NULL, 0);
        return;
    }
    if(network != NULL){
        network = realloc(network, sizeof(NetworkAddress_t*) * peer_count);
    }else{
        network = malloc(sizeof(NetworkAddress_t*));
    }
    network[peer_count] = malloc(sizeof(NetworkAddress_t));

    memcpy(network[peer_count]->ip, header->ip, IP_LEN);
    network[peer_count]->port = header->port;
    memcpy(network[peer_count]->signature, header->signature, SHA256_HASH_SIZE);
    //network[peer_count]->salt = header.

    peer_count++;

    send_inform();
}

void retrive(int fd, RequestHeader_t* data){
    char file_path[be32toh(data->length) + 1];
    ssize_t bytes = compsys_helper_readn(fd, file_path, be32toh(data->length));
    if(bytes == -1){
        printf("ERROR: %s\n", strerror(errno));
    }

    file_path[be32toh(data->length)] = 0;

    FILE* file = fopen(file_path, "r");
    if(file == NULL){
        respond(fd, STATUS_BAD_REQUEST, NULL, 0);
        return;
    }

    //The following file read code is copied from A0 as handed in by DHV501
    //Create initial read buffer
    int bufferSize = 128;
    char* buffer = malloc(bufferSize);

    int currentBufferIndex = 0;
    while(true){
        char c = fgetc(file);
        if(c != EOF){
            //if  the current read buffer is full reallocate at twice the current size.
            if(currentBufferIndex == bufferSize){
                char* newBuffer = malloc(bufferSize * 2);
                memcpy(newBuffer, buffer, bufferSize);
                free(buffer);
                buffer = newBuffer;
                bufferSize *= 2;
            }
            //place current byte into the read buffer and increment the index.
            *(buffer + currentBufferIndex) = c;
            currentBufferIndex++;
        }else{
            break;
        }
    }

    respond(fd, STATUS_OK, buffer, currentBufferIndex);
    free(buffer);
}

/*
 * Function to act as basis for running the server thread. This thread will be
 * run concurrently with the client thread, but is infinite in nature.
 */

// Det her er servertråden, som skal køre sideløbende med client_thread().
// Vi starter den i main()? med:
// pthread_create(&server_thread_id, NULL, server_thread, NULL);
// Den skal køre for evigt (”infinite in nature”), fordi en server altid skal lytte efter forbindelser fra andre peers.

// Hvad vi skal implementere her
// Oprette en TCP-socket
// Binde den til din egen IP og port
// Lytte på forbindelser
// Køre en uendelig løkke som accepterer klienter:
// Modtage beskeder (REGISTER, RETRIEVE, INFORM)
// Sende svar tilbage (som i Python-versionens RequestHandler.handle())

void* server_thread(){
       char port[5];
       sprintf(port, "%u", my_address->port);
       port[5] = '\0';
    
       int fd = compsys_helper_open_listenfd(port);
       if(fd == -1) printf("ERROR: %s\n", strerror(errno));
    while(true){
        int client_fd = accept(fd, NULL, NULL);
        //TODO: handle error.
        while(true){
            compsys_helper_state_t state;
            compsys_helper_readinitb(&state, client_fd);
            RequestHeader_t header;
            ssize_t bytes = compsys_helper_readn(client_fd, &header, sizeof(header));

            header.port = be32toh(header.port);
            header.command = be32toh(header.command);

            if(!is_valid_ip(header.ip) || !is_valid_port(header.port)){
                respond(client_fd, STATUS_MALFORMED, NULL, 0);
            }

            if(bytes == -1){
                if(errno == EBADF){
                    break;
                }else{
                    printf("ERROR: %s\n", strerror(errno));
                }
            }else if(bytes == 0){
                break; //Connection closed.
            }
            switch (header.command)
            {
            case COMMAND_REGISTER:
                register_peer(client_fd, &header);
                break;
            case COMMAND_INFORM:{
                char body[header.length];
                compsys_helper_readn(client_fd, body, header.length);
                handle_inform(client_fd, &header, body);
                break;
            }
            case COMMAND_RETREIVE:
                retrive(client_fd, &header);
                break;
            default:
                printf("ERROR: unkown command %u\n", header.command);
                respond(client_fd, STATUS_MALFORMED, NULL, 0);
                break;
            }
            close(client_fd);

        }
    }
    return NULL;
}











// main() er programmets entry point. Den kører først.

int main(int argc, char **argv)
{
// Users should call this script with a single argument describing what 
// config to use

// argc = antal argumenter, programmet fik ved start.
// argv = listen af argumenter som tekststrenge.
// burde være her den modtager vores argument som IP og PORT
// altså det vi skriver i terminalen når vi starter programmet
// fx: ./peer 127.0.0.1 5000


// Her er:
// argv[0] = "./peer"
// argv[1] = "127.0.0.1"
// argv[2] = "5000"
// hvis vi glemmer at skrive noget så tjekker vi det her:
// og udskriver en fejlbesked og afslutter programmet.


    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    } 







// Her oprettes vi vores egen adresse i hukommelsen:
// malloc() → reserverer plads til strukturen
// memset() → sletter alt i ip-feltet (fylder med null bytes)
// memcpy() → kopierer IP’en fra argumentet (f.eks. "127.0.0.1")
// atoi() → laver "5000" om til integer 5000 og gemmer den i port
// Efter det har vi:
// my_address->ip   = "127.0.0.1"
// my_address->port = 5000

    my_address = (NetworkAddress_t*)malloc(sizeof(NetworkAddress_t));
    memset(my_address->ip, '\0', IP_LEN);
    memcpy(my_address->ip, argv[1], strlen(argv[1]));
    my_address->port = atoi(argv[2]);





// De her funktioner (fra peer.h) tjekker:
// om IP’en er i korrekt format (xxx.xxx.xxx.xxx)
// om porten ligger inden for gyldigt område (typisk 1024–65535)
// Hvis noget er forkert → print fejl og stop.

    if (!is_valid_ip(my_address->ip)) {
        fprintf(stderr, ">> Invalid peer IP: %s\n", my_address->ip);
        exit(EXIT_FAILURE);
    }
    
    if (!is_valid_port(my_address->port)) {
        fprintf(stderr, ">> Invalid peer port: %d\n", 
            my_address->port);
        exit(EXIT_FAILURE);
    }






// Beder brugeren skrive et password til denne peer
// %16s beskytter mod overflow (maks. 16 tegn)
// Det bruges senere til at lave en digital signatur (hash)

    char password[PASSWORD_LEN];
    fprintf(stdout, "Create a password to proceed: ");
    scanf("%16s", password);



// Derefter “renser” vi bufferen:

    // Clean up password string as otherwise some extra chars can sneak in.
    for (int i=strlen(password); i<PASSWORD_LEN; i++)
    {
        password[i] = '\0';
    }




    
// Salt er en lille streng, der blandes med passwordet for at gøre hash mere sikker.
// Her bruges en hardcodet salt, men normalt skal den genereres tilfældigt:
// generate_random_salt(salt);
// Salt gemmes i my_address, så andre peers kan validere din signatur.


    // Most correctly, we should randomly generate our salts, but this can make
    // repeated testing difficult so feel free to use the hard coded salt below
    char salt[SALT_LEN+1] = "0123456789ABCDEF\0";
    //generate_random_salt(salt);
    memcpy(my_address->salt, salt, SALT_LEN);

    char salted[PASSWORD_LEN + SALT_LEN];
    memcpy(salted, password, PASSWORD_LEN);
    memcpy(&salted[PASSWORD_LEN], salt, SALT_LEN);
    
    get_data_sha(salted, my_address->signature, PASSWORD_LEN + SALT_LEN, SHA256_HASH_SIZE);




// pthread_create() starter en ny tråd.
// Den første kører funktionen client_thread()
// -> denne håndterer brugerinput og registrering hos andre peers.
// Den anden kører server_thread()
// -> denne håndterer indkommende forbindelser fra andre peers.
// Begge kører samtidigt (concurrently).

    // Setup the client and server threads 
    pthread_t client_thread_id;
    pthread_t server_thread_id;
    pthread_create(&client_thread_id, NULL, client_thread, NULL);
    pthread_create(&server_thread_id, NULL, server_thread, NULL);





// pthread_join() betyder “vent på at tråden bliver færdig”.
// Det er her main-tråden pauser, så programmet ikke bare afsluttes.
// Men i praksis bliver server_thread aldrig færdig (den skal køre for evigt).


    // Wait for them to complete. 
    pthread_join(client_thread_id, NULL);
    pthread_join(server_thread_id, NULL);

    exit(EXIT_SUCCESS);
}