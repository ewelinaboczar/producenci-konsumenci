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

#define MAX 10
#define MAX2 12
#define PELNY 2
#define PUSTY 1
#define odczyt pam[MAX]
#define zapis pam[MAX+1]

int main()
{
   key_t klucz_msg, klucz_shm, klucz_sem;
   int msgId;
   int shmId;
   int semId;

   int index,pid;
   struct bufor komunikat;


printf("[PRODUCENT]------------------------------------\n");

//uzyskanie dostepu do kolejki komunikatow
if ( (klucz_msg = ftok(".", 'A')) == -1 )
{
        printf("Blad ftok (prod)\n");
        exit(1);
}
msgId=msgget(klucz_msg,IPC_CREAT|0666);
if (msgId==-1)
{
    printf("Blad kolejki komunikatow\n");
    exit(1);
}

//uzyskanie dostepu do pamieci dzielonej
if ( (klucz_shm = ftok(".", 'B')) == -1 )
{
        printf("Blad ftok (prod)\n");
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
        printf("[PRODUCENT]-> error ftok\n");
        exit(1);
    }
semId = alokujSemafor(klucz_sem, 2, IPC_CREAT | 0666);
    for (int k = 0; k < 2; k++)
        inicjalizujSemafor(semId, k, 1);

//wysylanie/odbieranie odpowiednich komunikatow
//czekamy na konsumenta
if(msgrcv(msgId, &komunikat, sizeof(komunikat.mvalue), PUSTY, 0)==-1)
    msgctl(msgId,IPC_RMID,NULL);

//operacje na pamieci dzielonej w sekcji krytycznej -- semafory
waitSemafor(semId,0,0);

index=*(pam+10*sizeof(int)); //czytanie indeksu

//produkcja - dodanie rekordu do puli buforow  pod indeks - zapis  -- getpid()
pid=getpid();
*(pam+index*sizeof(int)) = pid;
printf("[PRODUCENT]wyslano komunikat: %s",komunikat.mtype);
printf("[PRODUCENT]PID:%d zapis do bufora nr: %d: %d\n",pid, index, pid);

index++;
if(index==MAX) //modyfikowanie indeksu
{
    index=0;
}
*(pam+10*sizeof(int))=index;

signalSemafor(semId,0);


//wyslanie odpowiedniego komunikatu
komunikat.mtype=PELNY;
if(msgsnd(msgId,&komunikat,sizeof(komunikat.mvalue),0)== -1)
{
    printf("[PRODUCENT] error msgsnd (prod)\n", komunikat.mvalue);
    shmctl(shmId, IPC_RMID, NULL);
    msgctl(msgId, IPC_RMID, NULL);
    exit(1);
}

//odlaczamy pamiec dzielona
shmdt(pam);
printf("[PRODUCENT] koniec\n");

return 0;

}

