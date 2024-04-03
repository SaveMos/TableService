
#include "funzioni.h"

typedef struct
{
    int numero;
    int tavolo;
    char *ordini;
    bool serv;
} comanda_kd; // struttura che esprime la comanda in un formato utile al kitchen device

void stampa_guida_comandi()
{
    printf("take --> Accetta una comanda\n");
    printf("show --> Mostra le comande accettate (in preparazione)\n");
    printf("ready --> Imposta lo stato della comanda\n");
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        perror("Too Many Arguments!\n");
        exit(1);
    }

    // Dichiarazione Variabili
    int sd;
    char *msg = malloc(sizeof(char) * BUFFER_SIZE);
    struct sockaddr_in server_addr;

    // Dichiarazione Set
    int fd_max;
    fd_set read_fs;
    fd_set master;

    // Variabili delle Comande Accettate
    comanda_kd *lista_comande_accettate = malloc(sizeof(comanda_kd) * MAX_COMANDE);
    // Contiene tutte le comande accettate da questo kd
    int numero_di_comande_accettate = 0;

    for (int i = 0; i < MAX_COMANDE; i++)
    {
        lista_comande_accettate[i].numero = 0;
        lista_comande_accettate[i].tavolo = 0;
    }

    stampa_guida_comandi();

    sd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr)); // Pulisco la zona di memoria.
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "10.0.2.69", &server_addr.sin_addr);
    int ret = connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (ret < 0)
    {
        printf("Errore nella Connect!\n");
        exit(1);
    }

    invia_intero(sd, 3); // Dico al server che sono un kitchen device

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
                        char **command = malloc(sizeof(char *) * 2);
                        int j = 0;
                        char *back_msg = malloc(sizeof(char) * BUFFER_SIZE);
                        strcpy(back_msg, msg);

                        stringa_vettorizzata(msg, " ", &j, command);

                        if (strcmp(command[0], "take") == 0 || strcmp(command[0], "take\n") == 0 ||
                            strcmp(command[0], "Take") == 0 || strcmp(command[0], "Take\n") == 0)
                        {
                            int num_comanda_take = -1;
                            for (int i = 0; i < MAX_COMANDE; i++)
                            {
                                if (lista_comande_accettate[i].tavolo == 0) // Prendo la prima posizione libera
                                {
                                    num_comanda_take = i;
                                    break;  
                                }
                            }
                            if (num_comanda_take == -1)
                            {
                                continue; // Raggiunto il limite massimo di comande da prendere
                            }
                            
                            invia_intero(sd, 1);
                            ricevi_messaggio(sd, msg);
                            printf("%s", msg);

                            if(msg[0] == 'N'){
                                continue; // non ci sono comande disponibili
                            }

                            lista_comande_accettate[num_comanda_take].ordini = malloc(sizeof(char) * strlen(msg));
                            strcpy(lista_comande_accettate[num_comanda_take].ordini, msg);

                            lista_comande_accettate[num_comanda_take].tavolo = ricevi_intero(sd);
                            lista_comande_accettate[num_comanda_take].numero = ricevi_intero(sd);
                            lista_comande_accettate[num_comanda_take].serv = 0;
                            numero_di_comande_accettate++;
                            continue;
                        }
                        if (
                            strcmp(command[0], "show") == 0 || strcmp(command[0], "show\n") == 0 ||
                            strcmp(command[0], "Show") == 0 || strcmp(command[0], "Show\n") == 0)
                        {
                            for (int i = 0; i < MAX_COMANDE; i++)
                            {
                                if (lista_comande_accettate[i].tavolo == 0 
                                || lista_comande_accettate[i].serv == 1) // solo comande in preparazione
                                {
                                    continue;
                                }
                                printf("%s", lista_comande_accettate[i].ordini);
                            }
                            continue;
                        }

                        if (strcmp(command[0], "ready") == 0 || strcmp(command[0], "ready\n") == 0 ||
                            strcmp(command[0], "Ready") == 0 || strcmp(command[0], "Ready\n") == 0)
                        {
                            char *buff = malloc(sizeof(char) * 20);
                            int num_parti = 0;

                            if (strlen(command[1]) < 7)
                            {
                                printf("Formato Errato, Riprovare\n");
                                continue;
                            }
                            strcpy(buff, command[1]);

                            command = malloc(sizeof(char *) * 2);

                            stringa_vettorizzata(buff, "-", &num_parti, command);

                            if (num_parti != 2 || strlen(command[0]) < 4 || strlen(command[1]) < 2)
                            {
                                printf("Formato Errato, Riprovare\n");
                                continue;
                            }
                            command[0] += 3;                        // faccio avanzare di 3 posizioni il puntatore per raggiungere il numero della comanda
                            int num_com = atoi((char *)command[0]); // Numero della Comanda

                            command[1] += 1;                        // faccio avanzare di 1 posizione il puntatore per raggiungere il numero del tavolo
                            int num_tav = atoi((char *)command[1]); // Numero del tavolo

                            int ok = 0;
                            int indice_serv = 0;
                            for (int i = 0; i < MAX_COMANDE; i++)
                            {       
                                if (
                                    lista_comande_accettate[i].tavolo == num_tav &&
                                    lista_comande_accettate[i].numero == num_com)
                                {
                                    invia_intero(sd, 2);

                                    invia_intero(sd, num_com);

                                    invia_intero(sd, num_tav);
                                    ok = 1;
                                    indice_serv = i;
                                    break;
                                }
                            }
                            if (ok == 0)
                            {
                                invia_intero(sd, 0);
                                invia_intero(sd, 0);
                            }
                            else
                            {
                                ok = ricevi_intero(sd);

                                if (ok == 1)
                                {
                                    printf("COMANDA IN SERVIZIO\n");
                                    lista_comande_accettate[indice_serv].serv = 1;
                                }
                                else
                                {
                                    printf("COMANDA NON IN SERVIZIO\n");
                                }
                            }
                            continue;
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
