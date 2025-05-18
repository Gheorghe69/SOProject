#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>

#define USERNAME_SIZE 32
#define CLUE_SIZE 128
#define MY_MAX_PATH 512
#define HUNT_ID_FILE "hunt_id.tmp"
#define VIEW_TREASURE_FILE "view_treasure.tmp"
#define MONITOR_STOP_DELAY 2000000 // 2 seconds (in microseconds)
#define MAX_OUTPUT_SIZE 4096 // Buffer for pipe output

typedef struct {
    int ID;
    char username[USERNAME_SIZE];
    float latitudine;
    float longitudine;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

pid_t monitor_pid = 0;
int monitor_stopping = 0;
int pipe_fd[2]; // Pipe for monitor to main process communication

void send_to_pipe(const char *message) {
    if (write(pipe_fd[1], message, strlen(message)) < 0) {
        perror("Eroare la scrierea în pipe");
    }
}

void list_hunts() {
    DIR *dir = opendir(".");
    if (!dir) {
        send_to_pipe("Eroare: Nu pot deschide directorul curent\n");
        return;
    }

    struct dirent *entry;
    char filepath[MY_MAX_PATH];
    char buffer[MAX_OUTPUT_SIZE];
    snprintf(buffer, MAX_OUTPUT_SIZE, "Lista vanatorilor:\n");
    send_to_pipe(buffer);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0 && strncmp(entry->d_name, ".", 1) != 0) {
            snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", entry->d_name);
            int fd = open(filepath, O_RDONLY);
            int count = 0;
            if (fd >= 0) {
                Treasure t;
                while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                    count++;
                }
                close(fd);
                snprintf(buffer, MAX_OUTPUT_SIZE, "Vanaatoare: %s | Numar comori: %d\n", entry->d_name, count);
                send_to_pipe(buffer);
            }
        }
    }
    closedir(dir);
}

void list_treasures(const char *hunt_id) {
    DIR *dir = opendir(".");
    if (!dir) {
        send_to_pipe("Eroare: Nu pot deschide directorul curent\n");
        return;
    }

    struct dirent *entry;
    char filepath[MY_MAX_PATH];
    int found = 0;
    char buffer[MAX_OUTPUT_SIZE];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, hunt_id) == 0) {
            found = 1;
            snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", entry->d_name);

            int fd = open(filepath, O_RDONLY);
            if (fd < 0) {
                snprintf(buffer, MAX_OUTPUT_SIZE, "Eroare: Nu pot deschide fisierul treasure.dat pentru %s\n", hunt_id);
                send_to_pipe(buffer);
                closedir(dir);
                return;
            }

            Treasure t;
            snprintf(buffer, MAX_OUTPUT_SIZE, "Comorile din vanatoarea '%s':\n", hunt_id);
            send_to_pipe(buffer);
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                snprintf(buffer, MAX_OUTPUT_SIZE, "ID: %d | User: %s | Lat: %.2f | Long: %.2f | Valoare: %d\n  Indiciu: %s\n",
                         t.ID, t.username, t.latitudine, t.longitudine, t.value, t.clue);
                send_to_pipe(buffer);
            }
            close(fd);
            closedir(dir);
            return;
        }
    }

    if (!found) {
        snprintf(buffer, MAX_OUTPUT_SIZE, "Nu am gasit niciun director pentru hunt_id: %s\n", hunt_id);
        send_to_pipe(buffer);
    }
    closedir(dir);
}

