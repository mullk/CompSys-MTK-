#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include "./endian.h"
#else
#include <endian.h>
#endif

#include "./peer.h"


// Denne peger på vores egen peer altså os selv i netværket.
// NetworkAddress_t er en struktur (defineret i peer.h)
// Den bruges til at gemme vores IP, vores portnummer, og vores salt (for signaturen).
NetworkAddress_t *my_address;




// Liste over peers i netværket. Det er en pointer til et array af 
// pointers, hvor hver peger på en anden NetworkAddress_t
// Hver entry i network repræsenterer en anden peer på netværket (IP + port + salt).
// I starten sættes den til NULL, fordi vi endnu ikke kender nogen andre peers.
// Når noget registrerer sig, får vi en liste over peers fra den 
// som vi forbinder os til, og så allokerer vi plads
NetworkAddress_t** network = NULL;




// Antal peers i netværket. Starter ved 0, fordi vi endnu ikke kender 
// nogen andre peers. Når vi fjerner en peer (f.eks. timeout), bliver den mindre.
uint32_t peer_count = 0;

/*
 * De snakker om hele    void* client_thread()      funktionen:
 * Function to act as thread for all required client interactions. This thread 
 * will be run concurrently with the server_thread. It will start by requesting
 * the IP and port for another peer to connect to. Once both have been provided
 * the thread will register with that peer and expect a response outlining the
 * complete network. The user will then be prompted to provide a file path to
 * retrieve. This file request will be sent to a random peer on the network.
 * This request/retrieve interaction is then repeated forever.
 */ 

// void* bruges fordi pthread_create() kræver den signatur:
void* client_thread()
{





// peer_ip er en lokal buffer til at gemme den IP-adresse brugeren indtaster.
// fprintf(stdout, ...) printer beskeden på skærmen.
// scanf("%16s", peer_ip); læser en streng fra tastaturet (maks. 16 tegn).
// %16s beskytter mod buffer overflow, så der aldrig skrives mere end 16 bytes ind.

char peer_ip[IP_LEN];
    fprintf(stdout, "Enter peer IP to connect to: ");
    scanf("%16s", peer_ip);






// Dette fylder resten af arrayet med null-tegn ('\0').
// Det sikrer, at der ikke ligger “gamle” eller tilfældige tegn i bufferen.
// Det gør output mere stabilt, når du senere kopierer den videre med memcpy().

    for (int i=strlen(peer_ip); i<IP_LEN; i++)
    {
        peer_ip[i] = '\0';
    }




// Samme idé som før — bare for portnummeret.
// Brugeren skriver fx:
// Enter peer port to connect to: 5000

    char peer_port[PORT_STR_LEN];
    fprintf(stdout, "Enter peer port to connect to: ");
    scanf("%16s", peer_port);


    

// Samme oprydning som for IP’en.
    // Clean up password string as otherwise some extra chars can sneak in.
    for (int i=strlen(peer_port); i<PORT_STR_LEN; i++)
    {
        peer_port[i] = '\0';
    }






// Her oprettes en NetworkAddress_t-struktur lokalt, hvor du:
// kopierer IP-adressen fra peer_ip over i peer_address.ip
// konverterer port-strengen til et tal (atoi = ASCII to integer)
// ideen er at vi skriver noget som 127.0.0.1 5000 i terminalen også
// og så gemmer vi det i peer_address strukturen som:

// peer_address.ip   = "127.0.0.1"
// peer_address.port = 5000

    NetworkAddress_t peer_address;
    memcpy(peer_address.ip, peer_ip, IP_LEN);
    peer_address.port = atoi(peer_port);



// Her slutter tråden — lige nu laver den intet efter inputtet.
// Men senere i opgaven skal vi have implementeret

// Mulighed for at oprette en socket
// at forbinde (connect()) til peer_address
// at der bliver mulighed for at sende en registreringsbesked
// at modtage svaret (netværkslisten)


    // You should never see this printed in your finished implementation
    printf("Client thread done\n");

    return NULL;
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

void* server_thread()
{
    // You should never see this printed in your finished implementation
    printf("Server thread done\n");

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