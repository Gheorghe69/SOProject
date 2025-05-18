#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

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

#define MAX_USERS 100

typedef struct {
    char username[USERNAME_SIZE];
    int score;
} UserScore;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Utilizare: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char filepath[MY_MAX_PATH];
    snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", argv[1]);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Eroare: Nu pot deschide fisierul %s\n", filepath);
        return 1;
    }

    UserScore scores[MAX_USERS];
    int user_count = 0;
    Treasure t;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(scores[i].username, t.username) == 0) {
                scores[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found && user_count < MAX_USERS) {
            strncpy(scores[user_count].username, t.username, USERNAME_SIZE - 1);
            scores[user_count].username[USERNAME_SIZE - 1] = '\0';
            scores[user_count].score = t.value;
            user_count++;
        }
    }

    close(fd);

    for (int i = 0; i < user_count; i++) {
        printf("%s: %d\n", scores[i].username, scores[i].score);
    }

    return 0;
}