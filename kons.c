#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "operacje.h"


struct bufor{
        int mtype;
        int mvalue;
};

int *pam;
#define MAX2 12
#define MAX 10
#define PELNY 2
#define PUSTY 1
#define zapis pam[MAX+1]
#define odczyt pam[MAX]

int main()
{
   key_t klucz_msg, klucz_shm, klucz_sem;
   int msgId;
   int shmId;
   int semId;

   int i,pid;
   struct bufor komunikat;

   printf("[KONSUMENT]------------------------------------\n");


//uzyskanie dostepu do kolejki komunikatow
if ( (klucz_msg = ftok(".", 'A')) == -1 )
{
        printf("Blad ftok (kons)\n");
        exit(1);
}
msgId=msgget(klucz_msg,IPC_CREAT|0666);
if (msgId==-1)
{
    printf("Blad kolejki komunikatow\n");
    exit(1);
}

//uzyskanie dostepu do pamieci dzielonej
if ( (klucz_shm = ftok(".", 'A')) == -1 )
{
        printf("Blad ftok (kons)\n");
        exit(1);
}
shmId=shmget(klucz_shm, MAX2*sizeof(int), IPC_CREAT|0666);
if (shmId==-1)
{
    printf("Blad pamieci dzielonej\n");
    exit(1);
}

//przylaczenie pamieci dzielonej
pam = (int*)shmat(shmId, NULL, 0);

//uzyskanie dostępu do semafora
if ((klucz_sem=ftok(".",'C')) == -1)
    {
        printf("[KONSUMENT]-> error ftok\n");
        exit(1);
    }
semId = alokujSemafor(klucz_sem, 2, IPC_CREAT | IPC_EXCL | 0666);

//odbieranie/wysylanie odpowiednich komunikatow
if(msgrcv(msgId, &komunikat, sizeof(komunikat.mvalue), PELNY, 0)== -1)
{
    msgctl(msgId, IPC_RMID, NULL);
}

//sekcja krytyczna -- semafor -- operacje na pamięci dzielonej
waitSemafor(msgId,1,0);


i=*(pam+11*sizeof(int));


pid=*(pam+i*sizeof(int)) ;
printf("[KONSUMENT] -warosc[%d]: %d\n", i, pid);

//modyfikowanie indeksu
i++;
if (i == MAX)
{
    i = 0;
}
*(pam + 11 * sizeof(int)) = i;

signalSemafor(semId,1);

//wysyłam komunikty do producentów
komunikat.mtype=PUSTY;

if(msgsnd(msgId,&komunikat,sizeof(komunikat.mvalue),0)== -1)
{
    printf("[KONSUMENT] error msgsnd (kons)\n");
    shmctl(shmId, IPC_RMID, NULL);
    msgctl(msgId, IPC_RMID, NULL);
    exit(1);
}
//odlaczamy pamiec dzielona
shmdt(pam);

printf("[KONSUMENT] koniec\n");

return 0;

//msgrcv -- odbiór komunikatu

}