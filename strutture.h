   
    #define BUFFER_SIZE 1024
    #define STANDARD_INPUT 0 // Socket ID dello Stantard Input

    #define ORARIO_APERTURA 10
    #define ORARIO_CHIUSURA 23

    #define MAX_LUNGHEZZA_COGNOME 50
    
    typedef struct{
        char tipo;
        int codice;
        char nome[255]; 
        int prezzo;
    } piatto;

    typedef struct{
        piatto lista_piatti[MAX_MENU];
        int numero_piatti; // Quanti piatti ci sono nel menù
    } menu;

    typedef struct{
        char tipo_piatto; // A, P, S o D
        int codice_piatto; 
        int quanti; // quanti piatti sono stati ordinati
    } ordine_piatto;

    typedef struct{
        ordine_piatto* comande;
        int tavolo;
        int numero_piatti_comanda;
        int stato; // stato della comanda
        int numero_comanda_relativa_al_tavolo; // quante comande sono state ordinate dal tavolo dopo che il cliente si è seduto

        int minuto_inserimento;
        int ora_inserimento;
    } comanda;

     typedef struct{
        short numero;
        short sala;
        short posti;
        char caratteristica[255];
        int numero_comande_tavolo;
    } tavolo;

    typedef struct{
        int id_prenotazione;
        int numero_tavolo;

        int minuto_prenotazione;
        int ora_prenotazione;
        int giorno_prenotazione;
        int mese_prenotazione;
        int anno_prenotazione;

        int posti_prenotazione;

        char* cognome_cliente;

        bool cliente_arrivato; // vale 1 se il cliente è effettivamente seduto al tavolo
    } prenotazione;
    