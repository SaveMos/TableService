#define MAX_COMANDE 200      // Massimo numero di comande
#define MAX_RICHIESTE 50     // Massimo numero di threads generabili
#define MAX_PRENOTAZIONI 200 // Massimo numero di prenotazioni
#define MAX_TAVOLI 60        // Massimo numero di tavoli

#include "funzioni.h"
// Variabili dei Threads
pthread_mutex_t Mutex_Prenotazioni;
pthread_mutex_t Mutex_Comande;
pthread_mutex_t Mutex_Termina_Tutto;
int termina_tutto = 0;
int numero_threads_in_esecuzione = 0;

// Variabili Comande
int numero_comande = 0;              // il numero totale di comannde al momento.
int numero_comande_preparazione = 0; // il numero totale di comannde in preparazione al momento.
int numero_comande_servizio = 0;     // il numero totale di comannde in servizio al momento.
int numero_comande_attesa = 0;       // il numero totale di comannde in attesa al momento.
comanda *lista_comande;              // Contiene i descrittori di tutte le comande

// Variabili Prenotazione
int n_prenotazione = 0;           // Numero delle prenotazioni nel registro.
prenotazione *lista_prenotazioni; // Registro delle prenotazioni

// Variabili Menù
menu menua; // struttura che contiene tutto il menù

// Variabili Tavoli
int numero_tavoli = 0; // numero di tavoli nel ristorante
tavolo *lista_tavoli;  // array dei descrittori dei tavoli

void stampa_nome_piatto(char *nome)
{
    if (nome == NULL)
    {
        return;
    }
    const int dim = strlen(nome);
    if (dim == 0)
    {
        return;
    }
    for (int i = 0; i < dim; i++)
    {
        if (nome[i] == '_')
        {
            printf(" ");
        }
        else
        {
            printf("%c", nome[i]);
        }
    }
}
bool tipo_piatto_valido(char c)
{
    if (c == 'A' || c == 'a')
    {
        return 1;
    }
    if (c == 'P' || c == 'p')
    {
        return 1;
    }
    if (c == 'S' || c == 's')
    {
        return 1;
    }
    if (c == 'D' || c == 'd')
    {
        return 1;
    }

    return 0;
}

