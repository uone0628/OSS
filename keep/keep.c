#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_FILE_PATH_LENGTH 100
#define MAX_NOTE_LENGTH 100

void keepInit();
void keepTrack(char* target);
void keepUntrack(char* target);
void keepVersions();
void keepStore(char* note);
void keepRestore(int version);

void init();
void track(char input[]);
void untrack(char input[]);
void versions();
void store(char input[]);
void restore();

typedef struct {
    char command[10];
    char arg[MAX_COMMAND_LENGTH];
} ParsedCommand;

ParsedCommand parseCommand(char* input) {
    ParsedCommand cmd;
    char* token = strtok(input, " ");
    strcpy(cmd.command, token);
    token = strtok(NULL, "\n");
    if (token != NULL) {
        strcpy(cmd.arg, token);
    } else {
        strcpy(cmd.arg, "");
    }
    return cmd;
}

int main() {
    char input[MAX_COMMAND_LENGTH];

    while (1) {
        printf("Enter a command: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove the trailing newline character

        ParsedCommand cmd = parseCommand(input);

        if (strcmp(cmd.command, "keep") == 0) {
            if (strcmp(cmd.arg, "init") == 0) {
                keepInit();
            } else if (strcmp(cmd.arg, "track") == 0) {
                track(input);
            } else if (strcmp(cmd.arg, "untrack") == 0) {
                untrack(input);
            } else if (strcmp(cmd.arg, "versions") == 0) {
                keepVersions();
            } else if (strcmp(cmd.arg, "store") == 0) {
                store(input);
            } else if (strcmp(cmd.arg, "restore") == 0) {
                restore();
            } else {
                printf("Invalid command. Please try again.\n");
            }
        } else if (strcmp(cmd.command, "quit") == 0) {
            printf("Exiting the program.\n");
            break;
        } else {
            printf("Invalid command. Please try again.\n");
        }
    }

    return 0;
}

void track(char input[]) {
    // Get target from user
    printf("Enter the target file or directory: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0'; // Remove the trailing newline character
    keepTrack(input);
}

void untrack(char input[]) {
    // Get target from user
    printf("Enter the target file or directory: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0'; // Remove the trailing newline character
    keepUntrack(input);
}

void store(char input[]) {
    // Get note from user
    printf("Enter the note: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0'; // Remove the trailing newline character
    keepStore(input);
}

void restore() {
    // Get version from user
    int version;
    printf("Enter the version number: ");
    scanf("%d", &version);
    getchar(); // Remove the newline character from the input buffer
    keepRestore(version);
}

void keepInit() {
    // Implementation for 'keep init' command
    struct stat st;
    if (stat(".keep", &st) == 0) {
        printf("Error: .keep directory already exists.\n");
    } else {
        int result = mkdir(".keep", 0700);
        if (result == 0) {
            FILE* trackingFiles = fopen(".keep/tracking-files", "w");
            FILE* latestVersion = fopen(".keep/latest-version", "w");
            if (trackingFiles != NULL && latestVersion != NULL) {
                fclose(trackingFiles);
                fclose(latestVersion);
                printf("Successfully initialized the backup space.\n");
            } else {
                printf("Error: Failed to create tracking files or latest version file.\n");
            }
        } else {
            printf("Error: Failed to create .keep directory.\n");
        }
    }
}

void keepTrack(char* target) {
    // Implementation for 'keep track <file or directory>' command
    FILE* trackingFiles = fopen(".keep/tracking-files", "a");
    if (trackingFiles != NULL) {
        fprintf(trackingFiles, "%s\n", target);
        fclose(trackingFiles);
        printf("Successfully added '%s' to tracking files.\n", target);
    } else {
        printf("Error: Failed to open tracking files.\n");
    }
}

void keepUntrack(char* target) {
    // Implementation for 'keep untrack <file or directory>' command
    FILE* trackingFiles = fopen(".keep/tracking-files", "r");
    if (trackingFiles != NULL) {
        FILE* tempFile = fopen(".keep/temp", "w");
        if (tempFile != NULL) {
            char line[MAX_FILE_PATH_LENGTH];
            int found = 0;
            while (fgets(line, sizeof(line), trackingFiles) != NULL) {
                line[strcspn(line, "\n")] = '\0'; // Remove the trailing newline character
                if (strcmp(line, target) != 0) {
                    fprintf(tempFile, "%s\n", line);
                } else {
                    found = 1;
                }
            }
            fclose(trackingFiles);
            fclose(tempFile);

            if (found) {
                remove(".keep/tracking-files");
                rename(".keep/temp", ".keep/tracking-files");
                printf("Successfully removed '%s' from tracking files.\n", target);
            } else {
                remove(".keep/temp");
                printf("Error: '%s' is not found in tracking files.\n", target);
            }
        } else {
            printf("Error: Failed to create temporary file.\n");
        }
    } else {
        printf("Error: Failed to open tracking files.\n");
    }
}

void keepVersions() {
    // Implementation for 'keep versions' command
    FILE* latestVersion = fopen(".keep/latest-version", "r");
    if (latestVersion != NULL) {
        int latest;
        fscanf(latestVersion, "%d", &latest);
        fclose(latestVersion);

        if (latest == 0) {
            printf("No versions found.\n");
        } else {
            printf("Latest version: %d\n", latest);

            char versionDir[MAX_FILE_PATH_LENGTH];
            char note[MAX_NOTE_LENGTH];
            for (int i = 1; i <= latest; i++) {
                snprintf(versionDir, sizeof(versionDir), ".keep/%d", i);
                snprintf(note, sizeof(note), "%s/%d/note", ".keep", i);

                FILE* noteFile = fopen(note, "r");
                if (noteFile != NULL) {
                    char noteText[MAX_NOTE_LENGTH];
                    fgets(noteText, sizeof(noteText), noteFile);
                    fclose(noteFile);

                    printf("Version %d - Note: %s\n", i, noteText);
                } else {
                    printf("Error: Failed to open note file for version %d.\n", i);
                }
            }
        }
    } else {
        printf("Error: Failed to open latest version file.\n");
    }
}

void keepStore(char* note) {
    // Implementation for 'keep store "note"' command
    int latestVersion = readLatestVersion();
    if (latestVersion == -1) {
        printf("Error: Failed to read the latest version number.\n");
        return;
    }

    // Check if any tracking file has been modified
    int modified = checkModifiedFiles(latestVersion);
    if (!modified) {
        printf("Nothing to update.\n");
        return;
    }

    // Create a new version directory
    latestVersion++;
    char versionDir[MAX_FILE_PATH_LENGTH];
    snprintf(versionDir, sizeof(versionDir), ".keep/%d", latestVersion);

    if (mkdir(versionDir, 0700) != 0) {
        printf("Error: Failed to create version directory.\n");
        return;
    }

    // Copy tracking files to the target directory
    if (!copyTrackingFiles(versionDir)) {
        printf("Error: Failed to copy tracking files to the target directory.\n");
        return;
    }

    // Update the latest version number
    if (!updateLatestVersion(latestVersion)) {
        printf("Error: Failed to update the latest version number.\n");
        return;
    }

    // Store the note for the version
    if (!storeNoteForVersion(versionDir, note)) {
        printf("Error: Failed to store the note for the version.\n");
        return;
    }

    printf("Successfully stored the current status as version %d.\n", latestVersion);
}

void keepRestore(int version) {
    // Implementation for 'keep restore <version>' command
    int modified = checkModifiedFiles(version);
    if (modified) {
        printf("Error: Some tracking files have been modified after the latest store or restore.\n");
        return;
    }

    // Restore the specified version
    char versionDir[MAX_FILE_PATH_LENGTH];
    snprintf(versionDir, sizeof(versionDir), ".keep/%d", version);

    char targetDir[MAX_FILE_PATH_LENGTH];
    snprintf(targetDir, sizeof(targetDir), "%s/target", versionDir);

    // Copy files from the version directory to the target directory
    if (!copyFilesToTarget(versionDir, targetDir)) {
        printf("Error: Failed to copy files from the version directory to the target directory.\n");
        return;
    }

    // Remove non-tracking files
    if (!removeNonTrackingFiles()) {
        printf("Error: Failed to remove non-tracking files.\n");
        return;
    }

    printf("Successfully restored version %d.\n", version);
}