void view_treasure(const char *hunt_id, int treasure_id) {
    DIR *dir = opendir(".");
    if (!dir) {
        send_to_pipe("Eroare: Nu pot deschide directorul curent\n");
        return;
    }

    struct dirent *entry;
    char filepath[MY_MAX_PATH];
    int found = 0;
    char buffer[MAX_OUTPUT_SIZE];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, hunt_id) == 0) {
            found = 1;
            snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", entry->d_name);

            int fd = open(filepath, O_RDONLY);
            if (fd < 0) {
                send_to_pipe("Eroare: Nu pot deschide fisierul treasure.dat\n");
                closedir(dir);
                return;
            }

            Treasure t;
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                if (t.ID == treasure_id) {
                    snprintf(buffer, MAX_OUTPUT_SIZE, "Detalii comoara ID %d:\nNume utilizator: %s\nLatitudine: %.2f\nLongitudine: %.2f\nIndiciu: %s\nValoare: %d\n",
                             t.ID, t.username, t.latitudine, t.longitudine, t.clue, t.value);
                    send_to_pipe(buffer);
                    close(fd);
                    closedir(dir);
                    return;
                }
            }
            snprintf(buffer, MAX_OUTPUT_SIZE, "Comoara cu ID-ul %d nu a fost gasita in hunt-ul '%s'.\n", treasure_id, hunt_id);
            send_to_pipe(buffer);
            close(fd);
            closedir(dir);
            return;
        }
    }

    if (!found) {
        snprintf(buffer, MAX_OUTPUT_SIZE, "Nu am gasit directorul hunt_id: %s\n", hunt_id);
        send_to_pipe(buffer);
    }
    closedir(dir);
}

void monitor_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGUSR1) {
        list_hunts();
    } else if (sig == SIGUSR2) {
        char hunt_id[MY_MAX_PATH] = {0};
        int fd = open(HUNT_ID_FILE, O_RDONLY);
        if (fd >= 0) {
            ssize_t bytes_read = read(fd, hunt_id, MY_MAX_PATH - 1);
            if (bytes_read > 0) {
                hunt_id[bytes_read] = '\0';
                list_treasures(hunt_id);
            } else {
                send_to_pipe("Eroare: Niciun hunt_id citit din fișier\n");
            }
            close(fd);
        } else {
            send_to_pipe("Eroare: Nu pot deschide fisierul hunt_id.tmp\n");
        }
    } else if (sig == SIGRTMIN) {
        char hunt_id[MY_MAX_PATH] = {0};
        int treasure_id = 0;
        int fd = open(VIEW_TREASURE_FILE, O_RDONLY);
        if (fd >= 0) {
            char buffer[MY_MAX_PATH + 16];
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                if (sscanf(buffer, "%s %d", hunt_id, &treasure_id) == 2) {
                    view_treasure(hunt_id, treasure_id);
                } else {
                    send_to_pipe("Eroare: Format invalid în fișierul view_treasure.tmp\n");
                }
            } else {
                send_to_pipe("Eroare: Niciun conținut citit din fișierul view_treasure.tmp\n");
            }
            close(fd);
        } else {
            send_to_pipe("Eroare: Nu pot deschide fisierul view_treasure.tmp\n");
        }
    } else if (sig == SIGTERM) {
        usleep(MONITOR_STOP_DELAY);
        close(pipe_fd[1]);
        exit(0);
    }
}

void setup_signals() {
    struct sigaction sa;
    sa.sa_sigaction = monitor_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("Eroare la configurarea SIGUSR1");
        exit(1);
    }
    if (sigaction(SIGUSR2, &sa, NULL) < 0) {
        perror("Eroare la configurarea SIGUSR2");
        exit(1);
    }
    if (sigaction(SIGRTMIN, &sa, NULL) < 0) {
        perror("Eroare la configurarea SIGRTMIN");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("Eroare la configurarea SIGTERM");
        exit(1);
    }
}

void monitor_process() {
    close(pipe_fd[0]); // Close read end in monitor
    setup_signals();
    while (1) {
        pause();
    }
}

void start_monitor() {
    if (monitor_pid > 0) {
        printf("Monitorul rulează deja cu PID %d\n", monitor_pid);
        return;
    }

    if (pipe(pipe_fd) == -1) {
        perror("Eroare la crearea pipe-ului");
        return;
    }

    // Set pipe read end to non-blocking
    int flags = fcntl(pipe_fd[0], F_GETFL);
    fcntl(pipe_fd[0], F_SETFL, flags | O_NONBLOCK);

    monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("Eroare la crearea procesului");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    } else if (monitor_pid == 0) {
        monitor_process();
        exit(0);
    } else {
        close(pipe_fd[1]); // Close write end in main process
        printf("Monitor pornit cu PID %d\n", monitor_pid);
    }
}

