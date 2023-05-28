#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILE_PATH_LENGTH 256

void keepInit();
void keepTrack(const char* path);
void keepUntrack(const char* path);
void keepVersions();
void keepStore(const char* note);
void keepRestore(int version);

int readLatestVersion();
int checkModifiedFiles(int latestVersion);
int copyTrackingFiles(const char* versionDir);
int updateLatestVersion(int latestVersion);
int storeNoteForVersion(const char* versionDir, const char* note);
int copyFilesToTarget(const char* versionDir, const char* targetDir);
int removeNonTrackingFiles();
int copyFileToTarget(const char* source, const char* target);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Error: No command specified.\n");
        return 1;
    }
    if (strcmp(argv[1], "keep") == 0) {
        if (strcmp(argv[2], "init") == 0) {
            keepInit();
        } else if (strcmp(argv[2], "track") == 0) {
            if (argc < 4) {
                printf("Error: No file or directory specified.\n");
                return 1;
            }
            keepTrack(argv[3]);
        } else if (strcmp(argv[2], "untrack") == 0) {
            if (argc < 4) {
                printf("Error: No file or directory specified.\n");
                return 1;
            }
            keepUntrack(argv[3]);
        } else if (strcmp(argv[2], "versions") == 0) {
            keepVersions();
        } else if (strcmp(argv[2], "store") == 0) {
            if (argc < 4) {
                printf("Error: No note specified.\n");
                return 1;
            }
            keepStore(argv[3]);
        } else if (strcmp(argv[2], "restore") == 0) {
            if (argc < 4) {
                printf("Error: No version specified.\n");
                return 1;
            }
            int version = atoi(argv[3]);
            keepRestore(version);
        } else {
            printf("Error: Invalid command.\n");
            return 1;
        }
    } else {
        printf("Error: Invalid command.\n");
        return 1;
    }

    return 0;
}

void keepInit() {
    DIR* keepDir = opendir(".keep");
    if (keepDir != NULL) {
        printf("Error: .keep directory already exists.\n");
        closedir(keepDir);
        return;
    }

    if (mkdir(".keep", 0700) != 0) {
        printf("Error: Failed to create .keep directory.\n");
        return;
    }

    FILE* trackingFiles = fopen(".keep/tracking-files", "w");
    if (trackingFiles == NULL) {
        printf("Error: Failed to create tracking-files file.\n");
        return;
    }
    fclose(trackingFiles);

    FILE* latestVersionFile = fopen(".keep/latest-version", "w");
    if (latestVersionFile == NULL) {
        printf("Error: Failed to create latest-version file.\n");
        return;
    }
    fprintf(latestVersionFile, "0");
    fclose(latestVersionFile);

    printf("Initialized .keep directory.\n");
}

void keepTrack(const char* path) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("Error: Failed to open '%s' directory.\n", path);
        return;
    }

    FILE* trackingFiles = fopen(".keep/tracking-files", "a");
    if (trackingFiles == NULL) {
        printf("Error: Failed to open tracking-files file.\n");
        closedir(dir);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filePath[MAX_FILE_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name);

        struct stat fileStat;
        if (stat(filePath, &fileStat) == 0) {
            fprintf(trackingFiles, "%s %ld\n", filePath, fileStat.st_mtime);
        } else {
            printf("Error: Failed to get information for file '%s'.\n", filePath);
        }
    }

    fclose(trackingFiles);
    closedir(dir);
    printf("Tracking files in '%s'.\n", path);
}

void keepUntrack(const char* path) {
    FILE* trackingFiles = fopen(".keep/tracking-files", "r");
    if (trackingFiles == NULL) {
        printf("Error: Failed to open tracking-files file.\n");
        return;
    }

    FILE* tempFile = fopen(".keep/tracking-files.temp", "w");
    if (tempFile == NULL) {
        printf("Error: Failed to create temporary file.\n");
        fclose(trackingFiles);
        return;
    }

    char filePath[MAX_FILE_PATH_LENGTH];
    int found = 0;

    while (fgets(filePath, sizeof(filePath), trackingFiles) != NULL) {
        filePath[strcspn(filePath, "\n")] = '\0'; // Remove the trailing newline character

        if (strcmp(filePath, path) == 0 || strncmp(filePath, path, strlen(path) + 1) == 0) {
            found = 1;
        } else {
            fprintf(tempFile, "%s\n", filePath);
        }
    }

    fclose(trackingFiles);
    fclose(tempFile);

    if (found) {
        remove(".keep/tracking-files");
        rename(".keep/tracking-files.temp", ".keep/tracking-files");
        printf("Untracked '%s'.\n", path);
    } else {
        remove(".keep/tracking-files.temp");
        printf("Error: '%s' is not tracked.\n", path);
    }
}

