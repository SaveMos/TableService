#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <signal.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>

#define MAX_MENU 100
#define MAX_COMANDE 200

#include "strutture.h"

int copia_file_in_string(char *buffer, const char *file)
{

    int i = 0; // Numero di Caratteri
    FILE *ptr = fopen(file, "r");
    char ch = fgetc(ptr);

    do
    {
        buffer[i] = ch;
        i++;
        ch = fgetc(ptr);
    } while (ch != EOF);
    fclose(ptr);
    return i;
}

void appendi_stringa_a_file(char *buffer, const char *file)
{
    FILE *ptr = fopen(file, "a+");
    fprintf(ptr, "%s", buffer);
    fclose(ptr);
}

int conta_occorrenze(char *msg, const char carattere)
{
    const int n = strlen(msg);

    if (n == 0)
    {
        return 0;
    }

    int occ = 0;
    for (int i = 0; i < n; i++)
    {
        if (msg[i] == carattere)
        {
            occ++;
        }
    }
    return occ;
}

int conta_righe_stringa(char *msg)
{
    return (conta_occorrenze(msg, '\n'));
}

void stringa_vettorizzata(char *msg_, const char *delimiter, int *numero_parti, char **command)
{
    char *token;
    int j = 0;

    char *msg = malloc(sizeof(char) * strlen(msg_));
    strcpy(msg, msg_);

    token = strtok(msg, delimiter);
    while (token != NULL)
    {
        command[j] = malloc(sizeof(char *));
        strcpy(command[j], token);
        token = strtok(NULL, delimiter);
        j++;
    }
    *numero_parti = j;
}

void stringa_de_vettorizzata(char *msg_, const char *delimiter, int numero_parti, char **command)
{
    char *t = malloc(sizeof(char) * BUFFER_SIZE * BUFFER_SIZE);
    strcpy(t, "");

    for (int i = 0; i < numero_parti; i++)
    {
        if (strlen((char *)command[i]) == 0)
        {
            continue;
        }

        if (strcmp((char *)command[i], (char *)delimiter) == 0)
        {
            continue;
        }

        strcat((char *)t, (char *)command[i]);
        strcat((char *)t, (char *)delimiter);
    }

    strcpy((char *)msg_, (char *)t);
}

bool elimina_riga_da_file(const char *file, char *row, int indice)
{
    if ((indice < 0 && row == NULL) || file == NULL)
    {
        return 0;
    }

    char *buffer = malloc(sizeof(char) * (BUFFER_SIZE));
    int num = copia_file_in_string(buffer, file);
    bool ok = 0;

    int numero_righe = conta_occorrenze(buffer, '\n');

    char **command = malloc(sizeof(char *) * (numero_righe + 1));

    stringa_vettorizzata(buffer, "\n", &numero_righe, command);

    if (row != NULL)
    {
        // Seleziono la riga per confronto.
        for (int i = 0; i < numero_righe; i++)
        {
            if (strncmp((char *)command[i], (char *)row, strlen((char *)row)) == 0)
            {
                strcpy((char *)command[i], "");
                ok = 1;
                break;
            }
        }
    }
    else
    {
        // seleziono la riga in base all'indice.
        for (int i = 0; i < numero_righe; i++)
        {
            if (i == indice)
            {
                strcpy((char *)command[i], "");
                ok = 1;
                break;
            }
        }
    }

    if (ok == 0)
    {
        return 0;
    }

    // Ora cancello il File
    FILE *ptr = fopen(file, "w");
    fclose(ptr);

    char *rec = malloc(sizeof(char) * (num + numero_righe + 2));
    stringa_de_vettorizzata(rec, "\n", numero_righe, command);
    // Sovrascrivo con la riga cancellata
    appendi_stringa_a_file(rec, file);

    return 1;
}

void stringa_vettorizzata_precisa(char *msg_, const char *delimiter, int *numero_parti, char **command, int max_parole)
{
    char *token;
    int j = 0;

    char *msg = malloc(sizeof(char) * strlen(msg_));
    strcpy(msg, msg_);

    token = strtok(msg, delimiter);
    while (token != NULL)
    {
        command[j] = malloc(sizeof(char *));
        strcpy(command[j], token);
        token = strtok(NULL, delimiter);
        j++;

        if (j == max_parole)
        {
            break;
        }
    }
    *numero_parti = j;
}