bool check_termina_tutto(int new_sd)
{
    bool k = 0;
    pthread_mutex_lock(&Mutex_Termina_Tutto);
    if (termina_tutto == 1)
    {
        invia_check_connesso(new_sd, 0);
        k = 1;
    }
    else
    {
        k = 0;
    }
    pthread_mutex_unlock(&Mutex_Termina_Tutto);
    if (k)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void set_termina_tutto()
{
    pthread_mutex_lock(&Mutex_Termina_Tutto);
    termina_tutto = 1;
    pthread_mutex_unlock(&Mutex_Termina_Tutto);
}

int get_oldest_comanda()
{
    // restituisce l'indice della più vecchia comanda in attesa.
    int index_min = 1;
    int minuto_min = 70;
    int ora_min = 70;

    if (numero_comande_attesa == 0)
    {
        return -1; // Se non ci sono comande nello stato <in preparazione>
    }

    for (int i = 0; i < MAX_COMANDE; i++)
    {
        if (lista_comande[i].tavolo == 0)
        {
            continue; // Cerco le comande valide
        }

        if (lista_comande[i].numero_piatti_comanda == 0)
        {
            continue; // Cerco le comande valide
        }

        if (lista_comande[i].stato != 0)
        {
            continue; // Cerco una comanda ancora nello stato <in attesa>
        }

        if (lista_comande[i].ora_inserimento < ora_min)
        {
            ora_min = lista_comande[i].ora_inserimento;
            minuto_min = lista_comande[i].minuto_inserimento;
            index_min = i;
        }

        if (lista_comande[i].ora_inserimento == ora_min && lista_comande[i].minuto_inserimento < minuto_min)
        {
            ora_min = lista_comande[i].ora_inserimento;
            minuto_min = lista_comande[i].minuto_inserimento;
            index_min = i;
        }
    }
    return index_min;
}

void print_stato_comanda(int stato)
{
    if (stato == 0)
    {
        printf("<in attesa>\n");
        return;
    }

    if (stato == 1)
    {
        printf("<in preparazione>\n");
        return;
    }

    if (stato == 2)
    {
        printf("<in servizio>\n");
        return;
    }
}

int get_tavolo_from_prenotazione(int prenotazione, int *index_tavolo)
{
    if (prenotazione <= 0)
    {
        return 0;
    }

    for (int i = 0; i < MAX_PRENOTAZIONI; i++)
    {
        if (lista_prenotazioni[i].id_prenotazione == prenotazione)
        {
            *index_tavolo = i;
            return (lista_prenotazioni[i].numero_tavolo);
        }
    }
    return 0;
}

bool piatto_valido(char tipo, int id)
{
    for (int i = 0; i < menua.numero_piatti; i++)
    {
        if (menua.lista_piatti[i].codice == id && menua.lista_piatti[i].tipo == tipo)
        {
            return 1;
        }
    }
    return 0;
}

int get_numero_comande_del_tavolo(int tavolo)
{
    if (tavolo <= 0)
    {
        return 0;
    }

    if (tavolo > MAX_TAVOLI)
    {
        return 0;
    }

    for (int i = 0; i < MAX_TAVOLI; i++)
    {
        if (lista_tavoli[i].numero <= 0)
        {
            continue;
        }
        if (lista_tavoli[i].numero == tavolo)
        {
            if (lista_tavoli[i].numero_comande_tavolo > 0)
            {
                return lista_tavoli[i].numero_comande_tavolo;
            }
            else
            {
                int num = 0;
                for (int j = 0; j < MAX_COMANDE; j++)
                {
                    if (lista_comande[j].tavolo == tavolo)
                    {
                        num++;
                    }
                }
                lista_tavoli[i].numero_comande_tavolo = num;
                return num;
            }
        }
    }
    return 0;
}

int aggiungi_comanda(int prenotazione, char *msg)
{
    if (prenotazione <= 0)
    {
        return 0;
    }

    if (strlen(msg) == 0)
    {
        return 0;
    }

    int indice_tavolo = 0;
    const int tavolo = get_tavolo_from_prenotazione(prenotazione, &indice_tavolo);

    if (tavolo == 0)
    {
        return 0; // prenotazione non valida
    }

    char tipo_piatto;
    int id_piatto = 0, quanti_piatti = 0;
    char **command = malloc((sizeof(char *)) * (MAX_COMANDE + 1));
    int numero_piatti = 0, indice_comanda = 0, min = 0, ora = 0;
    int last_num_comande = get_numero_comande_del_tavolo(tavolo);

    for (int i = 0; i < MAX_COMANDE; i++)
    {
        if (lista_comande[i].tavolo == 0) // Cerco la prima locazione libera
        {
            lista_comande[i].tavolo = tavolo;
            lista_comande[i].numero_piatti_comanda = 0;
            lista_comande[i].stato = 0; // <In attesa>
            indice_comanda = i;
            numero_comande++;
            numero_comande_attesa++;
            lista_tavoli[indice_tavolo].numero_comande_tavolo = last_num_comande + 1;
            lista_comande[i].numero_comanda_relativa_al_tavolo = last_num_comande + 1;
            get_minuto_ora_corrente(&min, &ora);
            lista_comande[i].minuto_inserimento = min;
            lista_comande[i].ora_inserimento = ora;
            break;
        }
    }

    stringa_vettorizzata(msg, " ", &numero_piatti, command);

    /*
    msg è del tipo
    comanda A1-2 A3-4 A6-8
    Quindi
    */

    lista_comande[indice_comanda].comande = malloc(sizeof(ordine_piatto) * (numero_piatti - 1));

    for (int i = 0; i < numero_piatti - 1; i++)
    {
        if (comanda_valida(command[i + 1]) == 1)
        {
            estrai_info_comanda(command[i + 1], &tipo_piatto, &id_piatto, &quanti_piatti);
            if (piatto_valido(tipo_piatto, id_piatto))
            {
                lista_comande[indice_comanda].comande[i].codice_piatto = id_piatto;
                lista_comande[indice_comanda].comande[i].tipo_piatto = tipo_piatto;
                lista_comande[indice_comanda].comande[i].quanti = quanti_piatti;
                lista_comande[indice_comanda].numero_piatti_comanda++;
            }
        }
    }
    return 1;
}

int get_prezzo(menu *menu, char tipo, int codice)
{
    for (int i = 0; i < menu->numero_piatti; i++)
    {
        if (menu->lista_piatti[i].codice == codice && menu->lista_piatti[i].tipo == tipo)
        {
            return menu->lista_piatti[i].prezzo;
        }
    }
    return -1;
}

bool check_esistenza_tavolo(int tav)
{
    if (tav <= 0 || tav > MAX_TAVOLI)
    {
        return 0;
    }

    for (int i = 0; i < numero_tavoli; i++)
    {
        if (lista_tavoli[i].numero == tav)
        {
            return 1;
        }
    }
    return 0;
}

int prenota_tavolo(char *cognome, int numero_tavolo, int quanti, int giorno, int mese, int anno, int ora)
{
    if (!check_esistenza_tavolo(numero_tavolo))
    {
        return 0;
    }

    if (quanti <= 0)
    {
        return 0;
    }

    if (cognome == NULL || strlen(cognome) == 0 || strlen(cognome) > MAX_LUNGHEZZA_COGNOME)
    {
        return 0;
    }

    char *buf = malloc(sizeof(char) * 255);

    for (int i = 0; i <= MAX_PRENOTAZIONI; i++)
    {
        if (lista_prenotazioni[i].id_prenotazione <= 0)
        {
            lista_prenotazioni[i].id_prenotazione = n_prenotazione + 1;
            lista_prenotazioni[i].minuto_prenotazione = 0;
            lista_prenotazioni[i].ora_prenotazione = ora;
            lista_prenotazioni[i].giorno_prenotazione = giorno;
            lista_prenotazioni[i].mese_prenotazione = mese;
            lista_prenotazioni[i].anno_prenotazione = anno;
            lista_prenotazioni[i].numero_tavolo = numero_tavolo;
            lista_prenotazioni[i].posti_prenotazione = quanti;
            lista_prenotazioni[i].cliente_arrivato = 0;
            lista_prenotazioni[i].cognome_cliente = malloc(sizeof(char) * strlen((char *)cognome));
            strcpy(lista_prenotazioni[i].cognome_cliente, cognome);
            n_prenotazione++;

            // &numero_prenotazione, cognome, &numero_tavolo, &numero_clienti, data_prenotazione

            sprintf(buf, "%d %s %d %d %d-%d-%d-%d-%d %d\n",
                    lista_prenotazioni[i].id_prenotazione,
                    lista_prenotazioni[i].cognome_cliente,
                    lista_prenotazioni[i].numero_tavolo,
                    lista_prenotazioni[i].posti_prenotazione,
                    lista_prenotazioni[i].ora_prenotazione,
                    lista_prenotazioni[i].minuto_prenotazione,
                    lista_prenotazioni[i].giorno_prenotazione,
                    lista_prenotazioni[i].mese_prenotazione,
                    lista_prenotazioni[i].anno_prenotazione,
                    lista_prenotazioni[i].cliente_arrivato);
            appendi_stringa_a_file(buf, "prenotazioni.txt");

            return (n_prenotazione + 1);
        }
    }
    return 0;
}

int check_esistenza_prenotazione(int numero_pren)
{
    // restituisce l'indice della prenotazione nell'array
    if (numero_pren <= 0 || numero_pren > MAX_PRENOTAZIONI)
    {
        return -1;
    }
    for (int i = 0; i < MAX_PRENOTAZIONI; i++)
    {
        if (lista_prenotazioni[i].id_prenotazione == numero_pren)
        {
            return i;
        }
    }
    return -1;
}

void inizializza_lista_prenotazioni()
{
    FILE *ptr = fopen("prenotazioni.txt", "r");

    if (ptr == NULL)
    {
        perror("Errore nell'apertura del File Menu\n");
        exit(1);
    }

    char *cognome = malloc(sizeof(char) * MAX_LUNGHEZZA_COGNOME);
    char *data_prenotazione = malloc(sizeof(char) * 14);
    int numero_prenotazione = 0, numero_tavolo = 0, numero_clienti = 0;
    int minuto = 0, ora = 0, giorno = 0, mese = 0, anno = 0;
    int arrivato = 0;
    int i = 0;

    while (fscanf(ptr, "%d %s %d %d %s %d\n", &numero_prenotazione, cognome, &numero_tavolo, &numero_clienti, data_prenotazione, &arrivato) != EOF)
    {
        if (numero_prenotazione <= 0)
        {
            continue;
        }
        lista_prenotazioni[i].id_prenotazione = numero_prenotazione;
        strncpy(lista_prenotazioni[i].cognome_cliente, cognome, strlen(cognome));
        lista_prenotazioni[i].numero_tavolo = numero_tavolo;
        lista_prenotazioni[i].posti_prenotazione = numero_clienti;

        estrai_info_data_plus(data_prenotazione, &minuto, &ora, &giorno, &mese, &anno);

        lista_prenotazioni[i].ora_prenotazione = ora;
        lista_prenotazioni[i].minuto_prenotazione = minuto;
        lista_prenotazioni[i].giorno_prenotazione = giorno;
        lista_prenotazioni[i].mese_prenotazione = mese;
        lista_prenotazioni[i].anno_prenotazione = anno;

        lista_prenotazioni[i].cliente_arrivato = arrivato;
        n_prenotazione = i + 1;
    }

    fclose(ptr);
}

bool check_prenotato(int numero_prenotazioni, int numero_tavolo, int giorno, int mese, int anno)
{
    /*
        dato un tavolo e una data, verifica se risulta prenotato per quella data.
    */
    if (!check_esistenza_tavolo(numero_tavolo))
    {
        return 0;
    }

    for (int i = 0; i < MAX_PRENOTAZIONI; i++)
    {
        if (lista_prenotazioni[i].id_prenotazione > 0)
        {
            if (lista_prenotazioni[i].numero_tavolo == numero_tavolo)
            {
                // Se esiste un altra prenotazione quel tavolo.
                if (lista_prenotazioni[i].giorno_prenotazione == giorno &&
                    lista_prenotazioni[i].mese_prenotazione == mese &&
                    lista_prenotazioni[i].anno_prenotazione == anno)
                {
                    // Nello stesso giorno , mese e anno allora ritorna false.
                    return 0;
                }
            }
        }
    }
    return 1; // Tavolo Libero
}

int table_searcher(int numero_prenotazioni, int giorno, int mese, int anno, int posti_necessari, int numero_tavoli, char *opzioni, int *tavoli_opzioni)
{
    /*
    Date delle specifiche questa funzione restituisce delle opzioni valide per il cliente.
    L'intero che restituisce come valore di ritorno è il numero di opzioni
    */
    char *str = malloc(sizeof(char) * 100);
    int numero_opzioni = 0;
    for (int i = 0; i < MAX_TAVOLI; i++)
    {
        if (lista_tavoli[i].numero <= 0)
        {
            continue;
        }

        if (lista_tavoli[i].numero < posti_necessari)
        {
            continue;
        }

        if (check_prenotato(n_prenotazione, lista_tavoli[i].numero, giorno, mese, anno) == 0)
        {
            continue;
        }

        sprintf(str, "\n%d) T%d\tSALA%d\t%s", (numero_opzioni + 1), lista_tavoli[i].numero, lista_tavoli[i].sala, lista_tavoli[i].caratteristica);
        strcat(opzioni, str);
        tavoli_opzioni[numero_opzioni] = lista_tavoli[i].numero;
        numero_opzioni++;
    }
    return numero_opzioni;
}

void popola()
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    const int giorno_oggi = timeinfo->tm_mday;
    const int mese_oggi = timeinfo->tm_mon + 1;
    const int anno_oggi = timeinfo->tm_year - 100;
    char *buff = malloc(sizeof(char) * 45);

    FILE *p = fopen("prenotazioni.txt", "w"); // cancello il contenuto del file delle prenotazioni
    fclose(p);

    strcpy(buff, "Rossi");
    prenota_tavolo(buff, 2, 2, giorno_oggi, mese_oggi, anno_oggi, 12);
    strcpy(buff, "Verdi");
    prenota_tavolo(buff, 1, 1, giorno_oggi, mese_oggi, anno_oggi, 11);
    strcpy(buff, "Gialli");
    prenota_tavolo(buff, 3, 2, giorno_oggi, mese_oggi, anno_oggi, 13);
    strcpy(buff, "Blu");
    prenota_tavolo(buff, 29, 28, giorno_oggi, mese_oggi, anno_oggi, 14);
    strcpy(buff, "Nelli");
    prenota_tavolo(buff, 2, 2, giorno_oggi, mese_oggi, anno_oggi + 2, 12);
    strcpy(buff, "Simonini");
    prenota_tavolo(buff, 1, 1, giorno_oggi, mese_oggi, anno_oggi + 3, 11);

    for (int i = 1; i < 4; i++)
    {
        lista_prenotazioni[i].cliente_arrivato = 1;
    }

    strcpy(buff, "comanda A1-2 A3-5 P1-21 S2-4 D1-5\n");
    aggiungi_comanda(2, buff);
    strcpy(buff, "comanda A1-2 A3-5 P1-21 S2-4 D1-5\n");
    aggiungi_comanda(3, buff);
    strcpy(buff, "comanda A1-2 A3-5 P1-21 S2-4 D1-5\n");
    aggiungi_comanda(4, buff);
    strcpy(buff, "comanda A3-2 A3-5 P1-21 S1-150 D1-7\n");
    aggiungi_comanda(4, buff);

    lista_comande[0].minuto_inserimento = 59;
    lista_comande[2].ora_inserimento = 10;
    lista_comande[3].ora_inserimento = 12;
}

