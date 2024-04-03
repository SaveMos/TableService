
#include "funzioni.h"

void stampa_guida_comandi()
{
    printf("****************************");
    printf(" BENVENUTO ");
    printf("****************************\n");
    printf("Digita un comando:\n\n");

    printf("1) help \t--> mostra i dettagli dei comandi\n");
    printf("2) menu \t--> mostra il menu dei piatti\n");
    printf("3) comanda \t--> invia una comanda\n");
    printf("4) conto \t--> chiede il conto\n\n");
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        perror("Too Many Arguments!\n");
        exit(1);
    }

    // Dichiarazione Costanti
    // const int port = atoi(argv[1]);

    // Dichiarazione Variabili
    char *msg = malloc(sizeof(char) * BUFFER_SIZE);
    struct sockaddr_in server_addr;

    // Dichiarazione Set
    int fd_max = 0;
    fd_set read_fs;
    fd_set master;

    stampa_guida_comandi();

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr)); // Pulisco la zona di memoria.
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "10.0.2.69", &server_addr.sin_addr);
    int ret = connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (ret < 0)
    {
        printf("Table Device --> Errore nella Connect!\n");
        exit(1);
    }

    invia_intero(sd, 2); // Dico al server che sono un table device
    int codice_prenotazione = 0;
    printf("Inserisci il Codice della Prenotazione: ");
    scanf("%d", &codice_prenotazione);

    invia_intero(sd, 0);

    invia_intero(sd, codice_prenotazione);

    bool ok = ricevi_intero(sd);

    if (!ok)
    {
        printf("Codice della Prenotazione Non Valido!\n");
        close(sd);
        exit(1);
    }
    else
    {
        printf("Benvenuto!\n");
    }

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
                        char **command;
                        command = malloc(sizeof(char *) * BUFFER_SIZE);
                        int j = 0;
                        char *back_msg = malloc(sizeof(char) * BUFFER_SIZE);
                        strcpy(back_msg, msg);

                        stringa_vettorizzata(msg, " ", &j, command);

                        if (strcmp(command[0], "help") == 0 || strcmp(command[0], "help\n") == 0 ||
                            strcmp(command[0], "Help") == 0 || strcmp(command[0], "Help\n") == 0)
                        {
                            stampa_guida_comandi();
                            continue;
                        }

                        if (
                            strcmp(command[0], "menu") == 0 || strcmp(command[0], "menu\n") == 0 ||
                            strcmp(command[0], "Menu") == 0 || strcmp(command[0], "Menu\n") == 0)
                        {

                            invia_intero(sd, 1);
                            ricevi_messaggio(sd, msg);
                            printf("\n%s", msg);
                            continue;
                        }

                        if (strcmp(command[0], "conto") == 0 || strcmp(command[0], "conto\n") == 0 ||
                            strcmp(command[0], "Conto") == 0 || strcmp(command[0], "Conto\n") == 0)
                        {
                            invia_intero(sd, 3);
                            ricevi_messaggio(sd, msg); // ricevo lo scontrino

                            printf("%s", msg); // stampo lo scontrino
                            exit(1);           // termino il processo client
                        }

                        if (strcmp(command[0], "comanda") == 0 || strcmp(command[0], "comanda\n") == 0 ||
                            strcmp(command[0], "Comanda") == 0 || strcmp(command[0], "Comanda\n") == 0)
                        {
                            invia_intero(sd, 2);
                            invia_messaggio(sd, back_msg);
                            if (ricevi_intero(sd))
                            {
                                printf("COMANDA RICEVUTA\n");
                            }
                            else
                            {
                                printf("COMANDA NON RICEVUTA\n");
                            }
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
