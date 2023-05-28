#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_PATH_LENGTH 1024
#define MAX_FILE_PATH_LENGTH 256
#define MAX_COMMAND_LENGTH 100
#define MAX_TRACKED_FILES 100

int numTrackedFiles = 0;
char trackedFiles[MAX_TRACKED_FILES][MAX_PATH_LENGTH];

/*----------------------------------------------------------------------------------*/
void keepInit();

int updateLatestVersion(int latestVersion);
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
void keepTrack(const char* path);

void updateTrackingFiles(const char* path);
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
void keepUntrack(const char* path);
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
void keepVersions();
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
void keepStore(const char* note);

void copyTrackingFiles(const char* targetDir, int version);

void copyFilesToTarget(const char* sourceDir, const char* targetDir, int version);

int readLatestVersion();

int checkModifiedFiles();

int storeNoteForVersion(const char* versionDir, const char* note);

int isLatestVersionFile(const char* filePath, int version);

int copyFileToTarget(const char* source, const char* target);

int copyFile(const char* source, const char* destination);
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
void keepRestore(const char* version);

int isModifiedAfterLatestStoreOrRestore();

int directoryExists(const char* directory);

int removeNonTrackingFiles();

time_t getFileModifiedTime(const char* filePath);
/*----------------------------------------------------------------------------------*/





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
    // fprintf(trackingFiles, "0");
    fclose(trackingFiles);

    FILE* latestVersionFile = fopen(".keep/latest-version", "w");
    if (latestVersionFile == NULL) {
        printf("Error: Failed to create latest-version file.\n");
        return;
    }
    fprintf(latestVersionFile, "0");
    fclose(latestVersionFile);

    updateLatestVersion(-1);
    printf("Initialized .keep directory.\n");
}



void updateTrackingFiles(const char* path) {
    // const char* filePath = "./keep/tracking-file.txt";

    FILE* trackingFile = fopen(".keep/tracking-files", "a");
    if (trackingFile == NULL) {
        printf("Error: Failed to open tracking file for writing.\n");
        return;
    }
    fprintf(trackingFile, "%s\n", path);
    fclose(trackingFile);
}
//트랙에서 트랙되면 맨 처음에는 0이 떠야 하는데 그 부분을 구현을 못 했습니다.
void keepTrack(const char* path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("Error");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR* dir = opendir(path);
        if (dir == NULL) {
            perror("Error");
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char subpath[MAX_PATH_LENGTH];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);

            keepTrack(subpath);
        }

        closedir(dir);
    }
    else if (S_ISREG(st.st_mode)) {
        if (numTrackedFiles < MAX_TRACKED_FILES) {
            strncpy(trackedFiles[numTrackedFiles], path, sizeof(trackedFiles[numTrackedFiles]));
            numTrackedFiles++;
            printf("Tracked file: %s\n", path);
            updateTrackingFiles(path);
        } else {
            printf("Error: Maximum number of tracked files reached.\n");
        }
    }
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