void keepVersions() {
    FILE* latestVersionFile = fopen(".keep/latest-version", "r");
    if (latestVersionFile == NULL) {
        printf("Error: Failed to open latest-version file.\n");
        return;
    }

    int latestVersion;
    fscanf(latestVersionFile, "%d", &latestVersion);
    fclose(latestVersionFile);

    printf("Latest Version: %d\n", latestVersion);

    for (int version = 1; version <= latestVersion; version++) {
        char versionDir[MAX_FILE_PATH_LENGTH];
        snprintf(versionDir, sizeof(versionDir), ".keep/%d", version);

        FILE* noteFile = fopen(".keep/note", "r");
        if (noteFile == NULL) {
            printf("Error: Failed to open note file for version %d.\n", version);
            continue;
        }

        char note[256];
        fgets(note, sizeof(note), noteFile);
        fclose(noteFile);

        printf("Version %d: %s", version, note);
    }
}

void keepStore(const char* note) {
    int latestVersion = readLatestVersion();
    int modifiedFiles = checkModifiedFiles(latestVersion);

    if (modifiedFiles == 0) {
        printf("Nothing to update.\n");
        return;
    }

    char versionDir[MAX_FILE_PATH_LENGTH];
    snprintf(versionDir, sizeof(versionDir), ".keep/%d", latestVersion + 1);

    if (mkdir(versionDir, 0700) != 0) {
        printf("Error: Failed to create version directory.\n");
        return;
    }

    char targetDir[MAX_FILE_PATH_LENGTH];
    snprintf(targetDir, sizeof(targetDir), "%s/target", versionDir);

    if (mkdir(targetDir, 0700) != 0) {
        printf("Error: Failed to create target directory.\n");
        return;
    }

    if (copyTrackingFiles(versionDir) != 0) {
        printf("Error: Failed to copy tracking files.\n");
        return;
    }

    if (copyFilesToTarget(versionDir, targetDir) != 0) {
        printf("Error: Failed to copy files to target directory.\n");
        return;
    }

    if (updateLatestVersion(latestVersion) != 0) {
        printf("Error: Failed to update latest version.\n");
        return;
    }

    if (storeNoteForVersion(versionDir, note) != 0) {
        printf("Error: Failed to store note for the version.\n");
        return;
    }

    printf("Stored version %d.\n", latestVersion + 1);
}

