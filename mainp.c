#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "operacje.h"


#define P 50    //liczba procesow prod i kons
#define MAX 10  //rozmiar buforu
#define MAX2 12 //dwa pola na indeksy zapis/odczyt
#define PUSTY 1 //typ komunikatu
#define PELNY 2 //typ komunikatu

//struktura komunikatu
struct bufor {
    long mtype;
    int mvalue;
};

int main() 
{
    key_t klucz_msg, klucz_shm, klucz_sem;
    int msgId; 
    int shmId; 
    int semId;

    struct bufor komunikat;
    printf("[MAIN]:\n");

    //tworzymy kolejke komunikatow
    if ((klucz_msg = ftok(".", 'A')) == -1) 
    {
        printf("[MAIN]-> error ftok\n");
        exit(1);
    }
    msgId = msgget(klucz_msg, IPC_CREAT | 0666);
    if (msgId == -1)
	{
        printf("[MAIN]-> blad kolejki komunikatow\n"); 
        exit(1);
    }

    //tworzymy pamiec dzielona
    if ((klucz_shm=ftok(".",'B')) == -1) 
    {
        printf("[MAIN]-> error ftok\n");
        exit(1);
    }
	shmId = shmget(klucz_shm, MAX2 * sizeof(int), IPC_CREAT | 0666);
    if (shmId == -1)
	{
        printf("[MAIN]-> blad pamieci dzielonej\n"); 
        exit(1);
    }

	//wysylamy 10 komunikatow pustych w celu uruchomienia producentow
    komunikat.mtype = PUSTY;
    for (int i = 0; i < MAX; i++) {
        if (msgsnd(msgId, &komunikat, sizeof(komunikat.mvalue), 0) == -1) {
            printf("[MAIN]] blad wyslania kom. pustego\n");
            exit(1);
        }
        printf("[MAIN] wyslany pusty komunikat %d\n", i);
    }

    //tworzymy semafory dla pamieci krytycznej
    if ((klucz_sem=ftok(".",'C')) == -1) 
    {
        printf("[MAIN]-> error ftok\n");
        exit(1);
    }
    semId = alokujSemafor(klucz_sem, 2, IPC_CREAT | 0666);
    for (int k = 0; k < 2; k++)
        inicjalizujSemafor(semId, k, 1);

    //uruchamiamy producentow
    for (int i = 0; i < P; i++) 
    {
        switch (fork()) 
        {
            case -1:
                perror("[MAIN] Blad fork (mainprog)");
                exit(2);
            case 0:
                execl("./prod", "prod", NULL);
        }
    }

    //uruchamiamy konsumentow
    for (int i = 0; i < P; i++) 
    {
        switch (fork()) 
        {
            case -1:
                printf("[MAIN] Blad fork (mainprog)\n");
                exit(2);
            case 0:
                execl("./kons", "kons", NULL);
        }
    }

    for (int i = 0; i < 2 * P; i++)
        wait(NULL);

    // zwalnianie zasobow
    msgctl(msgId,IPC_RMID,NULL); 
    shmctl(shmId,IPC_RMID, NULL);
    semctl(semId,1,IPC_RMID, NULL);
    zwolnijSemafor(semId,2);

    printf("[MAIN] koniec\n");
    return 0;
}