void cliente_soddisfatto(int prenotazione_)
{
    if (n_prenotazione <= 0)
    {
        return;
    }

    if (prenotazione_ <= 0 || prenotazione_ > MAX_PRENOTAZIONI)
    {
        return;
    }
    // elimina tutti i dati relativi alla prenotazione di un cliente
    int tav = 0;
    char *buf = malloc(sizeof(char) * BUFFER_SIZE);

    pthread_mutex_lock(&Mutex_Prenotazioni);

    for (int i = 0; i < MAX_PRENOTAZIONI; i++)
    {
        if (lista_prenotazioni[i].id_prenotazione == prenotazione_)
        {
            tav = lista_prenotazioni[i].numero_tavolo;

            sprintf(buf, "%d %s %d %d %d %d %d %d\n",
                    lista_prenotazioni[i].id_prenotazione,
                    lista_prenotazioni[i].cognome_cliente,
                    lista_prenotazioni[i].numero_tavolo,
                    lista_prenotazioni[i].posti_prenotazione,
                    lista_prenotazioni[i].ora_prenotazione,
                    lista_prenotazioni[i].giorno_prenotazione,
                    lista_prenotazioni[i].mese_prenotazione,
                    lista_prenotazioni[i].anno_prenotazione);
            elimina_riga_da_file("prenotazione.txt", buf, -1);

            lista_prenotazioni[i].id_prenotazione = 0; // Libero la Cella
            lista_prenotazioni[i].numero_tavolo = 0;   // Libero la Cella
            break;
        }
    }

    for (int i = 0; i < MAX_COMANDE; i++)
    {
        if (lista_comande[i].tavolo == tav)
        {
            lista_comande[i].tavolo = 0;
            numero_comande--;

            if (lista_comande[i].stato == 0)
            {
                numero_comande_attesa--;
            }

            if (lista_comande[i].stato == 1)
            {
                numero_comande_preparazione--;
            }

            if (lista_comande[i].stato == 2)
            {
                numero_comande_servizio--;
            }
        }
    }
    pthread_mutex_unlock(&Mutex_Prenotazioni);
}

