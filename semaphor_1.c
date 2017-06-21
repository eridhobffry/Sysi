#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
 
#include "util.h" 
 
#define N_DATA   10
#define N_SHARED 5

/*
 
 --------------------------      Definieren die einzelne shm_ und sem_ Methode      -----------------------
 
 */

/* 
 
 //neue Speicher anlegen
 //
 //--------------------------
 //
 
 
 shmget (
 key_t k,            ---- the key for the segment
 int size,           ---- the size of the segment
 int flag            ---- create / use flag
 )

//
//--------------------------
//
//IPC_PRIVATE -- es funktioniert in Kindprozesse
//0666        -- es erlaubt Read ubd Write für Klient
//


 */
int shm_create(size_t size) {
    return shmget(IPC_PRIVATE, size, 0666);
}



/*
 
 //Binden speicher
 //
 //-------------------------
 //
 
 shmat (
 int shmid,          ---- share memory ID
 char *ptr,          ---- a character pointer
 int flag,           ---- acces flag
 )
 
//
//-------------------------
//
//Ruckgabewert ist eine Pointer void* , sonst wird -1
//int flag spielt keine Rolle, weil nur binden
//

 */
void* shm_pointer(int shmid) {
    return shmat(shmid, NULL, 0);
}

/* 
 
//shmctl .. shared memory control           --- eine Verwaltung von Memory
//
//--------------------------------------
//
//int shmctl(int shmid, int cmd, struct shmid_ds *buf);
//
//--------------------------------------
//
 IPC_RMID          ---     Removes the shared memory identifier that shmid specifies from the system and destroys the shared memory segment.

// Erfolgt return 0, sonst -1
//
 
 */
int shm_remove(int shmid) {
    struct shmid_ds sb;
    
    if (shmctl(shmid, IPC_RMID, &sb) < 0) {
        return -1;
    }
    
    return 0;
}

/*
 
 //neue Gruppe von Semaphoren
 //
 //--------------------------------------
 //
 //int semget(key_t key, int nsems, int semflg);
 //
 //--------------------------------------
 //
 //
 
 */
int sem_create(int n) {
    return semget(IPC_PRIVATE, n, 0666);
}




/*
 
 //
 
 //--------------------------------------
 //
 //int semctl(int semid, int semnum, int cmd, union semun arg);
 //
 //--------------------------------------
 //
 //Setval            ---     Set the value of a single semaphore
 
 */
int sem_set(int semid, int value) {
    return semctl(semid, 0, SETVAL, value);
}

/*
 
 //down funktioniert die Blockierung von Verwaltungsprozesse
 //
 //--------------------------------------
 //
 //int semop(int semid, struct sembuf *sops, size_t nsops);
 //
 //--------------------------------------
 //
 //semid             -- Identifizierung
 //sembuf (sb)       -- enthält 3 type sem_num , sem_flag , sem_op
 //nsops             -- performed in single semaphore
 //
 //sem_op is less than zero, the process must have alter permission
 //--on the semaphore set
 //
 
 */
int sem_down(int semid) {
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_flg = 0;
    sb.sem_op = -1;
    
    if (semop(semid, &sb, 1) < 0) {
        return -1;
    }
    
    return 0;
}

/*
 
 //Unterbrechungsroutine
 //
 //--------------------------------------
 //
 //int semop(int semid, struct sembuf *sops, size_t nsops);
 //
 //--------------------------------------
 //
 //If sem_op is a positive integer, the operation adds this value to the
 //--semaphore value
 //
 
 */
int sem_up(int semid) {
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_flg = 0;
    sb.sem_op =  1;
    
    if (semop(semid, &sb, 1) < 0) {
        return -1;
    }
    
    return 0;
}

/* löscht eine Gruppe von Semaphoren */
int sem_remove(int semid) {
    if (semctl(semid, 0, IPC_RMID) < 0) {
        return -1;
    }
    
    return 0;
}