void read_pipe_output() {
    char pipe_buffer[MAX_OUTPUT_SIZE];
    ssize_t bytes_read;
    int retries = 5;
    while (retries-- > 0) {
        while ((bytes_read = read(pipe_fd[0], pipe_buffer, MAX_OUTPUT_SIZE - 1)) > 0) {
            pipe_buffer[bytes_read] = '\0';
            printf("%s", pipe_buffer);
        }
        if (bytes_read < 0 && errno != EAGAIN) {
            perror("Eroare la citirea din pipe");
            break;
        }
        if (bytes_read > 0) break; // Exit if we got data
        usleep(100000); // Wait 100ms before retrying
    }
}

int is_monitor_running() {
    if (monitor_pid <= 0) {
        return 0;
    }
    if (kill(monitor_pid, 0) == -1 && errno == ESRCH) {
        monitor_pid = 0;
        close(pipe_fd[0]);
        return 0;
    }
    return 1;
}

void send_list_hunts_signal() {
    if (!is_monitor_running()) {
        printf("Niciun monitor nu rulează\n");
        return;
    }
    if (kill(monitor_pid, SIGUSR1) == 0) {
        printf("Semnal SIGUSR1 trimis către monitor\n");
        read_pipe_output();
    } else {
        perror("Eroare la trimiterea semnalului");
        monitor_pid = 0;
        close(pipe_fd[0]);
    }
}

void send_list_treasures_signal(const char *hunt_id) {
    if (!is_monitor_running()) {
        printf("Niciun monitor nu rulează\n");
        return;
    }

    int fd = open(HUNT_ID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Eroare la crearea fișierului hunt_id.tmp");
        return;
    }
    if (write(fd, hunt_id, strlen(hunt_id)) < 0) {
        perror("Eroare la scrierea hunt_id în fișier");
        close(fd);
        return;
    }
    close(fd);

    if (kill(monitor_pid, SIGUSR2) == 0) {
        printf("Semnal SIGUSR2 trimis către monitor pentru hunt_id: %s\n", hunt_id);
        read_pipe_output();
    } else {
        perror("Eroare la trimiterea semnalului");
        monitor_pid = 0;
        close(pipe_fd[0]);
    }
}

void send_view_treasure_signal(const char *hunt_id, int treasure_id) {
    if (!is_monitor_running()) {
        printf("Niciun monitor nu rulează\n");
        return;
    }

    int fd = open(VIEW_TREASURE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Eroare la crearea fișierului view_treasure.tmp");
        return;
    }
    char buffer[MY_MAX_PATH + 16];
    snprintf(buffer, sizeof(buffer), "%s %d", hunt_id, treasure_id);
    if (write(fd, buffer, strlen(buffer)) < 0) {
        perror("Eroare la scrierea datelor în fișier");
        close(fd);
        return;
    }
    close(fd);

    if (kill(monitor_pid, SIGRTMIN) == 0) {
        printf("Semnal SIGRTMIN trimis către monitor pentru hunt_id: %s, treasure_id: %d\n", hunt_id, treasure_id);
        read_pipe_output();
    } else {
        perror("Eroare la trimiterea semnalului");
        monitor_pid = 0;
        close(pipe_fd[0]);
    }
}

void stop_monitor() {
    if (!is_monitor_running()) {
        printf("Niciun monitor nu rulează\n");
        return;
    }

    if (kill(monitor_pid, SIGTERM) == 0) {
        printf("'a Monitor. Aștept terminarea...\n");
        monitor_stopping = 1;

        int status;
        pid_t result = waitpid(monitor_pid, &status, 0);
        if (result == monitor_pid) {
            if (WIFEXITED(status)) {
                printf("Monitorul s-a terminat cu codul de ieșire %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Monitorul a fost terminat de semnalul %d\n", WTERMSIG(status));
            }
            unlink(HUNT_ID_FILE);
            unlink(VIEW_TREASURE_FILE);
            close(pipe_fd[0]);
            monitor_pid = 0;
            monitor_stopping = 0;
        } else {
            perror("Eroare la așteptarea terminării monitorului");
        }
    } else {
        perror("Eroare la trimiterea semnalului SIGTERM");
        monitor_pid = 0;
        monitor_stopping = 0;
    }
}

