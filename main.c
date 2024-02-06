#include <stdio.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#define MAX_PATH_LEN 1024
#define print(x) printf("%s",x)

char* path = "C:\\Users\\pepij\\CLionProjects";

typedef struct dirent* Dirent;

typedef struct FileInfo {
    DIR* dirPointer;
    char* filePath;
} FileInfo;


typedef struct Stack {
    FileInfo* fileInfo;
    struct Stack* top;
} Stack;

void push(Stack** stack, DIR* dirPointer, char* filePath) {
    Stack* newNode = (Stack*)malloc(sizeof(Stack));
    FileInfo* newFileInfo = (FileInfo*)malloc(sizeof(FileInfo));

    if (newNode == NULL) {
        exit(EXIT_FAILURE);
    }
    newFileInfo->filePath = filePath;
    newFileInfo->dirPointer = dirPointer;
    newNode->fileInfo = newFileInfo;
    newNode->top = *stack;
    *stack = newNode;
}

FileInfo* pop(Stack** stack){
    if (*stack == NULL) {
        return NULL;
    }
    FileInfo* fileInfo = (*stack)->fileInfo;
    Stack* new_top = (*stack)->top;
    free(*stack);
    *stack = new_top;
    return fileInfo;
}




int main() {
    char currentDirPath[1024]; // Buffer to store the current working directory
    Stack* stack = NULL;

    if (getcwd(currentDirPath, sizeof(currentDirPath)) != NULL) {
        printf("Current working directory: %s\n", currentDirPath);
    } else {
        perror("getcwd() error");
        return 1;
    }


    DIR *dirPointer;           // Pointer to a directory stream
    if ((dirPointer = opendir(currentDirPath)) == NULL) {
        fprintf(stderr, "can't open %s\n", currentDirPath);
        return 1;
    }
    push(&stack, dirPointer, currentDirPath);

    FileInfo* fileInfo;

    Dirent dirEntry;  // Pointer to a directory entry

    while ((fileInfo = pop(&stack)) != NULL && fileInfo->dirPointer != NULL) {
        //print(fileInfo->filePath);
        while((dirEntry = readdir(fileInfo->dirPointer)) != NULL){
            if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0) {
                continue; // Skip the . and .. directories
            }

            char* filename = dirEntry->d_name;
            struct stat fileStat;

            if (stat(filename, &fileStat) == 0) {
                if (S_ISDIR(fileStat.st_mode)) {
                    printf("DIR\t%s\n", filename);
                    char newPath[FILENAME_MAX];

                    printf("\tAdded to queue\n");
                    snprintf(newPath, sizeof(newPath), "%s/%s", fileInfo->filePath, filename);
                    push(&stack, opendir(newPath),newPath);
                } else {
                    printf("FILE\t%s\n", filename);

                }
            } else {
                perror("stat error");
                return 1;
            }
        }
    }


    return 0;
}
