#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120

typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} LocationType;

typedef union {
    int slipNum;
    char bayLetter;
    char licenseTag[10];
    int storageNum;
} LocationData;

typedef struct {
    char name[128];
    int length;
    LocationType type;
    LocationData location;
    float amountOwed;
} Boat;

Boat* boats[MAX_BOATS];
int boatCount = 0;

LocationType getLocationType(char* str) {
    if (strcmp(str, "slip") == 0) return SLIP;
    if (strcmp(str, "land") == 0) return LAND;
    if (strcmp(str, "trailor") == 0) return TRAILOR;
    if (strcmp(str, "storage") == 0) return STORAGE;
    return -1;
}

const char* getLocationTypeName(LocationType type) {
    switch (type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "unknown";
    }
}

void loadBoats(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (boatCount >= MAX_BOATS) break;

        Boat* b = malloc(sizeof(Boat));
        char typeStr[16], extra[16];

        sscanf(line, "%[^,],%d,%[^,],%[^,],%f",
               b->name, &b->length, typeStr, extra, &b->amountOwed);

        b->type = getLocationType(typeStr);
        switch (b->type) {
            case SLIP: b->location.slipNum = atoi(extra); break;
            case LAND: b->location.bayLetter = extra[0]; break;
            case TRAILOR: strcpy(b->location.licenseTag, extra); break;
            case STORAGE: b->location.storageNum = atoi(extra); break;
        }

        boats[boatCount++] = b;
    }

    fclose(file);
}

void saveBoats(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return;

    for (int i = 0; i < boatCount; i++) {
        Boat* b = boats[i];
        fprintf(file, "%s,%d,%s,", b->name, b->length, getLocationTypeName(b->type));
        switch (b->type) {
            case SLIP: fprintf(file, "%d", b->location.slipNum); break;
            case LAND: fprintf(file, "%c", b->location.bayLetter); break;
            case TRAILOR: fprintf(file, "%s", b->location.licenseTag); break;
            case STORAGE: fprintf(file, "%d", b->location.storageNum); break;
        }
        fprintf(file, ",%.2f\n", b->amountOwed);
    }

    fclose(file);
}

void printMenu() {
    printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
}

int compareBoatsByName(const void* a, const void* b) {
    Boat* b1 = *(Boat**)a;
    Boat* b2 = *(Boat**)b;
    return strcmp(b1->name, b2->name);
}

void printInventory() {
    if (boatCount == 0) {
        printf("No boats in inventory.\n");
        return;
    }

    Boat* sorted[MAX_BOATS];
    for (int i = 0; i < boatCount; i++) {
        sorted[i] = boats[i];
    }

    qsort(sorted, boatCount, sizeof(Boat*), compareBoatsByName);

    for (int i = 0; i < boatCount; i++) {
        Boat* b = sorted[i];
        printf("%s, %d ft, %s, ", b->name, b->length, getLocationTypeName(b->type));
        switch (b->type) {
            case SLIP: printf("slip %d", b->location.slipNum); break;
            case LAND: printf("bay %c", b->location.bayLetter); break;
            case TRAILOR: printf("tag %s", b->location.licenseTag); break;
            case STORAGE: printf("slot %d", b->location.storageNum); break;
        }
        printf(", $%.2f\n", b->amountOwed);
    }
}

void addBoat() {
    if (boatCount >= MAX_BOATS) {
        printf("No space for more boats.\n");
        return;
    }

    char line[256];
    printf("Enter boat CSV string: ");
    getchar();
    fgets(line, sizeof(line), stdin);

    Boat* b = malloc(sizeof(Boat));
    char typeStr[16], extra[16];

    if (sscanf(line, "%[^,],%d,%[^,],%[^,],%f",
               b->name, &b->length, typeStr, extra, &b->amountOwed) != 5) {
        printf("Invalid input.\n");
        free(b);
        return;
    }

    b->type = getLocationType(typeStr);
    switch (b->type) {
        case SLIP: b->location.slipNum = atoi(extra); break;
        case LAND: b->location.bayLetter = extra[0]; break;
        case TRAILOR: strcpy(b->location.licenseTag, extra); break;
        case STORAGE: b->location.storageNum = atoi(extra); break;
        default: printf("Invalid location type.\n"); free(b); return;
    }

    boats[boatCount++] = b;
    printf("Boat added.\n");
}

void removeBoat() {
    char name[128];
    printf("Enter boat name to remove: ");
    getchar();
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    for (int i = 0; i < boatCount; i++) {
        if (strcmp(boats[i]->name, name) == 0) {
            free(boats[i]);
            for (int j = i; j < boatCount - 1; j++) {
                boats[j] = boats[j + 1];
            }
            boatCount--;
            printf("Boat removed.\n");
            return;
        }
    }

    printf("Boat not found.\n");
}

void makePayment() {
    char name[128];
    float amount;
    printf("Enter boat name: ");
    getchar();
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    printf("Enter payment amount: ");
    scanf("%f", &amount);

    for (int i = 0; i < boatCount; i++) {
        if (strcmp(boats[i]->name, name) == 0) {
            boats[i]->amountOwed -= amount;
            if (boats[i]->amountOwed < 0) boats[i]->amountOwed = 0;
            printf("Payment applied. New balance: $%.2f\n", boats[i]->amountOwed);
            return;
        }
    }

    printf("Boat not found.\n");
}

void applyMonthlyCharge() {
    for (int i = 0; i < boatCount; i++) {
        float charge = 0;
        switch (boats[i]->type) {
            case SLIP: charge = boats[i]->length * 15; break;
            case LAND: charge = boats[i]->length * 12; break;
            case TRAILOR: charge = boats[i]->length * 10; break;
            case STORAGE: charge = boats[i]->length * 8; break;
        }
        boats[i]->amountOwed += charge;
    }
    printf("Monthly charges applied.\n");
}

void cleanup() {
    for (int i = 0; i < boatCount; i++) {
        free(boats[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <BoatData.csv>\n", argv[0]);
        return 1;
    }

    loadBoats(argv[1]);

    char option;
    do {
        printMenu();
        scanf(" %c", &option);
        option = toupper(option);

        switch (option) {
            case 'I': printInventory(); break;
            case 'A': addBoat(); break;
            case 'R': removeBoat(); break;
            case 'P': makePayment(); break;
            case 'M': applyMonthlyCharge(); break;
            case 'X': saveBoats(argv[1]); break;
            default: printf("Invalid option.\n");
        }

    } while (option != 'X');

    cleanup();
    return 0;
}
