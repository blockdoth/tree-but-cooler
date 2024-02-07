#include <stdio.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


#define MAX_PATH_LEN 1024

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

DirNode* addNode(DirNode* rootNode, char* filePath, char* fileName){
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
    //printf("Added %s to stack\n",dirNode->fileName);
}

DirNode* pop(Stack** stack){
    if (*stack == NULL || (*stack)->fileNode == NULL) {
        return NULL;
    }
    DirNode* fileNode = (*stack)->fileNode;
    Stack* new_top = (*stack)->top;
    free(*stack);
    *stack = new_top;
    //printf("Removed %s from stack\n",fileNode->fileName);
    return fileNode;
}


DirNode *buildDirTree(char* filePath) {
    Stack* stack = NULL;
    Dirent dirEntry;  // Pointer to a directory entry
    DirNode* rootNode = createNode(filePath, "Root");
    push(&stack, rootNode);

    //int i = 0;
    while(stack != NULL){
        //printf("Dirs traversed:\t%d\n",i++);
        DirNode* currentNode = pop(&stack);
        char* baseFilePath = currentNode->filePath;

        DIR* dirPointer = opendir(baseFilePath);
        while((dirEntry = readdir(dirPointer)) != NULL){
            if(strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0) {
                continue; // Skip the . and .. directories
            }
            //reading first file
            char newPath[FILENAME_MAX];
            snprintf(newPath, sizeof(newPath), "%s/%s", baseFilePath, dirEntry->d_name);
            Stat fileStat;
            if (stat(newPath, &fileStat) != 0) {
                perror("stat error");
                exit(1);
            }

            if (S_ISDIR(fileStat.st_mode)) {
                // Is dir
                //printf("DIR\t%s\n", dirEntry->d_name);
                DirNode* newNode = addNode(currentNode, newPath, dirEntry->d_name);
                push(&stack,  newNode);
            } else {
                // Is file
                addNode(currentNode, newPath,dirEntry->d_name);
                //printf("FILE\t%s\n", dirEntry->d_name);
            }
        }
//        fflush(stdout);
        //free(currentNode);
        closedir(dirPointer);
    }
    free(stack);
    return rootNode;
}

char* repeatString(char* base, size_t times){
    if (times < 1) return "";

    char *repeat = malloc(sizeof(base) * times + 1);
    if (repeat == NULL) return NULL;

    strcpy(repeat, base);
    while (--times > 0) {
        strcat(repeat, base);
    }
    return repeat;
}

char* toString(DirNode* rootNode, size_t depth, int leaf, char* prefix) {

    char* structure = strdup(rootNode->fileName);
    strcat(structure, "\n");

    DirNode* childNode = rootNode->childNodes;
    while(childNode != NULL){
        if(leaf){
            strcat(structure, repeatString("   ", depth));
        } else{
            strcat(structure, repeatString(" │ ", depth));
        }
        strcat(structure, prefix);

        if(childNode->nextNode == NULL){
            strcat(structure, " └─ ");
            strcat(structure, toString(childNode, depth + 1,prefix));

        }else{
            strcat(structure, " ├─ ");
            strcat(structure, toString(childNode, depth + 1,prefix));
        }
        childNode = childNode->nextNode;
    }
    return structure;
}

//        var structure: String = nodeId + "\n"
//        for (child in children) {
//            var folderStructure = ""
//            if (leave) {
//                folderStructure += "   ".repeat(depth)
//            }else {
//                folderStructure += " │ ".repeat(depth)
//            }
//            if (child == children.last()) {
//                folderStructure += " └ "
//                structure += folderStructure + child.toString(depth + 1, true)
//            } else{
//                folderStructure += " ├ "
//                structure += folderStructure + child.toString(depth + 1, false)
//            }
//        }



int main() {
    char currentDirPath[1024]; // Buffer to store the current working directory
    if (getcwd(currentDirPath, sizeof(currentDirPath)) != NULL) {
        printf("Current working directory: %s\n", currentDirPath);
    } else {
        perror("getcwd() error");
        exit(1);
    }

    DirNode* rootNode = buildDirTree("/mnt/c/Users/pepij/CLionProjects/tree_but_cooler/TestFolder");

    printf("%s",toString(rootNode, 0,0));

    return 0;
}
