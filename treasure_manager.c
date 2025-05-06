#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

#define USERNAME_SIZE 32
#define CLUE_SIZE 128
#define MY_MAX_PATH 512

typedef struct {
    int ID;
    char username[USERNAME_SIZE];
    float latitudine;
    float longitudine;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

void verifica_director(const char *hunt_id) {
    // Modificare: Înlocuit _mkdir cu mkdir POSIX
    if (mkdir(hunt_id, 0755) != 0 && errno != EEXIST) {
        perror("Eroare la crearea directorului");
    }
}

void adauga_treasure(const char *hunt_id) {
    verifica_director(hunt_id);

    Treasure t;
    printf("ID comoara: ");
    if (scanf("%d", &t.ID) != 1 || t.ID <= 0) {
        printf("ID invalid. Trebuie să fie un număr pozitiv.\n");
        return;
    }

    char filepath[MY_MAX_PATH];
    snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", hunt_id);
    int fd_check = open(filepath, O_RDONLY);
    if (fd_check >= 0) {
        Treasure temp;
        while (read(fd_check, &temp, sizeof(Treasure)) == sizeof(Treasure)) {
            if (temp.ID == t.ID) {
                printf("ID-ul %d există deja!\n", t.ID);
                close(fd_check);
                return;
            }
        }
        close(fd_check);
    }

    printf("Nume Utilizator: ");
    scanf("%s", t.username);
    if (strlen(t.username) >= USERNAME_SIZE) {
        printf("Nume utilizator prea lung!\n");
        return;
    }

    printf("Latitudine: ");
    if (scanf("%f", &t.latitudine) != 1 || t.latitudine < -90.0 || t.latitudine > 90.0) {
        printf("Latitudine invalidă! Trebuie să fie între -90 și 90.\n");
        return;
    }

    printf("Longitudine: ");
    if (scanf("%f", &t.longitudine) != 1 || t.longitudine < -180.0 || t.longitudine > 180.0) {
        printf("Longitudine invalidă! Trebuie să fie între -180 și 180.\n");
        return;
    }

    getchar();
    printf("Indiciu: ");
    fgets(t.clue, CLUE_SIZE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;
    if (strlen(t.clue) == 0) {
        printf("Indiciul nu poate fi gol!\n");
        return;
    }

    printf("Valoare: ");
    if (scanf("%d", &t.value) != 1 || t.value < 0) {
        printf("Valoare invalidă! Trebuie să fie un număr pozitiv.\n");
        return;
    }

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
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu pot deschide directorul curent");
        return;
    }

    struct dirent *entry;
    char filepath[MY_MAX_PATH];
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, hunt_id) == 0) {
            found = 1;
            snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", entry->d_name);

            int fd = open(filepath, O_RDONLY);
            if (fd < 0) {
                perror("Eroare la deschiderea fisierului treasure.dat");
                closedir(dir);
                return;
            }

            Treasure t;
            printf("Comorile din vanatoarea '%s':\n", hunt_id);
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                printf("ID: %d | User: %s | Lat: %.2f | Long: %.2f | Valoare: %d\n", 
                       t.ID, t.username, t.latitudine, t.longitudine, t.value);
                printf("  Indiciu: %s\n", t.clue);
            }
            close(fd);
            closedir(dir);
            return;
        }
    }

    if (!found) {
        printf("Nu am gasit niciun director pentru hunt_id: %s\n", hunt_id);
    }
    closedir(dir);
}

void view_treasure(const char *hunt_id, int treasure_id) {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu pot deschide directorul curent");
        return;
    }

    struct dirent *entry;
    char filepath[MY_MAX_PATH];
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, hunt_id) == 0) {
            found = 1;
            snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", entry->d_name);

            int fd = open(filepath, O_RDONLY);
            if (fd < 0) {
                perror("Nu pot deschide fisierul treasure.dat");
                closedir(dir);
                return;
            }

            Treasure t;
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                if (t.ID == treasure_id) {
                    printf("Detalii comoara ID %d:\n", t.ID);
                    printf("Nume utilizator: %s\n", t.username);
                    printf("Latitudine: %.2f\n", t.latitudine);
                    printf("Longitudine: %.2f\n", t.longitudine);
                    printf("Indiciu: %s\n", t.clue);
                    printf("Valoare: %d\n", t.value);
                    close(fd);
                    closedir(dir);
                    return;
                }
            }
            printf("Comoara cu ID-ul %d nu a fost gasita in hunt-ul '%s'.\n", treasure_id, hunt_id);
            close(fd);
            closedir(dir);
            return;
        }
    }

    if (!found) {
        printf("Nu am gasit directorul hunt_id: %s\n", hunt_id);
    }
    closedir(dir);
}