void keepRestore(int version) {
    int latestVersion = readLatestVersion();
    int modifiedFiles = checkModifiedFiles(latestVersion);

    if (modifiedFiles > 0) {
        printf("Error: There are modified files. Please store the changes before restoring.\n");
        return;
    }

    if (version <= 0 || version > latestVersion) {
        printf("Error: Invalid version number.\n");
        return;
    }

    char versionDir[MAX_FILE_PATH_LENGTH];
    snprintf(versionDir, sizeof(versionDir), ".keep/%d", version);

    char targetDir[MAX_FILE_PATH_LENGTH];
    snprintf(targetDir, sizeof(targetDir), "%s/target", versionDir);

    DIR* dir = opendir(targetDir);
    if (dir == NULL) {
        printf("Error: Failed to open target directory for version %d.\n", version);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filePath[MAX_FILE_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s/%s", targetDir, entry->d_name);

        if (remove(filePath) != 0) {
            printf("Error: Failed to remove file '%s'.\n", filePath);
        }
    }

    closedir(dir);

    if (copyFilesToTarget(versionDir, targetDir) != 0) {
        printf("Error: Failed to restore files to target directory.\n");
        return;
    }

    if (removeNonTrackingFiles() != 0) {
        printf("Error: Failed to remove non-tracking files.\n");
        return;
    }

    printf("Restored version %d.\n", version);
}

int readLatestVersion() {
    FILE* latestVersionFile = fopen(".keep/latest-version", "r");
    if (latestVersionFile == NULL) {
        printf("Error: Failed to open latest-version file.\n");
        return -1;
    }

    int latestVersion;
    fscanf(latestVersionFile, "%d", &latestVersion);
    fclose(latestVersionFile);

    return latestVersion;
}

int checkModifiedFiles(int latestVersion) {
    FILE* trackingFiles = fopen(".keep/tracking-files", "r");
    if (trackingFiles == NULL) {
        printf("Error: Failed to open tracking-files file.\n");
        return -1;
    }

    char filePath[MAX_FILE_PATH_LENGTH];
    int modifiedFiles = 0;

    while (fgets(filePath, sizeof(filePath), trackingFiles) != NULL) {
        filePath[strcspn(filePath, "\n")] = '\0'; // Remove the trailing newline character

        char* filePtr = strtok(filePath, " ");
        char* timePtr = strtok(NULL, " ");

        if (filePtr == NULL || timePtr == NULL) {
            continue;
        }

        struct stat fileStat;
        if (stat(filePtr, &fileStat) == 0) {
            if (fileStat.st_mtime > atoi(timePtr)) {
                modifiedFiles++;
            }
        }
    }

    fclose(trackingFiles);
    return modifiedFiles;
}

int copyTrackingFiles(const char* versionDir) {
    FILE* trackingFiles = fopen(".keep/tracking-files", "r");
    if (trackingFiles == NULL) {
        printf("Error: Failed to open tracking-files file.\n");
        return -1;
    }

    char filePath[MAX_FILE_PATH_LENGTH];
    FILE* targetFile = fopen(".keep/target/tracking-files", "w");
    if (targetFile == NULL) {
        printf("Error: Failed to create target tracking-files file.\n");
        fclose(trackingFiles);
        return -1;
    }

    while (fgets(filePath, sizeof(filePath), trackingFiles) != NULL) {
        filePath[strcspn(filePath, "\n")] = '\0'; // Remove the trailing newline character
        fprintf(targetFile, "%s\n", filePath);
    }

    fclose(trackingFiles);
    fclose(targetFile);
    return 0;
}

int updateLatestVersion(int latestVersion) {
    FILE* latestVersionFile = fopen(".keep/latest-version", "w");
    if (latestVersionFile == NULL) {
        printf("Error: Failed to open latest-version file.\n");
        return -1;
    }

    fprintf(latestVersionFile, "%d", latestVersion + 1);
    fclose(latestVersionFile);
    return 0;
}

int storeNoteForVersion(const char* versionDir, const char* note) {
    FILE* noteFile = fopen(".keep/note", "w");
    if (noteFile == NULL) {
        printf("Error: Failed to open note file.\n");
        return -1;
    }

    fprintf(noteFile, "%s\n", note);
    fclose(noteFile);

    char targetNoteFile[MAX_FILE_PATH_LENGTH];
    snprintf(targetNoteFile, sizeof(targetNoteFile), "%s/target/note", versionDir);

    if (copyFileToTarget(".keep/note", targetNoteFile) != 0) {
        printf("Error: Failed to copy note file to target directory.\n");
        return -1;
    }

    return 0;
}

int copyFilesToTarget(const char* versionDir, const char* targetDir) {
    FILE* trackingFiles = fopen(".keep/tracking-files", "r");
    if (trackingFiles == NULL) {
        printf("Error: Failed to open tracking-files file.\n");
        return -1;
    }

    char filePath[MAX_FILE_PATH_LENGTH];
    while (fgets(filePath, sizeof(filePath), trackingFiles) != NULL) {
        filePath[strcspn(filePath, "\n")] = '\0'; // Remove the trailing newline character
        char targetFile[MAX_FILE_PATH_LENGTH];
        snprintf(targetFile, sizeof(targetFile), "%s/target/%s", versionDir, filePath);
        if (copyFileToTarget(filePath, targetFile) != 0) {
            fclose(trackingFiles);
            return -1;
        }
    }

    fclose(trackingFiles);
    return 0;
}

int removeNonTrackingFiles() {
    DIR* dir = opendir(".");
    if (dir == NULL) {
        printf("Error: Failed to open current directory.\n");
        return -1;
    }

    FILE* trackingFiles = fopen(".keep/tracking-files", "r");
    if (trackingFiles == NULL) {
        printf("Error: Failed to open tracking-files file.\n");
        closedir(dir);
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filePath[MAX_FILE_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s", entry->d_name);

        int found = 0;
        char trackedFilePath[MAX_FILE_PATH_LENGTH];
        rewind(trackingFiles);
        while (fgets(trackedFilePath, sizeof(trackedFilePath), trackingFiles) != NULL) {
            trackedFilePath[strcspn(trackedFilePath, "\n")] = '\0'; // Remove the trailing newline character

            if (strcmp(filePath, trackedFilePath) == 0) {
                found = 1;
                break;
            }
        }

        if (!found) {
            if (remove(filePath) != 0) {
                printf("Error: Failed to remove file '%s'.\n", filePath);
            }
        }
    }

    fclose(trackingFiles);
    closedir(dir);
    return 0;
}

int copyFileToTarget(const char* source, const char* target) {
    FILE* sourceFile = fopen(source, "r");
    if (sourceFile == NULL) {
        printf("Error: Failed to open file '%s'.\n", source);
        return -1;
    }

    FILE* targetFile = fopen(target, "w");
    if (targetFile == NULL) {
        printf("Error: Failed to create target file '%s'.\n", target);
        fclose(sourceFile);
        return -1;
    }

    char ch;
    while ((ch = fgetc(sourceFile)) != EOF) {
        fputc(ch, targetFile);
    }

    fclose(sourceFile);
    fclose(targetFile);
    return 0;
}
