#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <direct.h>

#define USERNAME_SIZE 32
#define CLUE_SIZE 128
#define MY_MAX_PATH 256

typedef struct {
    int ID;
    char username[USERNAME_SIZE];
    float latitudine;
    float longitudine;
    char clue[CLUE_SIZE];
    int value;
} Treasure;



void verifica_director(const char *hunt_id) {
    if (_mkdir(hunt_id) != 0) {
        perror("Eroare la crearea directorului");
    }
}


void adauga_treasure(const char *hunt_id) {
    verifica_director(hunt_id);

    Treasure t;
    printf("ID comoara: ");
    scanf("%d", &t.ID);
    printf("Nume Utilizator: ");
    scanf("%s", t.username);
    printf("Latitudine: ");
    scanf("%f", &t.latitudine);
    printf("Longitudine: ");
    scanf("%f", &t.longitudine);
    getchar();  // pentru a curăța newline-ul rămas
    printf("Indiciu: ");
    fgets(t.clue, CLUE_SIZE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;  // îndepărtează newline-ul
    printf("Valoare: ");
    scanf("%d", &t.value);

    char filepath[MY_MAX_PATH];
    snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", hunt_id);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Eroare la deschiderea fisierului");
        return;
    }
    write(fd, &t, sizeof(Treasure));
    close(fd);
    printf("Comoara adaugata in '%s'\n", hunt_id);
}

void list_treasures(const char *hunt_id) {
    char filepath[MY_MAX_PATH];
    snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", hunt_id);

    struct stat st;
    if (stat(filepath, &st) < 0) {
        perror("Nu pot accesa fisierul treasure.dat");
        return;
    }

    printf("Vanatoare: %s\n", hunt_id);
    printf("Dimensiune fisier: %ld bytes\n", st.st_size);
    printf("Ultima modificare: %s", ctime(&st.st_mtime));

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Eroare la deschiderea fisierului");
        return;
    }

    Treasure t;
    printf("\nLista de comori:\n");
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d | User: %s | Lat: %.2f | Long: %.2f | Valoare: %d\n", 
            t.ID, t.username, t.latitudine, t.longitudine, t.value);
        printf("  Indiciu: %s\n", t.clue);
    }
    close(fd);
}
void view_treasure(const char *hunter_id,int treasure_id){
    char filepath[MY_MAX_PATH];
    snprintf(filepath,MY_MAX_PATH,"%s/treasure.dat",hunter_id);
    
    struct stat st;
    if(stat(filepath,&st)<0){
        perror("nu pot accesa fisierul");
        return;
    }
    int fd=open(filepath,O_RDONLY);
    if(fd<0){
        perror("nu pot deschide fisierul");
        return;
    }
    Treasure t;
    while(read(fd,&t,sizeof(Treasure))==sizeof(Treasure)){
        if(t.ID==treasure_id){
            printf("Detalii comoara ID %d:\n",t.ID);
            printf("Nume utilizator:%s\n",t.username);
            printf("Latitudinea:%f\n",t.latitudine);
            printf("Longitudinea:%f\n",t.longitudine);
            printf("Indiciu:%s\n",t.clue);
            printf("Valoare:%d\n",t.value);
            close(fd);
            return;
        }
    }
printf("Comoara cu ID-ul %d nu a fost gasita nicaieri",treasure_id);
close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Utilizare:\n");
        printf("%s add <hunt_id>\n", argv[0]);
        printf("%s list <hunt_id>\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    const char *hunt_id = argv[2];

    if (strcmp(cmd, "add") == 0) {
        adauga_treasure(hunt_id);
    } else if (strcmp(cmd, "list") == 0) {
        list_treasures(hunt_id);
    }else if (strcmp(cmd,"view")==0){
        if(argc<4){
            printf("Pentru a apela acesta functie este necesar de specificat ID-ul comorii");
            return 1;
        }
        int treasure_id=atoi(argv[3]);
        view_treasure(hunt_id,treasure_id);
    } else {
        printf("Comanda necunoscuta\n");
        return 1;
    }

    return 0;
}
