#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

// Funcție care va trata semnalul SIGUSR1 și va lista vânătorile
void handle_sigusr1(int sig) {
    printf("Monitorul a primit comanda de listare a vânătorilor.\n");

    // Deschidem directorul curent pentru a căuta vânătorile
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu pot deschide directorul curent");
        return;
    }

    struct dirent *entry;
    // Parcurgem fiecare director din directorul curent
    while ((entry = readdir(dir)) != NULL) {
        // Ignorăm directoarele ascunse (care încep cu .)
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/treasure.dat", entry->d_name);

            // Verificăm dacă există fișierul treasure.dat în directorul respectiv
            int fd = open(filepath, O_RDONLY);
            if (fd < 0) {
                continue;  // Dacă fișierul nu există, trecem la următorul director
            }

            // Numărăm comorile din fișierul treasure.dat
            int count = 0;
            // Aici presupunem că fiecare comoară ocupă exact sizeof(Treasure) bytes
            while (read(fd, NULL, sizeof(int)) == sizeof(int)) {  // presupunând un format simplu
                count++;
            }
            close(fd);
            // Afișăm informațiile despre vânătoare
            printf("Hunt: %s | Comori: %d\n", entry->d_name, count);
        }
    }

    closedir(dir);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;  // Setează handler-ul pentru SIGUSR1
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Setează semnalul SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Eroare la configurarea semnalului SIGUSR1");
        exit(1);
    }

    // Așteaptă semnale de la treasure_hub
    printf("Monitorul este pornit. Așteaptă comenzi...\n");
    while (1) {
        pause();  // Așteaptă semnale
    }

    return 0;
}