void calculate_score() {
    DIR *dir = opendir(".");
    if (!dir) {
        printf("Eroare: Nu pot deschide directorul curent\n");
        return;
    }

    struct stat st;
    if (stat("./calculate_score", &st) != 0 || !(st.st_mode & S_IXUSR)) {
        printf("Eroare: Programul calculate_score nu există sau nu este executabil\n");
        closedir(dir);
        return;
    }

    struct dirent *entry;
    char filepath[MY_MAX_PATH];

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strncmp(entry->d_name, ".", 1) != 0) {
            snprintf(filepath, MY_MAX_PATH, "%s/treasure.dat", entry->d_name);
            int fd = open(filepath, O_RDONLY);
            if (fd >= 0) {
                close(fd);
                int score_pipe[2];
                if (pipe(score_pipe) == -1) {
                    perror("Eroare la crearea pipe-ului pentru scor");
                    continue;
                }

                pid_t pid = fork();
                if (pid < 0) {
                    perror("Eroare la crearea procesului pentru scor");
                    close(score_pipe[0]);
                    close(score_pipe[1]);
                    continue;
                } else if (pid == 0) {
                    close(score_pipe[0]); // Close read end
                    dup2(score_pipe[1], STDOUT_FILENO); // Redirect stdout to pipe
                    close(score_pipe[1]);
                    execl("./calculate_score", "calculate_score", entry->d_name, NULL);
                    perror("Eroare la executarea calculate_score");
                    exit(1);
                } else {
                    close(score_pipe[1]); // Close write end
                    char buffer[MAX_OUTPUT_SIZE];
                    ssize_t bytes_read;
                    printf("Scoruri pentru vanatoarea '%s':\n", entry->d_name);
                    int flags = fcntl(score_pipe[0], F_GETFL);
                    fcntl(score_pipe[0], F_SETFL, flags | O_NONBLOCK);
                    int empty = 1;
                    int retries = 5;
                    while (retries-- > 0) {
                        while ((bytes_read = read(score_pipe[0], buffer, MAX_OUTPUT_SIZE - 1)) > 0) {
                            buffer[bytes_read] = '\0';
                            printf("%s", buffer);
                            empty = 0;
                        }
                        if (bytes_read > 0) break;
                        usleep(100000); // Wait 100ms
                    }
                    if (empty) {
                        printf("Niciun scor disponibil\n");
                    }
                    close(score_pipe[0]);
                    waitpid(pid, NULL, 0);
                }
            }
        }
    }
    closedir(dir);
}

int main() {
    char command[128], hunt_id[MY_MAX_PATH];
    int treasure_id;

    printf("Bine ați venit la Treasure Hub!\n");
    printf("Comenzi disponibile: start_monitor, list_hunts, list_treasures <hunt_id>, view_treasure <hunt_id> <treasure_id>, calculate_score, stop_monitor, exit\n");

    while (1) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        command[strcspn(command, "\n")] = 0;

        if (monitor_stopping) {
            if (strcmp(command, "exit") != 0) {
                printf("Eroare: Monitorul este în proces de oprire. Așteptați terminarea sau folosiți exit.\n");
                continue;
            }
        }

        if (strcmp(command, "start_monitor") == 0) {
            if (is_monitor_running()) {
                printf("Monitorul rulează deja cu PID %d\n", monitor_pid);
            } else {
                start_monitor();
            }
        } else if (strcmp(command, "list_hunts") == 0) {
            send_list_hunts_signal();
        } else if (sscanf(command, "list_treasures %s", hunt_id) == 1) {
            send_list_treasures_signal(hunt_id);
        } else if (sscanf(command, "view_treasure %s %d", hunt_id, &treasure_id) == 2) {
            send_view_treasure_signal(hunt_id, treasure_id);
        } else if (strcmp(command, "calculate_score") == 0) {
            calculate_score();
        } else if (strcmp(command, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(command, "exit") == 0) {
            if (is_monitor_running()) {
                printf("Eroare: Monitorul încă rulează. Opriți-l mai întâi cu stop_monitor.\n");
            } else {
                break;
            }
        } else {
            printf("Comandă necunoscută: %s\n", command);
        }
    }

    printf("Treasure Hub se închide\n");
    unlink(HUNT_ID_FILE);
    unlink(VIEW_TREASURE_FILE);
    return 0;
}