void *codice_thread_client(void *arg)
{
    int new_sd = *((int *)arg);                       // socket di comunicazione
    char *msg_s = malloc(sizeof(char) * BUFFER_SIZE); // Buffer dei Messaggi.
    char **command = malloc((sizeof(char *)) * 4);
    int P_giorno = 0, P_mese = 0, P_anno = 0, P_ora = 0; // variabili per contenere i dettagli della data scelta.
    char *cognome_cliente = malloc(sizeof(char) * MAX_LUNGHEZZA_COGNOME);
    bool ok = 0;
    char *opzioni = malloc(sizeof(char *)); // conterrà le opzioni da inviare al client.
    int opzioni_tavoli[numero_tavoli];      // conterrà i numeri dei tavoli espressi nelle opzioni
    int numero_opzioni = 0, numero_clienti_tavolo = 0, r = 0, j = 0;
    int opzione = 0, last_opzione = 0;

    for (j = 0; j < numero_tavoli; j++)
    {
        opzioni_tavoli[j] = -1; // Inizializzo la lista delle opzioni
    }

    while (1)
    {
        opzione = ricevi_intero_con_uscita(new_sd); // il client mi dice cosa vorrebbe fare.

        if (opzione == 1)
        {
            // il client ha invocato una find.
            if (last_opzione != 0 && last_opzione != 1)
            {
                continue; // risulta che il client ha già invocato una book
            }
            else
            {
                last_opzione = 1; // segno che il client ha invocato la find
            }

            ricevi_messaggio(new_sd, msg_s); // ricevo le specifiche per la find

            stringa_vettorizzata(msg_s, " ", &j, command); // vettorizzo il messaggio per esaminare le componenti
            /*
            Il messaggio ricevuto è del tipo:
            cognome  quanti_clienti data_prenotazione ora_prenotazione
            */
            strcpy(cognome_cliente, (char *)command[0]); // salvo il cognome

            estrai_info_data(command[2], &P_giorno, &P_mese, &P_anno); // prendo <giorno , mese , anno>

            P_ora = atoi((char *)command[3]);
            numero_clienti_tavolo = atoi((char *)command[1]);

            if (!data_valida(P_ora, 0, P_giorno, P_mese, P_anno))
            {
                continue; // se la data indicata dal client non è valida esco
            }

            pthread_mutex_lock(&Mutex_Prenotazioni);
            // sezione critica, leggo e modifico la lista delle prenotazioni
            // il semaforo rimarrà bloccato per tutto il tempo compreso tra la find e la book

            numero_opzioni = table_searcher(n_prenotazione, P_giorno, P_mese, P_anno,
                                            numero_clienti_tavolo, numero_tavoli, opzioni, opzioni_tavoli);

            if (numero_opzioni == 0)
            {
                // non ci sono opzioni disponibili per il cliente
                pthread_mutex_unlock(&Mutex_Prenotazioni);
                strcpy(opzioni, "NON CI SONO OPZIONI DISPONBILI!");
                invia_messaggio(new_sd, opzioni);
                pthread_exit((void *)NULL);
            }
            else
            {
                invia_messaggio(new_sd, opzioni); // Invio le Opzioni
            }
        }

        if (opzione == 2)
        {
            // il client ha richiamato una book
            if (last_opzione != 1)
            {
                continue; // devo prima aver eseguito una find.
            }
            else
            {
                last_opzione = 2;
            }

            ricevi_messaggio(new_sd, msg_s); // Arrivo book

            if (conta_occorrenze(msg_s, ' ') > 0)
            {
                continue; // mi aspetto 1 componente
            }

            command = malloc((sizeof(char *)));

            stringa_vettorizzata(msg_s, " ", &j, command);

            const int scelta = atoi((char *)command[0]); // il secondo componente è la scelta indicata dal cliente

            ok = 1;

            if (scelta <= 0)
            {
                ok = 0;
            }

            if (scelta > numero_opzioni)
            {
                ok = 0;
            }

            if (ok == 1)
            {
                r = prenota_tavolo(cognome_cliente,
                                   opzioni_tavoli[scelta - 1], numero_clienti_tavolo,
                                   P_giorno, P_mese, P_anno, P_ora);
                if (r)
                {
                    sprintf(msg_s,
                            "PRENOTAZIONE EFFETTUATA!\nCodice della Prenotazione [ %d ]\n A Nome: %s per %d Persone.\n Il %d/%d/%d dalle %d:00\n", (r - 1), cognome_cliente, numero_clienti_tavolo, P_anno, P_mese, P_anno, P_ora);
                }
                else
                {
                    strcpy(msg_s, "ERRORE NELLA PRENOTAZIONE - Errore del Server, Riprovare!");
                }
            }
            else
            {
                strcpy(msg_s, "ERRORE NELLA PRENOTAZIONE - Opzione Non Valida!");
            }

            invia_messaggio(new_sd, msg_s); // Invio Messaggio Finale

            pthread_mutex_unlock(&Mutex_Prenotazioni);
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}

void *codice_thread_table_device(void *arg)
{
    int new_sd = *((int *)arg);
    char *msg = malloc(sizeof(char) * BUFFER_SIZE);
    char *temp = malloc(sizeof(char) * BUFFER_SIZE);

    int operazione = 0; // Operazione voluta
    int prenot = 0;     // Numero della prenotazione

    while (1)
    {
        if (termina_tutto)
        {
            pthread_exit(NULL);
        }

        operazione = ricevi_intero(new_sd);

        if (operazione == 0)
        {
            // il table device fa il login con il codice prenotazione
            prenot = ricevi_intero(new_sd);
            bool trovato = 0;

            time_t rawtime;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            const int giorno_oggi = timeinfo->tm_mday;
            const int mese_oggi = timeinfo->tm_mon + 1;
            const int anno_oggi = timeinfo->tm_year - 100;

            for (int i = 0; i < MAX_PRENOTAZIONI; i++)
            {
                if (prenot <= 0 || prenot > MAX_PRENOTAZIONI)
                {
                    break;
                }
                if (lista_prenotazioni[i].id_prenotazione == prenot)
                {
                    if (lista_prenotazioni[i].cliente_arrivato == 1)
                    {
                        // cerco prenotazioni per cui il cliente non è già arrivato
                        break;
                    }

                    if (
                        lista_prenotazioni[i].giorno_prenotazione != giorno_oggi ||
                        lista_prenotazioni[i].mese_prenotazione != mese_oggi ||
                        lista_prenotazioni[i].anno_prenotazione != anno_oggi)
                    {
                        // giorno corrente differente da quello specificato nella prenotazione
                        break;
                    }

                    trovato = 1;
                    lista_prenotazioni[i].cliente_arrivato = 1;
                    break;
                }
            }
            if (trovato == 1)
            {
                invia_intero(new_sd, 1);
            }
            else
            {
                invia_intero(new_sd, 0);
                close(new_sd);
                pthread_exit(NULL);
            }
        }

        if (operazione == 1)
        {
            // Il kitchen device chiede il menù

            for (int i = 0; i < menua.numero_piatti; i++)
            {
                sprintf(temp, "%c%d - %s\t \t %d\n",
                        menua.lista_piatti[i].tipo,
                        menua.lista_piatti[i].codice,
                        menua.lista_piatti[i].nome,
                        menua.lista_piatti[i].prezzo);
                strcat(msg, temp);
            }
            invia_messaggio(new_sd, msg);
        }

        if (operazione == 2)
        {
            // Il Table device ordina una comanda
            ricevi_messaggio(new_sd, msg);

            pthread_mutex_lock(&Mutex_Comande);

            bool ok = aggiungi_comanda(prenot, msg);

            pthread_mutex_unlock(&Mutex_Comande);

            if (ok)
            {
                invia_intero(new_sd, 1);
            }
            else
            {
                invia_intero(new_sd, 0);
            }
            continue;
        }

        if (operazione == 3)
        {
            // il table device chiede il conto
            int somma = 0, prezzo = 0;
            char *scontrino = malloc(sizeof(char) * BUFFER_SIZE);
            char *buf = malloc(sizeof(char) * BUFFER_SIZE);
            int indice_tavolo = 0;

            pthread_mutex_lock(&Mutex_Comande);

            int tavolo = get_tavolo_from_prenotazione(prenot, &indice_tavolo);

            if (tavolo <= 0)
            {
                pthread_mutex_unlock(&Mutex_Comande);
                continue; // Indice Prenotazione Non Valido
            }

            for (int i = 0; i < MAX_COMANDE; i++)
            {
                if (lista_comande[i].tavolo == tavolo)
                {
                    // cerco tutte le comande associate alla prenotazione
                    for (int j = 0; j <= lista_comande[i].numero_piatti_comanda; j++)
                    {
                        if (!tipo_piatto_valido(lista_comande[i].comande[j].tipo_piatto))
                        {
                            continue;
                        }
                        prezzo = get_prezzo(&menua,
                                            lista_comande[i].comande[j].tipo_piatto,
                                            lista_comande[i].comande[j].codice_piatto);

                        // Prendo il prezzo di ogni piatto acquistato
                        // Moltiplicato per la quantità
                        prezzo *= lista_comande[i].comande[j].quanti;

                        somma += prezzo; // Sommo

                        sprintf(buf, "%c%d %d %d\n",
                                lista_comande[i].comande[j].tipo_piatto,
                                lista_comande[i].comande[j].codice_piatto,
                                lista_comande[i].comande[j].quanti,
                                prezzo); // Inizio a creare la stringa che fingerà da scontrino
                        strcat(scontrino, buf);
                    }
                }
            }
            cliente_soddisfatto(prenot);

            pthread_mutex_unlock(&Mutex_Comande);
            sprintf(buf, "--------------\n");
            sprintf(buf, "Totale: %d\n", somma);

            strcat(scontrino, buf);

            invia_messaggio(new_sd, scontrino); // invio lo scontrino

            // Il cliente è soddisfatto e lascerà il locale
            // quindi elimino tutte le informazioni relative alla prenotazione
            pthread_exit((void *)NULL); // Il thread può terminare
        }
    }

    pthread_exit((void *)NULL);
}

void *codice_thread_kitchen_device(void *arg)
{
    int new_sd = *((int *)arg);
    char *msg = malloc(sizeof(char) * BUFFER_SIZE);
    char *temp = malloc(sizeof(char) * BUFFER_SIZE);
    int operazione = 0;

    while (1)
    {
        if (termina_tutto)
        {
            pthread_exit(NULL);
        }

        operazione = ricevi_intero_con_uscita(new_sd);

        if (operazione == 1)
        {
            // il kitchen device ha chiamato una take
            pthread_mutex_lock(&Mutex_Comande);
            const int indice_comanda_old = get_oldest_comanda();

            if (indice_comanda_old == -1)
            {
                strcpy(msg, "Non ci sono Comande Disponibili!\nRiprova più tardi!\n");
            }
            else
            {
                // invio al kitchen device la comanda espressa per intero
                /*
                codice_comanda tavolo piatto piatto ... piatto
                */
                sprintf(msg, "com%d T%d\n",
                        lista_comande[indice_comanda_old].numero_comanda_relativa_al_tavolo,
                        lista_comande[indice_comanda_old].tavolo);

                for (int i = 0; i < lista_comande[indice_comanda_old].numero_piatti_comanda; i++)
                {
                    sprintf(temp, "%c%d %d\n",
                            lista_comande[indice_comanda_old].comande[i].tipo_piatto,
                            lista_comande[indice_comanda_old].comande[i].codice_piatto,
                            lista_comande[indice_comanda_old].comande[i].quanti);

                    strcat(msg, temp);
                }
                lista_comande[indice_comanda_old].stato = 1; // Passa in <in preparazione>
                numero_comande_preparazione++;
                numero_comande_attesa--;
            }

            invia_messaggio(new_sd, msg); // invio la comanda presa al kitchen device

            if (indice_comanda_old != -1)
            {
                // se tutto è andato bene, invio due dati supplementari al kitchen device
                // che lo agevoleranno in altre elaborazioni
                invia_intero(new_sd, lista_comande[indice_comanda_old].tavolo);
                invia_intero(new_sd, lista_comande[indice_comanda_old].numero_comanda_relativa_al_tavolo);
            }

            pthread_mutex_unlock(&Mutex_Comande);
            continue;
        }

        if (operazione == 2)
        {
            // il kitchen device vuole fare una Ready
            // Ricevo le informazioni sulla comanda che è andata in servizio
            const int com = ricevi_intero(new_sd);
            const int tav = ricevi_intero(new_sd);

            if (com <= 0 && tav <= 0)
            {
                continue; // tavolo o codice comanda non validi
            }
            int ok = 0;

            // Il kitchen device mi ha inviato il numero della comanda e il tavolo
            // Relati alla comanda pronta per essere servita
            pthread_mutex_lock(&Mutex_Comande);
            for (int i = 0; i < MAX_COMANDE; i++)
            {
                if (lista_comande[i].tavolo == 0)
                {
                    continue; // Evito comande non valide
                }

                if (lista_comande[i].stato != 1)
                {
                    continue; // Sto cercando una comanda <in preparazione>
                }

                if (lista_comande[i].tavolo == tav &&
                    lista_comande[i].numero_comanda_relativa_al_tavolo == com)
                {
                    // Se la comanda esiste ed è valida
                    invia_intero(new_sd, 1);    // dò l'ok al kitchen device
                    lista_comande[i].stato = 2; // La comanda passa <in servizio>
                    numero_comande_preparazione--;
                    numero_comande_servizio++;
                    ok = 1;
                    break;
                }
            }

            if (ok == 0)
            {
                invia_intero(new_sd, 0); // dico al kitchen device che qualcosa è andato storto
            }

            pthread_mutex_unlock(&Mutex_Comande);
            continue;
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        perror("Too Many Arguments!\n");
        exit(1);
    }

    // Dichiarazione Variabili
    int ret = 0, new_sd = 0, len = 0, i = 0;
    int request_socket = 0; // Socket di ascolto.
    char msg_s[BUFFER_SIZE]; // Buffer dei Messaggi.
    struct sockaddr_in my_addr, client_addr;
    int fd_max = 0; // Massimo dei descrittori di socket nel set Master.
    fd_set read_fs; // Set dei socket copia.
    fd_set master; // Set dei socket Principale.

    // Inizializzazione Semafori dei Threads
    if (pthread_mutex_init(&Mutex_Prenotazioni, NULL) != 0)
    {
        printf("Creazione Mutex Prenotazione Fallita!\n");
        exit(1);
    }

    if (pthread_mutex_init(&Mutex_Termina_Tutto, NULL) != 0)
    {
        printf("Creazione Mutex TerminaTutto Fallita!\n");
        exit(1);
    }

    if (pthread_mutex_init(&Mutex_Comande, NULL) != 0)
    {
        printf("Creazione Mutex Comande Fallita!\n");
        exit(1);
    }

    lista_prenotazioni = malloc(sizeof(prenotazione) * MAX_PRENOTAZIONI);
    lista_comande = malloc(sizeof(comanda) * MAX_COMANDE);
    lista_tavoli = malloc(sizeof(tavolo) * MAX_TAVOLI);

    for (int i = 0; i < MAX_COMANDE; i++)
    {
        lista_comande[i].tavolo = 0;
    }

    // Dichiarazione Costanti
    const int port = atoi(argv[1]);

    for (i = 0; i < MAX_MENU; i++)
    {
        menua.lista_piatti[i].codice = 0;
        menua.lista_piatti[i].prezzo = 0;
    }
    menua.numero_piatti = 0;

    FILE *ptr = fopen("menu.txt", "r");
    if (ptr == NULL)
    {
        perror("Errore nell'apertura del File Menu!\n");
        exit(1);
    }

    char *element1 = malloc(sizeof(char) * 30);
    char *element2 = malloc(sizeof(char) * 255);
    int element3 = 0, element4 = 0, element5 = 0;
    i = 0;

    while (fscanf(ptr, "%s %s %d;\n", element1, element2, &element3) != EOF)
    {
        menua.lista_piatti[i].tipo = element1[0];
        element1++;
        menua.lista_piatti[i].codice = atoi(element1);
        strncpy(menua.lista_piatti[i].nome, element2, strlen(element2));
        menua.lista_piatti[i].prezzo = element3;
        i++;
    }
    menua.numero_piatti = i;
    fclose(ptr);

    ptr = fopen("tavoli.txt", "r");
    if (ptr == NULL)
    {
        perror("Errore nell'apertura del File Menu\n");
        exit(1);
    }
    i = 0;

    for (char c = getc(ptr); c != EOF; c = getc(ptr))
    {
        if (c == '\n')
        {
            i++;
        }
    }
    numero_tavoli = i;

    for (i = 0; i < numero_tavoli; i++)
    {
        lista_tavoli[i].numero = 0;
        lista_tavoli[i].posti = 0;
        lista_tavoli[i].numero_comande_tavolo = 0;
        lista_tavoli[i].sala = 0;
        strcpy(lista_tavoli[i].caratteristica, ".\0");
    }

    i = 0;
    rewind(ptr); // Riporto il puntatore del file all'inizio.
    while (fscanf(ptr, "%d %d %d %s", &element3, &element4, &element5, element2) != EOF)
    {
        lista_tavoli[i].numero = element3;
        lista_tavoli[i].sala = element4;
        lista_tavoli[i].posti = element5;
        strncpy(lista_tavoli[i].caratteristica, element2, strlen(element2));
        lista_tavoli[i].numero_comande_tavolo = 0;
        i++;
    }
    fclose(ptr); // Chiudo il File
    // Fine Inizializzazione Tavoli

    // Inizializzazione Prenotazioni
    for (int j = 0; j < MAX_PRENOTAZIONI; j++)
    {
        lista_prenotazioni[j].id_prenotazione = 0;
        lista_prenotazioni[j].numero_tavolo = 0;
    }
    n_prenotazione = 0;
    // inizializza_lista_prenotazioni();
    //  Fine Inizializzazione Prenotazioni

    request_socket = socket(AF_INET, SOCK_STREAM, 0); // Creo un socket TCP di ascolto per il Server.
    memset(&my_addr, 0, sizeof(my_addr));             // Pulisco la zona di memoria.
    my_addr.sin_family = AF_INET;                     // La famiglia dell'indirizzo IP del mio server.
    my_addr.sin_port = htons(port);                   // La porta del mio processo server.
    my_addr.sin_addr.s_addr = INADDR_ANY;             // Si ascolta su tutte le interfacce di rete della Macchina.
    inet_pton(AF_INET, "10.0.2.69", &my_addr.sin_addr);

    ret = bind(request_socket, (struct sockaddr *)&my_addr, sizeof(my_addr));

    // Metto il socket request_socket in ascolto.
    ret = listen(request_socket, MAX_RICHIESTE);

    if (ret == -1)
    {
        perror("SERVER ERROR: Errore nella Listen!\n");
        exit(1);
    }

    FD_ZERO(&master);  // Azzero il set master.
    FD_ZERO(&read_fs); // Azzero il set copia.

    FD_SET(request_socket, &master); // Inserisco nel set il socket di ascolto.
    FD_SET(STANDARD_INPUT, &master); // Inserisco nel set lo standard Input.
    fd_max = request_socket;         // L'ultimo inserito è request_socket.

    pthread_t vettore_thread[MAX_RICHIESTE];

    int numero_thread = 0;

    popola();

    while (1)
    {
        read_fs = master;
        select(fd_max + 1, &read_fs, NULL, NULL, NULL);

        for (i = 0; i <= fd_max; i++)
        {
            if (FD_ISSET(i, &read_fs))
            { // Il Socket i è pronto in lettura.
                if (i == STANDARD_INPUT)
                {
                    if (fgets(msg_s, BUFFER_SIZE, stdin))
                    {
                        const int dim_command = strlen(msg_s);

                        if (dim_command <= 3)
                        {
                            perror("Input Non Valido!\n");
                            continue;
                        }

                        char **command = malloc(sizeof(char *) * 2);
                        int j = 0;

                        stringa_vettorizzata(msg_s, " ", &j, command);

                        if (strcmp(command[0], "stat\n") == 0 || strcmp(command[0], "stat") == 0 ||
                            strcmp(command[0], "Stat\n") == 0 || strcmp(command[0], "Stat") == 0)
                        {
                            pthread_mutex_lock(&Mutex_Comande);

                            if (j == 1)
                            {
                                // Non ho Parametri
                                // Stampo tutte le comande
                                int num = 1;
                                for (int k = 0; k < numero_comande; k++)
                                {
                                    if (lista_comande[k].tavolo <= 0)
                                    {
                                        continue;
                                    }
                                    printf("com%d T", (num));
                                    num++;
                                    printf("%d [ %d : %d ]\n",
                                           lista_comande[k].tavolo,
                                           lista_comande[k].ora_inserimento,
                                           lista_comande[k].minuto_inserimento);

                                    for (int j = 0; j < lista_comande[k].numero_piatti_comanda; j++)
                                    {
                                        if (lista_comande[k].comande[j].codice_piatto <= 0 ||
                                            lista_comande[k].comande[j].quanti <= 0)
                                        {
                                            continue;
                                        }

                                        if (!tipo_piatto_valido(lista_comande[k].comande[j].tipo_piatto))
                                        {
                                            continue;
                                        }

                                        printf("%c%d %d\n",
                                               lista_comande[k].comande[j].tipo_piatto,
                                               lista_comande[k].comande[j].codice_piatto,
                                               lista_comande[k].comande[j].quanti);
                                    }
                                }
                            }
                            else
                            {
                                int num_tav = atoi(command[1] + 1);
                                int num = 1;
                                for (int k = 0; k < numero_comande; k++)
                                {
                                    if (lista_comande[k].tavolo != num_tav)
                                    {
                                        continue;
                                    }

                                    if (lista_comande[k].numero_piatti_comanda <= 0)
                                    {
                                        continue;
                                    }

                                    printf("com%d ", (num));
                                    print_stato_comanda(lista_comande[k].stato);
                                    num++;

                                    for (int j = 0; j < lista_comande[k].numero_piatti_comanda; j++)
                                    {
                                        if (
                                            lista_comande[k].comande[j].codice_piatto <= 0 ||
                                            lista_comande[k].comande[j].quanti <= 0)
                                        {
                                            continue;
                                        }
                                        if (!tipo_piatto_valido(lista_comande[k].comande[j].tipo_piatto))
                                        {
                                            continue;
                                        }

                                        printf("%c%d %d\n",
                                               lista_comande[k].comande[j].tipo_piatto,
                                               lista_comande[k].comande[j].codice_piatto,
                                               lista_comande[k].comande[j].quanti);
                                    }
                                }
                            }
                            pthread_mutex_unlock(&Mutex_Comande);
                            continue;
                        }

                        if (strcmp(command[0], "stop\n") == 0 || strcmp(command[0], "Stop\n") == 0)
                        {
                            if (numero_comande_attesa > 0 || numero_comande_preparazione > 0)
                            {
                                printf("ERRORE, ci sono ancora:\n[%d] comande in attesa.\n[%d] comande in preparazione.\n", numero_comande_attesa, numero_comande_preparazione);
                                continue;
                            }
                            else
                            {
                                set_termina_tutto();
                                sleep(2);
                                exit(1);
                            }
                        }

                        if (strcmp(command[0], "pren\n") == 0 || strcmp(command[0], "Pren\n") == 0)
                        {
                            for (int p = 0; p < MAX_PRENOTAZIONI; p++)
                            {
                                if (lista_prenotazioni[p].id_prenotazione > 0)
                                {
                                    printf("ID Prenotazione: %d\n", lista_prenotazioni[p].id_prenotazione);
                                    printf("Cognome Cliente: %s\n", lista_prenotazioni[p].cognome_cliente);
                                    printf("Tavolo: %d, il ", lista_prenotazioni[p].numero_tavolo);
                                    printf("%d/", lista_prenotazioni[p].giorno_prenotazione);
                                    printf("%d/", lista_prenotazioni[p].mese_prenotazione);
                                    printf("%d ", lista_prenotazioni[p].anno_prenotazione);
                                    printf("dalle %d:00 ", lista_prenotazioni[p].ora_prenotazione);
                                    printf("Per %d Persone.\n", lista_prenotazioni[p].posti_prenotazione);
                                    if (lista_prenotazioni[p].cliente_arrivato)
                                    {
                                        printf("Il Cliente è seduto al Tavolo\n\n");
                                    }
                                    else
                                    {
                                        printf("Il Cliente non è ancora arrivato\n\n");
                                    }
                                }
                            }
                            continue;
                        }

                        if (strcmp(command[0], "menu\n") == 0 || strcmp(command[0], "Menu\n") == 0 ||
                            strcmp(command[0], "menù\n") == 0 || strcmp(command[0], "Menù\n") == 0)
                        {
                            for (int p = 0; p < menua.numero_piatti; p++)
                            {
                                printf("%d)  %c%d  -  ", p + 1,
                                       menua.lista_piatti[p].tipo,
                                       menua.lista_piatti[p].codice);
                                stampa_nome_piatto(menua.lista_piatti[p].nome);
                                printf("\t \t %d.00€\n", menua.lista_piatti[p].prezzo);
                            }
                            continue;
                        }

                        if (strcmp(command[0], "tav\n") == 0 || strcmp(command[0], "Tav\n") == 0)
                        {
                            for (int i = 0; i < MAX_TAVOLI; i++)
                            {
                                if (lista_tavoli[i].numero == 0)
                                {
                                    continue;
                                }
                                printf("%d - %d - %d - %s\n", lista_tavoli[i].numero, lista_tavoli[i].sala, lista_tavoli[i].posti, lista_tavoli[i].caratteristica);
                            }

                            continue;
                        }
                    }
                    continue;
                }

                if (i == request_socket)
                {
                    // E' il request_socket.
                    len = sizeof(client_addr);
                    new_sd = accept(request_socket, (struct sockaddr *)&client_addr, (socklen_t *)&len);

                    FD_SET(new_sd, &master);

                    if (new_sd > fd_max)
                    {
                        fd_max = new_sd;
                    }

                    const int tipo_richesta = ricevi_intero(new_sd);

                    if (tipo_richesta > 3 || tipo_richesta <= 0)
                    {
                        continue;
                    }
                    if (tipo_richesta == 1)
                    {
                        if (pthread_create(&vettore_thread[numero_thread++], (pthread_attr_t *)NULL, &codice_thread_client, (void *)&new_sd) != 0)
                        {
                            printf("Creazione Thread Client Fallita!\n");
                        }
                    }
                    if (tipo_richesta == 2)
                    {
                        if (pthread_create(&vettore_thread[numero_thread++], (pthread_attr_t *)NULL, &codice_thread_table_device, (void *)&new_sd) != 0)
                        {
                            printf("Creazione Thread Table Device Fallita!\n");
                        }
                    }
                    if (tipo_richesta == 3)
                    {
                        if (pthread_create(&vettore_thread[numero_thread++], (pthread_attr_t *)NULL, &codice_thread_kitchen_device, (void *)&new_sd) != 0)
                        {
                            printf("Creazione Thread Kitchen Device Fallita!\n");
                        }
                    }
                    continue;
                }
                // Socket Non Valido
                FD_CLR(i, &master);
                close(i);
            }
        }
    }
}
