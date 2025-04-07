#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_BOATS 120
#define MAX_NAME_LEN 127
#define MAX_LINE_LEN 256

// Storage types for boats
typedef enum {
	 SLIP,
	 LAND,
	 TRAILOR,
	 STORAGE
 } PlaceType;

// Type-specific storage details
typedef union {
    int slip_num;       // Slip number (1-85)
    char bay_letter;    // Bay letter (A-Z)
    char license_tag[8]; // Trailer license tag
    int storage_num;    // Storage space number (1-50)
} ExtraInfo;


// Boat information structure
typedef struct {
    char name[MAX_NAME_LEN + 1]; 
    int length;                  
    PlaceType type;              
    ExtraInfo extra_info;        
    float amount_owed;       
} Boat;

// Function Declerations

// Load boat data from CSV file into marina array
void loadFromCSV(const char *filename, Boat *marina[], int *count);

// Save boat data from marina array to CSV file
void saveToCSV(const char *filename, Boat *marina[], int count);

// Add new boat from CSV string (format: "name,length,type,extra,owed")
void addBoat(Boat *marina[], int *count, const char *csvLine);

// Remove boat by name (case-insensitive search)
void removeBoat(Boat *marina[], int *count, const char *name);

// Process payment for a boat (rejects negative/over payments)
void processPayment(Boat *marina[], int count, const char *name, float amount);

// Update all boat balances for new month (applies storage fees)
void updateMonthly(Boat *marina[], int count);

//Comparison function for qsort to sort boats alphabetically by name 
int compareBoats(const void *a, const void *b);

// Print all boats sorted by name
void printInventory(Boat *marina[], int count);

// Check if boat exists by name
bool isBoatExists(Boat *marina[], int count, const char *name);

// Free all allocated boat memory
void freeAllBoats(Boat *marina[], int count);

// Case-insensitive string comparison
int strcasecmp(const char *s1, const char *s2);

// Main Function
int main(int argc, char *argv[]) {
    if (argc != 2 || strcmp(argv[1], "BoatData.csv") != 0) {
        if (argc != 2) {
            fprintf(stderr, "Error: %s\n",
                argc < 2 ? "Needs BoatData.csv" : "Invalid input (too many arguments)");
        } else {
            fprintf(stderr, "Error: Invalid file\n");
        }
        return 1;
    }

    Boat *marina[MAX_BOATS] = { NULL };
    int boatCount = 0;

    loadFromCSV(argv[1], marina, &boatCount);

    char choice;
    do {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        scanf(" %c", &choice);
        choice = tolower(choice);

        
        while (getchar() != '\n') { /* empty */ }

        switch (choice) {
            case 'i':
                printInventory(marina, boatCount);
                break;
            case 'a': {
                char csvLine[MAX_LINE_LEN];
                printf("Enter boat data (CSV format): ");
                scanf(" %255[^\n]", csvLine);
                addBoat(marina, &boatCount, csvLine);
                break;
            }

            case 'r': {
                char name[MAX_NAME_LEN + 1];
                printf("Enter boat name to remove: ");
                scanf(" %127[^\n]", name);
                removeBoat(marina, &boatCount, name);
                break;
            }
            case 'p': {
                char name[MAX_NAME_LEN + 1];
                float amount;
                printf("Enter boat name: ");
                scanf(" %127[^\n]", name);
 		
		bool boatExists = false;
    		for (int i = 0; i < boatCount; i++) {
        		if (marina[i] && strcasecmp(marina[i]->name, name) == 0) {
            			boatExists = true;
            			break;
       			 }
 		 }
    
   		 if (!boatExists) {
      			 printf("No boat with that name\n");
       			 break;  
   		 }
    
                printf("Enter payment amount: ");
                scanf("%f", &amount);
                processPayment(marina, boatCount, name, amount);
                break;
           }
            case 'm':
                updateMonthly(marina, boatCount);
                printf("Monthly charges applied.\n");
                break;
            case 'x':
                break;
            default:
                printf("Invalid option %c\n ",toupper(choice));
        }
    } while (choice != 'x');

    saveToCSV(argv[1], marina, boatCount);
    freeAllBoats(marina, boatCount);
    printf("Exiting the Boat Management System\n");
    return 0;
}

// Function Definitions