//여기서 변경사항이 있으면 체크를 해야 하는데 그게 안 됩니다...
int checkModifiedFiles() {
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

int copyFile(const char* source, const char* destination) {
    FILE* src = fopen(source, "rb");
    FILE* dest = fopen(destination, "wb");
    
    if (src == NULL || dest == NULL) {
        return -1;  // Error opening files
    }
    
    int ch;
    while ((ch = fgetc(src)) != EOF) {
        fputc(ch, dest);
    }
    
    fclose(src);
    fclose(dest);
    
    return 0;  // File copy successful
}

int isLatestVersionFile(const char* filePath, int version) {
    char latestVersionDir[MAX_PATH_LENGTH];
    sprintf(latestVersionDir, ".keep/%d", readLatestVersion());
    char latestFilePath[MAX_PATH_LENGTH];
    sprintf(latestFilePath, "%s/%s/%s", latestVersionDir, "target", filePath);

    struct stat st1, st2;
    if (stat(filePath, &st1) == -1 || stat(latestFilePath, &st2) == -1) {
        perror("Error");
        return 0;
    }

    return st1.st_mtime > st2.st_mtime;
}

void copyFilesToTarget(const char* sourceDir, const char* targetDir, int version) {
    DIR* dir = opendir(sourceDir);
    if (dir == NULL) {
        perror("Error");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char sourcePath[MAX_PATH_LENGTH];
        char targetPath[MAX_PATH_LENGTH];
        sprintf(sourcePath, "%s/%s", sourceDir, entry->d_name);
        sprintf(targetPath, "%s/%s", targetDir, entry->d_name);

        struct stat st;
        if (stat(sourcePath, &st) == -1) {
            perror("Error");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // 하위 디렉토리 복사
            if (mkdir(targetPath, 0777) == -1) {
                perror("Error");
                continue;
            }

            copyFilesToTarget(sourcePath, targetPath, version);
        } else if (S_ISREG(st.st_mode)) {
            // 파일 복사
            if (copyFile(sourcePath, targetPath) != 0) {
                continue;
            }

            // 최신 버전의 파일인 경우, 링크 생성
            if (isLatestVersionFile(sourcePath, version)) {
                char latestVersionDir[MAX_PATH_LENGTH];
                sprintf(latestVersionDir, ".keep/%d", readLatestVersion());
                char latestFilePath[MAX_PATH_LENGTH];
                sprintf(latestFilePath, "%s/%s/%s", latestVersionDir, "target", sourcePath);

                if (link(latestFilePath, targetPath) == -1) {
                    perror("Error");
                    continue;
                }
            }
        }
    }

    closedir(dir);
}

void copyTrackingFiles(const char* targetDir, int version) {
    FILE* trackingFile = fopen(".keep/tracking-files", "r");
    if (trackingFile == NULL) {
        perror("Error");
        return;
    }

    char path[MAX_PATH_LENGTH];
    time_t lastModified;

    while (fscanf(trackingFile, "%s %ld", path, &lastModified) == 2) {
        struct stat st;
        if (stat(path, &st) == -1) {
            perror("Error");
            continue;
        }

        char destPath[MAX_PATH_LENGTH];
        sprintf(destPath, "%s/%s", targetDir, path);

        if (S_ISDIR(st.st_mode)) {
            // 디렉토리인 경우, 해당 디렉토리를 생성하고 내부 파일 복사
            if (mkdir(destPath, 0777) == -1) {
                perror("Error");
                continue;
            }

            copyFilesToTarget(path, destPath, version);
        } else if (S_ISREG(st.st_mode)) {
            // 파일인 경우, 파일 복사
            if (copyFile(path, destPath) != 0) {
                continue;
            }

            // 최신 버전의 파일인 경우, 링크 생성
            if (lastModified == 0) {
                char latestVersionDir[MAX_PATH_LENGTH];
                sprintf(latestVersionDir, ".keep/%d", readLatestVersion());
                char latestFilePath[MAX_PATH_LENGTH];
                sprintf(latestFilePath, "%s/%s/%s", latestVersionDir, "target", path);

                if (link(latestFilePath, destPath) == -1) {
                    perror("Error");
                    continue;
                }
            }
        }
    }

    fclose(trackingFile);
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

void keepStore(const char* note) {
    // tracking-files 확인
    FILE* trackingFile = fopen(".keep/tracking-files", "r");
    if (trackingFile == NULL) {
        perror("Error");
        return;
    }
    fclose(trackingFile);

    FILE* file = fopen("store", "w");
    if (file == NULL) {
        printf("Error: Failed to open note file.\n");
        return;
    }
    fprintf(file, "%s\n", note);
    fclose(file);

    // 변경된 파일이 있는 경우
    if (checkModifiedFiles()) {
        int version = readLatestVersion() + 1;
        char versionDirName[MAX_PATH_LENGTH];
        sprintf(versionDirName, ".keep/%d", version);
        
        if (mkdir(versionDirName, 0777) == -1) {
            perror("Error");
            return;
        }
        
        char targetDirName[MAX_PATH_LENGTH];
        sprintf(targetDirName, "%s/target", versionDirName);
        
        if (mkdir(targetDirName, 0777) == -1) {
            perror("Error");
            return;
        }
        
        copyTrackingFiles(targetDirName, version);
        updateLatestVersion(version);

        char versionDir[10];
        snprintf(versionDir, sizeof(versionDir), "%d", version);
        storeNoteForVersion(versionDir, note);

        printf("Stored as version %d\n", version);
    } else {
        printf("Nothing to update.\n");
    }
}



time_t getFileModifiedTime(const char* filePath) {
    struct stat attr;
    if (stat(filePath, &attr) == 0) {
        return attr.st_mtime;
    } else {
        // 파일 정보를 가져오지 못한 경우에 대한 처리
        // 예를 들어, 파일이 존재하지 않는 경우 등
        return -1;  // 수정 시간을 가져올 수 없음을 나타내는 값
    }
}

int isModifiedAfterLatestStoreOrRestore() {
    const char* trackingFilesPath = "./keep/tracking-files";

    // 추적 파일을 읽어옵니다.
    FILE* trackingFile = fopen(trackingFilesPath, "r");
    if (trackingFile == NULL) {
        printf("Error: Failed to open tracking file.\n");
        return 1;  // 파일 열기 실패 시 수정된 것으로 간주합니다.
    }

    int isModified = 0;  // 수정 여부를 나타내는 플래그 변수

    char line[256];
    while (fgets(line, sizeof(line), trackingFile) != NULL) {
        char* filePath = strtok(line, ",");
        char* lastModifiedTimeStr = strtok(NULL, ",");

        // 파일의 수정 시간과 저장된 수정 시간을 비교하여 수정 여부를 확인합니다.
        if (filePath != NULL && lastModifiedTimeStr != NULL) {
            time_t lastModifiedTime = strtol(lastModifiedTimeStr, NULL, 10);
            time_t currentModifiedTime = getFileModifiedTime(filePath);
            if (currentModifiedTime > lastModifiedTime) {
                isModified = 1;  // 수정된 파일이 있다면 플래그 변수를 설정합니다.
                break;
            }
        }
    }

    fclose(trackingFile);

    return isModified;
}

int directoryExists(const char* directory) {
    DIR* dir = opendir(directory);
    if (dir) {
        closedir(dir);
        return 1;  // Directory exists
    } else {
        return 0;  // Directory does not exist
    }
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

void keepRestore(const char* version) {
    char versionDir[100];
    snprintf(versionDir, sizeof(versionDir), "./keep/version%s", version);

    if (isModifiedAfterLatestStoreOrRestore()) {
        printf("Error: Modified tracking files found. Please update or store the changes first.\n");
        return;
    }

    if (!directoryExists(versionDir)) {
        printf("Error: Version %s does not exist.\n", version);
        return;
    }

    char trackingFilePath[100];
    snprintf(trackingFilePath, sizeof(trackingFilePath), "%s/tracking-files", versionDir);

    FILE* trackingFile = fopen(trackingFilePath, "r");
    if (trackingFile == NULL) {
        printf("Error: Failed to open tracking file.\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), trackingFile) != NULL) {
        char* filePath = strtok(line, ",");
        char* lastModifiedTimeStr = strtok(NULL, ",");

        if (filePath != NULL && lastModifiedTimeStr != NULL) {
            // 파일 복원 작업을 수행하는 로직
            char targetPath[100];
            snprintf(targetPath, sizeof(targetPath), "./keep/target%s", filePath);

            if (copyFile(filePath, targetPath) != 0) {
                printf("Error: Failed to restore file %s\n", filePath);
            }
        }
    }

    fclose(trackingFile);

    removeNonTrackingFiles();
    updateTrackingFiles(trackingFilePath);

    printf("Successfully restored to version %s\n", version);
}





void runKeep(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: Invalid command. Please more command.\n");
        return;
    }

    char *keepCommand = argv[1];
    char *argument = argv[2];

    if (strcmp(keepCommand, "init") == 0) {
        keepInit();
    } else if (strcmp(keepCommand, "track") == 0) {
        keepTrack(argument);
    } else if (strcmp(keepCommand, "untrack") == 0) {
        keepUntrack(argument);
    } else if (strcmp(keepCommand, "versions") == 0) {
        keepVersions();
    } else if (strcmp(keepCommand, "store") == 0) {
        keepStore(argument);
    } else if (strcmp(keepCommand, "restore") == 0) {
        // int version = atoi(argument);
        keepRestore(argument);
    } else {
        printf("Error: Invalid keep command.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 1) {
        printf("Error: No command provided.\n");
        return 0;
    }

    char *command = argv[0];

    if (strcmp(command, "keep") == 0) {
        runKeep(argc, argv);
    } else if (strcmp(command, "exit") != 0) {
        printf("Error: Invalid command.\n");
    }

    return 0;
}