/*
 
 --------------------------------------------------------------------------------------------------
 
 */
 
static int mutexid = 0;
static int semfullid = 0;
static int sememptyid = 0;
static int shmid = 0;
 
int producer() {
    int* buf;
    int* shared;
    int i;
     
    printf("producer start");
 
    shared = shm_pointer(shmid);
    if (shared == (void*)-1) {
        perror("shm_pointer");
        return 1;
    }
 
    buf = (int*)malloc(N_DATA<<2);    
    if (buf == NULL) {
        perror("malloc");
        return 1;   
    }
 
    srand48(buf[0]); /*function is used to initialize the internal buffer r(n) of lrand(48)*/
     
    for (i=0; i<N_DATA; i++) {
        buf[i] = lrand48();
        /*
         -lrand functions return values of type long in the range [0, 2**31-1]
         -The random generator used is the linear congruential algorithm and 48-bit integer arithmetic.
         */
    }   
     
    for (i=0; i<N_DATA; i++) {
     
        sem_down(sememptyid);          /* Zaehler fuer leere Plaetze dekrementieren */
        sem_down(mutexid);             /* Eintritt in kritischen Abschnitt */
 
        shared[i%N_SHARED] = buf[i];   /* neues Item in Puffer */
         
        sem_up(mutexid);               /* Verlassen des kritischen Abschnitts */
        sem_up(semfullid);             /* Zaehler fuer volle Plaetze inkrementieren. */
 
        printf("producer: shared[%d] = %d\n", i%N_SHARED, buf[i]);
    }
 
    free(buf);  
 
    printf("producer end");
    return 0;
}
 
int consumer() {
    int* shared;
    int item;
    int i;
     
    printf("consumer start");
 
    shared = shm_pointer(shmid);
    if (shared == (void*)-1) {
        perror("shm_pointer");
        return 1;
    }
 
    for (i=0; i<N_DATA; i++) {
        sem_down(semfullid);          /* Zaehler fuer volle Plaetze dekrementieren */
        sem_down(mutexid);            /* Eintritt in kritischen Abschnitt */
         
        item = shared[i%N_SHARED];    /* Item vom Puffer entnehmen */
        shared[i%N_SHARED] = 0;
 
        sem_up(mutexid);              /* Verlassen des kritischen Abschnitts */
        sem_up(sememptyid);           /* Zaehler fuer leere Plaetze inkrementieren. */
         
        printf("consumer: shared[%d] = %d\n", i%N_SHARED, item);
    }
     
    printf("consumer end");
    return 0;
}
 
int main(int argc, char* argv[]) {
    pid_t pid;  
    int status;
    int ret;    
     
    /* Zaehler fuer volle Plaetze */
    semfullid = sem_create(N_SHARED);
    if (semfullid < 0 || sem_set(semfullid, 0) < 0) {
        ret = 1;
        goto out;
    }
     
    /* Zaehler fuer leere Plaetze */
    sememptyid = sem_create(N_SHARED);
    if (sememptyid < 0 || sem_set(sememptyid, N_SHARED) < 0) {
        ret = 1;
        goto out;
    }
 
    /* Kritischer Abschnitt */
    mutexid = sem_create(1);
    if (mutexid < 0 || sem_set(mutexid, 1) < 0) {
        ret = 1;
        goto out;
    }
 
    /* Gemeinsamer Speicher */
    shmid = shm_create(N_SHARED<<2);  
    if (shmid < 0) {
        ret = 1;
        goto out;
    }   
 
    pid = fork();   /* Kopie des Prozesses erzeugen */
 
    if (pid > 0) {
        ret = consumer();
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
        }
    } else if (pid == 0) {
        ret = producer();
        exit(ret);
    } else {
        perror("fork");
        ret = 1;
    }
 
out:
    sem_remove(semfullid);
    sem_remove(sememptyid);
    sem_remove(mutexid);
    shm_remove(shmid);
 
    return ret;
}