void invia_messaggio(int sd, char *msg)
{
    int l_dim_msg = 0, dim_msg = 0, ret = 0;

    msg[strlen(msg)] = '\0';    // Applico la marca di fine stringa.
    dim_msg = strlen(msg) + 1;  // Aggiorno la dimensione.
    l_dim_msg = htons(dim_msg); // Porto in formato network la dimensione.

    ret = send(sd, (void *)&l_dim_msg, sizeof(uint16_t), 0); // invio la lunghezza del messaggio
    if (ret < 0)
    {
        pthread_exit((void*)NULL);
    }

    ret = send(sd, (void *)msg, dim_msg, 0); // Invio il messaggio
    if (ret < 0)
    {
        pthread_exit((void*)NULL);
    }
}

void ricevi_messaggio(int sd, char *msg)
{
    int dim_msg = 0, ret = 0, quanti_byte = 0;

    ret = recv(sd, (void *)&dim_msg, sizeof(uint16_t), 0);

    if (ret < 0)
    {
        pthread_exit((void*)NULL);
    }

    dim_msg = ntohs(dim_msg);
    ret = recv(sd, (void *)msg, dim_msg, 0);

    if (ret < 0)
    {
       pthread_exit((void*)NULL);
    }
    else
    {
        if (ret < dim_msg)
        {
            quanti_byte = ret;
            do
            {
                ret = recv(sd, &msg[quanti_byte], dim_msg, 0);
                quanti_byte += ret;
            } while (quanti_byte < dim_msg);
        }
    }
}

void invia_intero(int sd, int mess)
{
    uint32_t msg = htonl(mess);
    int ret = send(sd, (void *)&msg, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        pthread_exit((void *)NULL);
    }
}

int ricevi_intero(int sd)
{
    uint32_t msg = 0;
    int ret = recv(sd, (void *)&msg, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        return -1;
    }
    else
    {
        return (int)(ntohl(msg));
    }
}

int ricevi_intero_con_uscita(int sd)
{
    uint32_t msg = 0;
    int ret = recv(sd, (void *)&msg, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        pthread_exit((void *)NULL);
    }
    else
    {
       return (int)(ntohs(msg));
    }
}

bool invia_check_connesso(int sd, int mode)
{
    // se mode == 0 , dico al destinatario di disconnettersi
    // se mode != 0 è solo un check della connessione

    if (mode != 0)
    {
        mode = 1;
    }

    uint16_t msg = mode;
    uint16_t check = mode;
    int ret = 1;

    ret = send(sd, (void *)&msg, sizeof(uint16_t), 0);
    if (ret < 0)
    {
        return 0;
    }
    ret = recv(sd, (void *)&msg, sizeof(uint16_t), 0);
    if (ret < 0)
    {
        return 0;
    }

    if (msg != check)
    {
        return 0;
    }

    return 1;
}

bool ricevi_check_connesso(int sd)
{
    uint16_t msg = 0;
    uint16_t check = 0;
    int ret = 0;

    ret = recv(sd, (void *)&msg, sizeof(uint16_t), 0);
    if (ret < 0)
    {
        return 0;
    }

    check = msg;

    ret = send(sd, (void *)&msg, sizeof(uint16_t), 0);
    if (ret < 0)
    {
        return 0;
    }

    if (check == 0)
    {
        // il mittente vuole che chiuda la connessione
        return 0;
    }

    return 1;
}

void estrai_info_data(char *data, int *giorno, int *mese, int *anno)
{
    int m = 0;
    char **command;
    command = malloc((sizeof(char *)) * 3);
    stringa_vettorizzata(data, "-", &m, command);

    *giorno = atoi((char *)command[0]);
    *mese = atoi((char *)command[1]);
    *anno = atoi((char *)command[2]);
}

void estrai_info_data_plus(char *data, int *minuto, int *ora, int *giorno, int *mese, int *anno)
{
    int m = 0;
    char **command;
    command = malloc((sizeof(char *)) * 5);
    stringa_vettorizzata(data, "-", &m, command);
    *ora = atoi(command[0]);
    *minuto = atoi(command[1]);
    *giorno = atoi(command[2]);
    *mese = atoi(command[3]);
    *anno = atoi(command[4]);
}

void get_minuto_ora_corrente(int *minuto, int *ora)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    *minuto = timeinfo->tm_min;
    *ora = timeinfo->tm_hour;
}

