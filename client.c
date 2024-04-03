

#include "funzioni.h"

void stampa_guida_utente()
{
    printf("find --> ricerca la disponibilità per una prenotazione\n");
    printf("book --> invia una prenotazione\n");
    printf("esc  --> termina il client\n");
}
int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        perror("Too Many Arguments!\n");
        exit(1);
    }

    // Dichiarazione Costanti
    // 
    //const int port = atoi(argv[1]);

    // Dichiarazione Variabili
    int sd;
    char msg[BUFFER_SIZE];
    struct sockaddr_in server_addr;

    // Dichiarazione Set
    int fd_max;
    fd_set read_fs;
    fd_set master;

    stampa_guida_utente();

    sd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr)); // Pulisco la zona di memoria.
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "10.0.2.69", &server_addr.sin_addr);
    int ret = connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("Errore nella Connessione\n");
        exit(1);
    }

    invia_intero(sd, 1); // Dico al server che sono un client

    FD_ZERO(&master);  // Azzero il set master.
    FD_ZERO(&read_fs); // Azzero il set copia.

    FD_SET(sd, &master);             // Inserisco nel set il socket di ascolto.
    FD_SET(STANDARD_INPUT, &master); // Inserisco nel set lo standard input.
    fd_max = sd;                     // L'ultimo inserito è sd.

    while (1)
    {
        read_fs = master;

        select(fd_max + 1, &read_fs, NULL, NULL, NULL);

        for (int i = 0; i <= fd_max; i++)
        {
            
            if (FD_ISSET(i, &read_fs))
            { // Il Socket i è pronto in lettura.

                if (i == STANDARD_INPUT)
                {
                    if (fgets(msg, BUFFER_SIZE, stdin))
                    {
                        char **command = malloc(sizeof(char *) * 5);
                        char *messaggio = malloc(sizeof(char) * BUFFER_SIZE);
                        int numero_parole = 0;

                        stringa_vettorizzata_precisa(msg, " ", &numero_parole, command, 5);

                        if (numero_parole == 0)
                        {
                            continue; // comando non valido
                        }

                        if (strcmp(command[0], "find") == 0 || strcmp(command[0], "find\n") == 0 ||
                            strcmp(command[0], "Find") == 0 || strcmp(command[0], "Find\n") == 0)
                        {

                            const int num_posti = atoi(command[2]);
                            char *dummy = malloc(sizeof(char) * BUFFER_SIZE);
                            const int ora = atoi(command[4]);
                            int g_giorno = 0, g_mese = 0, g_anno = 0;

                            if (num_posti <= 0)
                            {
                                printf("Numero Posti non valido\n");
                                continue;
                            }

                            sprintf(dummy , "%s" , command[3]);

                            if (data_valida_str(dummy, ora, 30) == 0)
                            {
                                printf("Data Inserita non Valida");
                                continue;
                            }

                            if(strlen((char*)command[1]) > MAX_LUNGHEZZA_COGNOME || strlen((char*)command[1]) == 0){
                                printf("Cognome non Valido\n");
                                continue;
                            }

                            estrai_info_data(dummy, &g_giorno, &g_mese, &g_anno);

                            sprintf(messaggio, "%s %d %s %d", command[1],
                                    num_posti, command[3], ora);

                            
                            invia_intero(sd, 1); // dico al server che eseguo una find

                            invia_messaggio(sd, messaggio); // invio le specifche

                            printf("Attendi le opzioni selezionabili\n");

                            ricevi_messaggio(sd, messaggio); // ricevo le opzioni

                            printf("%s \n\n", messaggio); // stampo le opzioni
                        }

                        if (strcmp(command[0], "esc") == 0 || strcmp(command[0], "esc\n") == 0)
                        {
                            exit(1); // Termino il Client
                        }

                        if (strcmp(command[0], "book") == 0 || strcmp(command[0], "book\n") == 0)
                        {
                            const int table = atoi((char*)command[1]);
                            if(table <= 0){
                                continue;
                            }

                            invia_intero(sd, 2); // dico al server che intendo fare una book

                            sprintf(messaggio, "%d\n", table); // dico al server la mia scelta

                            invia_messaggio(sd, messaggio); // invio la scelta

                            printf("Attendi la risposta del Server...\n");

                            ricevi_messaggio(sd, messaggio);
                            // ricevo la risposta , dove viene mostrato il codice prenotazione

                            printf("%s \n", messaggio); // stampo la risposta
                            exit(1);                    // chiudo il client
                        }
                    }
                    continue;
                }
            }
        }
    }
    close(sd);
    return 0;
}
