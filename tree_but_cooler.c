#include <stdio.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// Ansi codes
#define RED "\033[91m"
#define ORANGE "\033[38;5;208m"
#define GREEN "\033[92m"
#define YELLOW "\033[93m"
#define BLUE "\033[94m"
#define PURPLE "\033[95m"
#define CYAN "\033[96m"
#define WHITE "\033[37m"
#define RESET "\033[0m"

// Colors list
const char* colors[] = {
        RED,
        ORANGE,
        YELLOW,
        GREEN,
        CYAN,
        BLUE,
        PURPLE
};
const size_t colorsLength = sizeof(colors) / sizeof(colors[0]);
size_t colorPointer = 0;

// Pipes
#define STRAIGHT " │  "
#define LCORNER " └─ "
#define TSPLIT " ├─ "
#define EMPTY "    "

// Typedefs
typedef struct dirent* Dirent;
typedef struct stat Stat;


//Global flags
int includeFiles = 0;
int gayMode = 0;


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
DirNode *buildDirTree(char* filePath) {
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
    // Decide color for this branch
    const char* color;
    if(gayMode){
        color = colors[colorPointer++];
        if(colorPointer > colorsLength - 1){
            colorPointer = 0;
        }
    }else{
        color = WHITE;
    }
    size_t ansiLength = strlen(color) + strlen(RESET);
    size_t prefixLength = ansiLength + 9; // Not sure why its 9, maybe because "└"and "├" encode to multiple chars
    // Builds dirname
    char* structure = (char*)malloc(strlen(rootNode->fileName) + ansiLength + 3);
    sprintf(structure,"%s%s%s\n", color, rootNode->fileName,  RESET);

    DirNode* childNode = rootNode->childNodes;
    while(childNode != NULL){
        char* pipeString =  (char*)malloc(prefixLength);
        char* prefixPipeString =  (char*)malloc(prefixLength);
        if(childNode->nextNode == NULL){ // Last item in the list
            sprintf(pipeString,         "%s%s%s", color, LCORNER ,  RESET);
            sprintf(prefixPipeString, "%s%s%s", color, EMPTY , RESET);
        }else{
            sprintf(pipeString,         "%s%s%s", color, TSPLIT,  RESET);
            sprintf(prefixPipeString,   "%s%s%s", color, STRAIGHT ,  RESET);
        }

        char* childPrefix = (char*)malloc( strlen(prefix) + strlen(prefixPipeString) + 1); // Make a copy for concat's
        sprintf(childPrefix, "%s%s", prefix, prefixPipeString);

        // Build the tree of the child dir
        char* childStructure = toString(childNode, depth + 1, childPrefix);
        // Merges the current tree of the current structure with the tree's of the child structures

        structure = (char*)realloc(structure, strlen(structure) + strlen(prefix) + strlen(pipeString) + strlen(childStructure) + 1);
        strcat(structure,prefix);
        strcat(structure,pipeString);
        strcat(structure,childStructure);

        free(pipeString);
        free(prefixPipeString);
        free(childPrefix);
        free(childStructure);
        childNode = childNode->nextNode;
    }

    return structure;
}



int main(int argc, char *argv[]) {

    // Parsing args
    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        if(*arg == '-'){
            if(*(++arg) == '-'){ // Multi char arg, ex "--gay"
                ++arg;
                if (strcmp(arg, "gay") == 0) {
                    gayMode = 1;
                } else if (strcmp(arg, "todo") == 0) {

                }
            }else{ // Single char arg, ex "-f"
                switch (*arg) {
                    case 'f':
                        includeFiles = 1;
                        break;
                    default:
                }
            }
        }else{
            printf("Usage: %s -f or --gay\n", argv[0]);
        }
    }

    // Get current working dir
    char currentDirPath[1024];
    if (getcwd(currentDirPath, sizeof(currentDirPath)) == NULL) {
        perror("getcwd() error");
        exit(1);
    }
    printf("Current working directory: %s\n", currentDirPath);

    //"/mnt/c/Users/pepij/CLionProjects"
    // Building the tree datastructure
    DirNode* rootNode = buildDirTree(currentDirPath);

    // Visualizing it
    char* structure = toString(rootNode, 0, "");
    printf("%s", structure);

    freeDirTree(rootNode);
    free(structure);
    return 0;
}