bool data_valida(int ora, int minuto, int giorno, int mese, int anno_f)
{
    // controlla che la data inserita come inpout sia valida e che sia successiva alla data odierna
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    const int giorno_current = timeinfo->tm_mday;
    const int mese_current = timeinfo->tm_mon + 1;
    const int anno_current = timeinfo->tm_year + 1900;
    const int ora_current = timeinfo->tm_hour;
    const int minuto_current = timeinfo->tm_min;

    const int anno = anno_f + 2000;

    if (ora < ORARIO_APERTURA || ora > ORARIO_CHIUSURA)
    {
        return 0;
    }

    if (minuto > 59 || minuto < 0)
    {
        return 0;
    }

    if (giorno <= 0 || mese <= 0)
    {
        return 0;
    }

    if (anno < anno_current)
    {
        return 0;
    }

    if (anno > 3000)
    {
        return 0;
    }

    if (anno == anno_current && mese_current > mese)
    {
        return 0;
    }

    if (anno == anno_current && mese_current == mese && giorno_current > giorno)
    {
        return 0;
    }

    if (anno == anno_current && mese_current == mese && giorno_current == giorno && ora_current > ora)
    {
        return 0;
    }

    if (anno == anno_current && mese_current == mese && giorno_current == giorno && ora_current == ora && minuto_current > minuto)
    {
        return 0;
    }

    if (mese == 2 && giorno >= 29) // Validità nel mese di febbraio
    {
        if (giorno > 29)
        {
            return 0;
        }

        if (giorno == 29 && ((anno % 400 != 0) && ((anno % 4 != 0) || (anno % 100 == 0))))
        {
            return 0;
        }
    }

    if (mese == 4 || mese == 6 || mese == 9 || mese == 11)
    {
        if (giorno > 30)
        {
            return 0;
        }
    }
    else
    {
        if (giorno > 31)
        {
            return 0;
        }
    }

    return 1;
}

void estrai_info_comanda(char *com, char *tipo, int *numero, int *quanti)
{
    char **command;
    int s;
    command = malloc(sizeof(char *) * 2);
    stringa_vettorizzata(com, "-", &s, command);

    *tipo = (char)command[0][0];
    command[0]++;
    *numero = atoi(command[0]);
    *quanti = atoi(command[1]);
}

bool comanda_valida(char *com)
{
    if (strlen(com) < 4)
    {
        return 0;
    }

    if (conta_occorrenze(com, '-') != 1)
    {
        return 0;
    }

    int a = 0;
    char **command = malloc(sizeof(char *) * strlen(com));

    stringa_vettorizzata(com, "-", &a, command);

    if (a != 2)
    {
        return 0;
    }
    const char tipo = command[0][0];

    if (
        tipo != 'A' && tipo != 'P' && tipo != 'S' && tipo != 'D' &&
        tipo != 'a' && tipo != 'p' && tipo != 's' && tipo != 'd')
    {
        return 0;
    }

    if (tipo == 'a')
    {
        command[0][0] = 'A';
    }

    if (tipo == 'p')
    {
        command[0][0] = 'P';
    }

    if (tipo == 's')
    {
        command[0][0] = 'S';
    }

    if (tipo == 'd')
    {
        command[0][0] = 'D';
    }

    command[0]++;

    for (int i = 0; i < strlen((char *)command[0]); i++)
    {
        if (!isdigit(command[0][i]))
        {
            return 0;
        }
    }

    for (int i = 0; i < strlen((char *)command[1]); i++)
    {
        if (command[1][i] == '\n' || command[1][i] == '\0')
        {
            continue;
        }
        if (!isdigit(command[1][i]))
        {
            return 0;
        }
    }

    return 1;
}

bool data_valida_str(char *com, int ora, int minuto)
{
    if (strlen(com) < 5)
    {
        return 0;
    }

    if (strlen(com) > 8)
    {
        return 0;
    }
    int num_ = conta_occorrenze(com, '-');

    if (num_ != 2)
    {
        return 0;
    }

    int a = 0;
    char **command = malloc(sizeof(char *) * (num_ + 1));

    stringa_vettorizzata(com, "-", &a, command);

    if (a != 3)
    {
        return 0;
    }

    a = data_valida(ora, minuto, atoi((char *)command[0]), atoi((char *)command[1]), atoi((char *)command[2]));

    if (a == 0)
    {
        return 0;
    }

    return 1;
}
