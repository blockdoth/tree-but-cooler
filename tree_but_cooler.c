#include <stdio.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


typedef struct dirent* Dirent;
typedef struct stat Stat;

// === Tree ===
typedef struct DirNode {
    char* fileName;
    char* filePath;
    struct DirNode* nextNode;
    struct DirNode* childNodes;
} DirNode;

DirNode* createNode(char* filePath, char* fileName){
    DirNode* newNode = (DirNode*)malloc(sizeof(DirNode));
    newNode->fileName = strdup(fileName);
    newNode->filePath = strdup(filePath);
    newNode->nextNode = NULL;
    newNode->childNodes = NULL;
    return newNode;
}

void freeNode(DirNode* node) {
    free(node->fileName);
    free(node->filePath);
    free(node);
}

DirNode* addNode(DirNode* rootNode, char* filePath, char* fileName){
    // Add a new node as a child of the rootnode
    DirNode* newNode = createNode(filePath, fileName);
    if(rootNode == NULL){
        rootNode = newNode;
    }
    if(rootNode->childNodes == NULL) {
        rootNode->childNodes = newNode;
    } else {
        DirNode* lastChild = rootNode->childNodes;
        while (lastChild->nextNode != NULL) {
            lastChild = lastChild->nextNode;
        }
        lastChild->nextNode = newNode;
    }
    return newNode;
}

// === Stack ===
typedef struct Stack {
    DirNode* fileNode;
    struct Stack* top;
} Stack;

void push(Stack** stack, DirNode* dirNode) {
    Stack* newStack = (Stack*)malloc(sizeof(Stack));
    newStack->fileNode = dirNode;
    newStack->top = *stack;
    *stack = newStack;
}

DirNode* pop(Stack** stack){
    if (*stack == NULL || (*stack)->fileNode == NULL) {
        return NULL;
    }
    DirNode* fileNode = (*stack)->fileNode;
    Stack* new_top = (*stack)->top;
    free(*stack);
    *stack = new_top;
    return fileNode;
}

//=== Dir Name ===
char* getDirNameFromPath(char* path){
    // Dir name can only be retrieved by chopping up the path string
    char* lastSeparator = strrchr(path, '/');
    if (lastSeparator == NULL) {
        printf("No directory separator found.\n");
        exit(1);
    }
    lastSeparator++; // Remove '/' before dir name
    size_t dirNameLength = lastSeparator - path + 1;
    char* dirName = (char*)malloc( sizeof(char*) * (dirNameLength));
    strncpy(dirName, lastSeparator, dirNameLength);
    return dirName;
}

// === Tree Builder ===
DirNode *buildDirTree(char* filePath, int8_t includeFiles) {
    // Traverses the dir tree dir by dir, using a stack rather then recursion
    char* dirName = getDirNameFromPath(filePath);
    // Add the root node to the tree
    DirNode* rootNode = createNode(filePath, dirName);
    free(dirName);

    // Init stack by pushing the root
    Stack* stack = NULL;
    push(&stack, rootNode);
    while(stack != NULL){
        DirNode* currentNode = pop(&stack);
        char* baseFilePath = currentNode->filePath;

        Dirent dirEntry;
        DIR* dirPointer = opendir(baseFilePath);
        while((dirEntry = readdir(dirPointer)) != NULL){ // Each call of returns the dirEntry of the next file in the dir
            if(strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0) {
                continue; // Skip the . and .. directories (current/parent)
            }
            // Construct new filepath
            char newPath[FILENAME_MAX];
            snprintf(newPath, sizeof(newPath), "%s/%s", baseFilePath, dirEntry->d_name);
            Stat fileStat;
            if (stat(newPath, &fileStat) != 0) {
                perror("stat error");
                exit(1);
            }

            // Add to stack if it is a dir
            if (S_ISDIR(fileStat.st_mode)) {
                DirNode* newNode = addNode(currentNode, newPath, dirEntry->d_name);
                push(&stack,  newNode);
            }else if(includeFiles){ // Only add normal files if includeFiles is true
                addNode(currentNode, newPath, dirEntry->d_name);
            }
        }
        closedir(dirPointer);
    }
    free(stack);
    return rootNode;
}

void freeDirTree(DirNode *rootNode) {
    DirNode* childNode = rootNode->childNodes;
    while(childNode != NULL){
        DirNode* nextNode = childNode->nextNode;
        freeDirTree(childNode);
        childNode = nextNode;
    }
    freeNode(rootNode);
}



char* toString(DirNode* rootNode, size_t depth, char* prefix) {
    // Builds dirname
    char* structure = (char*)malloc(sizeof(char*) * (strlen(rootNode->fileName) + 2));
    strcpy(structure, rootNode->fileName);
    strcat(structure, "\n");

    DirNode* childNode = rootNode->childNodes;
    while(childNode != NULL){
        char* childPrefix = (char*)malloc(sizeof(char*) * (strlen(prefix) + 4)); // Make a copy for concat's
        strcpy(childPrefix, prefix);

        if(childNode->nextNode == NULL){ // Last item in the list
            strcat(structure, prefix);
            strcat(structure, " └─ ");
            strcat(childPrefix, "   ");
        }else{
            strcat(structure, prefix);
            strcat(structure, " ├─ ");
            strcat(childPrefix, " │ ");
        }
        // Build the tree of the child dir
        char* childStructure = toString(childNode, depth + 1, childPrefix);
        // Merges the current tree of the current structure with the tree's of the child structures
        structure = (char*)realloc(structure, sizeof(char*) * (strlen(structure) + strlen(childStructure) + 1));
        strcat(structure, childStructure);

        free(childPrefix);
        free(childStructure);
        childNode = childNode->nextNode;
    }
    return structure;
}


int main(int argc, char *argv[]) {
    // Check if there are at least two arguments (including the program name)
    if (argc > 2) {
        printf("Usage: %s (-f) includes files )\n", argv[0]);
        return 1;  // Return with an error code
    }
    // Check if files should be included
    int8_t includeFiles = 0;
    if(argc == 2){
        if(*argv[1] == '-' && *(++argv[1]) == 'f'){
            includeFiles = 1;
        }
    }
    // Get current working dir
    char currentDirPath[1024];
    if (getcwd(currentDirPath, sizeof(currentDirPath)) == NULL) {
        perror("getcwd() error");
        exit(1);
    }
    printf("Current working directory: %s\n", currentDirPath);

    // Building the tree datastructure
    DirNode* rootNode = buildDirTree("/mnt/c/Users/pepij/CLionProjects", includeFiles);

    // Visualizing it
    char* structure = toString(rootNode, 0, "");
    printf("%s", structure);

    freeDirTree(rootNode);
    free(structure);
    return 0;
}