void loadFromCSV(const char *filename, Boat *marina[], int *count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: File '%s' not found.\n", filename);
        exit(1);
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), file)) {
        if (strlen(line) <= 1) continue;

        Boat *newBoat = malloc(sizeof(Boat));
        if (!newBoat) {
            perror("malloc failed");
            fclose(file);
            exit(1);
        }

        char *token = strtok(line, ",");
        if (!token) continue;
        strncpy(newBoat->name, token, MAX_NAME_LEN);
        newBoat->name[MAX_NAME_LEN] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        newBoat->length = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        if (strcmp(token, "slip") == 0) {
            newBoat->type = SLIP;
            token = strtok(NULL, ",");
            if (token) newBoat->extra_info.slip_num = atoi(token);
        } else if (strcmp(token, "land") == 0) {
            newBoat->type = LAND;
            token = strtok(NULL, ",");
            if (token) newBoat->extra_info.bay_letter = token[0];
        } else if (strcmp(token, "trailor") == 0) {
            newBoat->type = TRAILOR;
            token = strtok(NULL, ",");
            if (token) strncpy(newBoat->extra_info.license_tag, token, 7);
        } else if (strcmp(token, "storage") == 0) {
            newBoat->type = STORAGE;
            token = strtok(NULL, ",");
            if (token) newBoat->extra_info.storage_num = atoi(token);
        }

        token = strtok(NULL, ",");
        if (token) newBoat->amount_owed = atof(token);

        if (isBoatExists(marina, *count, newBoat->name)) {
            fprintf(stderr, "Duplicate boat name: %s\n", newBoat->name);
            free(newBoat);
            continue;
        }

        marina[(*count)++] = newBoat;
        if (*count >= MAX_BOATS) break;
    }
    fclose(file);
}

void saveToCSV(const char *filename, Boat *marina[], int count) {
    Boat *sortedMarina[MAX_BOATS];
    memcpy(sortedMarina, marina, count * sizeof(Boat *));
    qsort(sortedMarina, count, sizeof(Boat *), compareBoats);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file for writing");
        return;
    }

    for (int i = 0; i < count; i++) {
        Boat *boat = sortedMarina[i];
        fprintf(file, "%s,%d,", boat->name, boat->length);

        switch (boat->type) {
            case SLIP:
                fprintf(file, "slip,%d,", boat->extra_info.slip_num);
                break;
            case LAND:
                fprintf(file, "land,%c,", boat->extra_info.bay_letter);
                break;
            case TRAILOR:
                fprintf(file, "trailor,%s,", boat->extra_info.license_tag);
                break;
            case STORAGE:
                fprintf(file, "storage,%d,", boat->extra_info.storage_num);
                break;
        }
        fprintf(file, "%.2f\n", boat->amount_owed);
    }
    if (fclose(file) == EOF) {
        perror("Error closing file");
    }
}

void addBoat(Boat *marina[], int *count, const char *csvLine) {
    if (*count >= MAX_BOATS) {
        printf("Marina is full (max %d boats).\n", MAX_BOATS);
        return;
    }

    char tempLine[MAX_LINE_LEN];
    strncpy(tempLine, csvLine, MAX_LINE_LEN);
    tempLine[MAX_LINE_LEN - 1] = '\0';
    
    char *nameToken = strtok(tempLine, ",");
    if (nameToken && isBoatExists(marina, *count, nameToken)) {
        printf("Boat '%s' already exists!\n", nameToken);
        return;
    }

    Boat *newBoat = malloc(sizeof(Boat));
    if (!newBoat) {
        perror("Failed to allocate memory for boat");
        return;
    }

    strncpy(tempLine, csvLine, MAX_LINE_LEN);
    tempLine[MAX_LINE_LEN - 1] = '\0';

    char *token = strtok(tempLine, ",");
    if (!token) {
        free(newBoat);
        return;
    }
    strncpy(newBoat->name, token, MAX_NAME_LEN);
    newBoat->name[MAX_NAME_LEN] = '\0';

    token = strtok(NULL, ",");
    if (!token) {
        free(newBoat);
        return;
    }
    newBoat->length = atoi(token);

    token = strtok(NULL, ",");
    if (!token) {
        free(newBoat);
        return;
    }
    if (strcmp(token, "slip") == 0) {
        newBoat->type = SLIP;
        token = strtok(NULL, ",");
        if (token) newBoat->extra_info.slip_num = atoi(token);
    } else if (strcmp(token, "land") == 0) {
        newBoat->type = LAND;
        token = strtok(NULL, ",");
        if (token) newBoat->extra_info.bay_letter = token[0];
    } else if (strcmp(token, "trailor") == 0) {
        newBoat->type = TRAILOR;
        token = strtok(NULL, ",");
        if (token) strncpy(newBoat->extra_info.license_tag, token, 7);
    } else if (strcmp(token, "storage") == 0) {
        newBoat->type = STORAGE;
        token = strtok(NULL, ",");
        if (token) newBoat->extra_info.storage_num = atoi(token);
    }

   
    token = strtok(NULL, ",");
    if (!token) {
        free(newBoat);
        return;
    }
    float amount = atof(token);
    if (amount < 0) {
        printf("Error: Payment amount cannot be negative\n");
        free(newBoat);
        return;
    }
    newBoat->amount_owed = amount;

   
    marina[(*count)++] = newBoat;
}
    