void remove_treasure(const char *hunt_id, int treasure_id) {
    char filepath[MY_MAX_PATH], temp_filepath[MY_MAX_PATH];
    snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", hunt_id);
    snprintf(temp_filepath, MY_MAX_PATH, "%s/temp.dat", hunt_id);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Nu pot deschide fisierul original pentru citire");
        return;
    }

    int temp_fd = open(temp_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) {
        perror("Nu pot crea fisierul temporar");
        close(fd);
        return;
    }

    Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.ID != treasure_id) {
            write(temp_fd, &t, sizeof(Treasure));
        } else {
            found = 1;
        }
    }

    close(fd);
    close(temp_fd);

    if (!found) {
        printf("Comoara cu ID-ul %d nu a fost gasita.\n", treasure_id);
        unlink(temp_filepath);
        return;
    }

   
    if (chmod(filepath, 0644) != 0) {
        perror("Eroare la setarea permisiunilor");
    }

    if (rename(temp_filepath, filepath) != 0) {
        perror("Eroare la inlocuirea fisierului");
        if (unlink(filepath) == 0) {
            if (rename(temp_filepath, filepath) == 0) {
                printf("Comoara cu ID-ul %d a fost stearsa cu succes.\n", treasure_id);
            } else {
                perror("Redenumirea a esuat si dupa stergere");
            }
        } else {
            perror("Nu am putut sterge fisierul original");
        }
    } else {
        printf("Comoara cu ID-ul %d a fost stearsa cu succes.\n", treasure_id);
    }
}

void remove_hunt(const char *hunt_id) {
    char filepath[MY_MAX_PATH];
    snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", hunt_id);
    if (unlink(filepath) != 0) {
        perror("Nu am putut sterge treasure.dat");
    } else {
        printf("Fisierul %s a fost sters cu succes\n", filepath);
    }


    if (rmdir(hunt_id) != 0) {
        perror("Nu am putut sterge directorul");
    } else {
        printf("Directorul %s a fost sters cu succes\n", hunt_id);
    }
}

void list_hunts() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu pot deschide directorul curent");
        return;
    }

    struct dirent *entry;
    char filepath[MY_MAX_PATH];
    printf("Lista vanatorilor:\n");
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", entry->d_name);
            int fd = open(filepath, O_RDONLY);
            int count = 0;
            if (fd >= 0) {
                Treasure t;
                while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                    count++;
                }
                close(fd);
            }
            printf("Vanaatoare: %s | Numar comori: %d\n", entry->d_name, count);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Utilizare:\n");
        printf("%s add <hunt_id>\n", argv[0]);
        printf("%s list <hunt_id>\n", argv[0]);
        printf("%s view <hunt_id> <treasure_id>\n", argv[0]);
        printf("%s remove_treasure <hunt_id> <treasure_id>\n", argv[0]);
        printf("%s remove_hunt <hunt_id>\n", argv[0]);
        printf("%s list_hunts\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "add") == 0) {
        if (argc < 3) {
            printf("Specificati hunt_id\n");
            return 1;
        }
        adauga_treasure(argv[2]);
    } else if (strcmp(cmd, "list") == 0) {
        if (argc < 3) {
            printf("Specificati hunt_id\n");
            return 1;
        }
        list_treasures(argv[2]);
    } else if (strcmp(cmd, "view") == 0) {
        if (argc < 4) {
            printf("Specificati hunt_id si treasure_id\n");
            return 1;
        }
        int treasure_id = atoi(argv[3]);
        view_treasure(argv[2], treasure_id);
    } else if (strcmp(cmd, "remove_treasure") == 0) {
        if (argc < 4) {
            printf("Specificati hunt_id si treasure_id\n");
            return 1;
        }
        int treasure_id = atoi(argv[3]);
        remove_treasure(argv[2], treasure_id);
    } else if (strcmp(cmd, "remove_hunt") == 0) {
        if (argc < 3) {
            printf("Specificati hunt_id\n");
            return 1;
        }
        remove_hunt(argv[2]);
    } else if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    } else {
        printf("Comanda necunoscuta: %s\n", cmd);
        return 1;
    }

    return 0;
}