void removeBoat(Boat *marina[], int *count, const char *name) {
    for (int i = 0; i < *count; i++) {
        if (strcasecmp(marina[i]->name, name) == 0) {
            free(marina[i]);
            for (int j = i; j < *count - 1; j++) {
                marina[j] = marina[j + 1];
            }
            (*count)--;
            return;
        }
    }
    printf("No boat with that name\n");
}

void processPayment(Boat *marina[], int count, const char *name, float amount) {
    if (amount <= 0) {
        printf("Payment must be positive amount!\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (strcasecmp(marina[i]->name, name) == 0) {
            if (amount > marina[i]->amount_owed) {
                printf("Payment exceeds amount owed ($%.2f).\n", marina[i]->amount_owed);
                return;
            }
            marina[i]->amount_owed -= amount;
            printf("Payment of $%.2f accepted for '%s'.\n", amount, name);
            return;
        }
    }
    printf("No boat with name '%s' exists\n", name);
}

void updateMonthly(Boat *marina[], int count) {
    for (int i = 0; i < count; i++) {
        switch (marina[i]->type) {
            case SLIP:   marina[i]->amount_owed += marina[i]->length * 12.50; break;
            case LAND:   marina[i]->amount_owed += marina[i]->length * 14.00; break;
            case TRAILOR: marina[i]->amount_owed += marina[i]->length * 25.00; break;
            case STORAGE: marina[i]->amount_owed += marina[i]->length * 11.20; break;
        }
    }
}

int compareBoats(const void *a, const void *b) {
    const Boat *boatA = *(const Boat **)a;
    const Boat *boatB = *(const Boat **)b;
    return strcasecmp(boatA->name, boatB->name);
}

void printInventory(Boat *marina[], int count) {
    Boat *sortedMarina[MAX_BOATS];
    memcpy(sortedMarina, marina, count * sizeof(Boat *));
    
    qsort(sortedMarina, count, sizeof(Boat *), compareBoats);
    
    printf("\nBoat Inventory:\n");
    printf("-----------------------------------------\n");
    for (int i = 0; i < count; i++) {
        Boat *boat = sortedMarina[i];
        printf("%-20s %3d' ", boat->name, boat->length);

        switch (boat->type) {
            case SLIP:   printf("slip   # %-5d", boat->extra_info.slip_num); break;
            case LAND:   printf("land      %-3c", boat->extra_info.bay_letter); break;
            case TRAILOR: printf("trailor %-8s", boat->extra_info.license_tag); break;
            case STORAGE: printf("storage # %-5d", boat->extra_info.storage_num); break;
        }
        printf(" Owes $%8.2f\n", boat->amount_owed);
    }
    printf("-----------------------------------------\n");
}

bool isBoatExists(Boat *marina[], int count, const char *name) {
    for (int i = 0; i < count; i++) {
        if (strcasecmp(marina[i]->name, name) == 0) {
            return true;
        }
    }
    return false;
}

void freeAllBoats(Boat *marina[], int count) {
    for (int i = 0; i < count; i++) {
        free(marina[i]);
    }
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
    }
    return tolower(*s1) - tolower(*s2);
}
