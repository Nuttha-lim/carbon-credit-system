#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 10

int user_login();
int user_register();
void save_project_to_file(const char *username, const char *type, float carbon_credit, float price, const char *start_date, const char *end_date);
void load_projects_from_file();
void edit_project(int project_index, const char *username, const char *type, float carbon_credit, float price, const char *start_date, const char *end_date);

typedef struct {
    char username[100];
    char password[100];
    float carbon_credit;
} User;

typedef struct {
    char project_type[100];
    float carbon_credit;
    float price;
    char start_date[20];  // Format: YYYY-MM-DD
    char end_date[20];    // Format: YYYY-MM-DD
    char username[100];   // ผู้ขายที่เป็นเจ้าของโครงการ
} Project;

User *users = NULL;
Project *projects = NULL;
int user_count = 0;
int user_capacity = INITIAL_CAPACITY;
int project_count = 0;
int project_capacity = INITIAL_CAPACITY;

void clear_screen() {
    system("cls");
}

void load_users_from_file();
void load_projects_from_file();
void save_user_to_file(User user);

int main() {
    users = malloc(INITIAL_CAPACITY * sizeof(User));
    projects = malloc(INITIAL_CAPACITY * sizeof(Project));
    load_users_from_file();
    load_projects_from_file();

    int choose;
    do {
        printf("1. Login\n");
        printf("2. Register\n");
        printf("Choose: ");
        
        if (scanf("%d", &choose) != 1) {
            while(getchar() != '\n');
            printf("Invalid input! Please enter 1 or 2.\n");
            continue;
        }
        
        if (choose == 1) {
            clear_screen();
            int result = user_login();
            if (result == -1) continue;
            break;
        } else if (choose == 2) {
            clear_screen();
            int result = user_register();
            if (result == -1) continue;
            break;
        } else {
            printf("Please choose 1 or 2\n");
        }
    } while (1);

    int role_choice;
    do {
        clear_screen();
        printf("Choose your role:\n");
        printf("1. Buyer\n");
        printf("2. Seller\n");
        printf("Choose: ");
        
        if (scanf("%d", &role_choice) != 1) {
            while(getchar() != '\n');
            printf("Invalid input! Please enter 1 or 2.\n");
            continue;
        }
        
        if (role_choice == 1) {
            clear_screen();
            printf("Search projects by type:\n");
            char search_type[100];
            printf("Enter project type: ");
            scanf("%s", search_type);
            printf("Results for '%s' projects:\n", search_type);
            for (int i = 0; i < project_count; i++) {
                if (strstr(projects[i].project_type, search_type)) {
                    printf("Type: %s, Carbon Credit: %.2f, Price: %.2f, Start Date: %s, End Date: %s, Seller: %s\n",
                            projects[i].project_type, projects[i].carbon_credit,
                            projects[i].price, projects[i].start_date, projects[i].end_date, projects[i].username);
                }
            }
            break;
        } else if (role_choice == 2) {
            clear_screen();
            char project_type[100], start_date[20], end_date[20];
            float carbon_credit, price;
            char username[100];
            printf("Enter your username: ");
            scanf("%s", username);
            printf("Enter project type (e.g., renewable energy, forest conservation): ");
            scanf("%s", project_type);
            printf("Enter carbon credit (tons CO2): ");
            scanf("%f", &carbon_credit);
            printf("Enter price: ");
            scanf("%f", &price);
            printf("Enter project start date (YYYY-MM-DD): ");
            scanf("%s", start_date);
            printf("Enter project end date (YYYY-MM-DD): ");
            scanf("%s", end_date);
            
            // Save project data with the seller's username
            save_project_to_file(username, project_type, carbon_credit, price, start_date, end_date);
            break;
        } else {
            printf("Please choose 1 or 2\n");
        }
    } while (1);

    // Option to edit a project
    int edit_choice;
    printf("\nDo you want to edit a project? (1 for Yes, 0 for No): ");
    scanf("%d", &edit_choice);
    if (edit_choice == 1) {
        char username[100], new_type[100], new_start_date[20], new_end_date[20];
        float new_carbon_credit, new_price;
        printf("Enter your username: ");
        scanf("%s", username);
        printf("Enter project index to edit (0 to %d): ", project_count - 1);
        int index;
        scanf("%d", &index);
        if (index >= 0 && index < project_count && strcmp(projects[index].username, username) == 0) {
            printf("Enter new project type: ");
            scanf("%s", new_type);
            printf("Enter new carbon credit: ");
            scanf("%f", &new_carbon_credit);
            printf("Enter new price: ");
            scanf("%f", &new_price);
            printf("Enter new start date (YYYY-MM-DD): ");
            scanf("%s", new_start_date);
            printf("Enter new end date (YYYY-MM-DD): ");
            scanf("%s", new_end_date);

            // Edit project
            edit_project(index, username, new_type, new_carbon_credit, new_price, new_start_date, new_end_date);
        } else {
            printf("Invalid project index or you are not the owner of this project.\n");
        }
    }

    free(users);
    free(projects);
    return 0;
}

int user_login() {
    char username[100], password[100];
    int success = 0;
    
    do {
        clear_screen();
        printf("***Login***\n");
        printf("Enter 'back' to return to the main menu.\n");
        printf("Username: ");
        scanf("%s", username);
        
        if (strcmp(username, "back") == 0) return -1;

        printf("Password: ");
        scanf("%s", password);
        
        if (strcmp(password, "back") == 0) return -1;

        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
                printf("Login successful\n");
                success = 1;
                break;
            }
        }
        
        if (success == 0) {
            printf("Please check your username and password again\n");
        }
    } while (success == 0);
    
    return 0;
}

int user_register() {
    char username[100], password[100], confirm_password[100];
    float carbon_credit;
    int username_exists;
    
    do {
        clear_screen();
        username_exists = 0;
        printf("***Register***\n");
        printf("Enter 'back' to return to the main menu.\n");
        printf("Username: ");
        scanf("%s", username);
        
        if (strcmp(username, "back") == 0) return -1;
        
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, username) == 0) {
                printf("Username already exists. Please enter a new username.\n");
                username_exists = 1;
                break;
            }
        }
    } while (username_exists == 1);
    
    printf("Password: ");
    scanf("%s", password);
    
    if (strcmp(password, "back") == 0) return -1;
    
    printf("Confirm Password: ");
    scanf("%s", confirm_password);
    
    if (strcmp(password, confirm_password) != 0) {
        printf("Passwords do not match! Try again.\n");
        return -1;
    }

    printf("Enter your carbon credit: ");
    scanf("%f", &carbon_credit);
    
    if (user_count >= user_capacity) {
        user_capacity *= 2;
        users = realloc(users, user_capacity * sizeof(User));
    }

    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    users[user_count].carbon_credit = carbon_credit;
    user_count++;
    
    save_user_to_file(users[user_count - 1]);
    printf("Registration successful\n");
    
    return 0;
}

void save_user_to_file(User user) {
    FILE *file = fopen("user_data.txt", "a");
    if (file == NULL) {
        printf("Error saving user data\n");
        return;
    }
    fprintf(file, "%s %s %.2f\n", user.username, user.password, user.carbon_credit);
    fclose(file);
}

void save_project_to_file(const char *username, const char *type, float carbon_credit, float price, const char *start_date, const char *end_date) {
    if (project_count >= project_capacity) {
        project_capacity *= 2;
        projects = realloc(projects, project_capacity * sizeof(Project));
    }

    strcpy(projects[project_count].project_type, type);
    projects[project_count].carbon_credit = carbon_credit;
    projects[project_count].price = price;
    strcpy(projects[project_count].start_date, start_date);
    strcpy(projects[project_count].end_date, end_date);
    strcpy(projects[project_count].username, username); // เพิ่มชื่อผู้ขาย
    project_count++;

    FILE *file = fopen("data.txt", "a");
    if (file == NULL) {
        printf("Error saving project data\n");
        return;
    }

    fprintf(file, "%s %.2f %.2f %s %s %s\n", type, carbon_credit, price, start_date, end_date, username);
    fclose(file);
}

void edit_project(int project_index, const char *username, const char *type, float carbon_credit, float price, const char *start_date, const char *end_date) {
    strcpy(projects[project_index].project_type, type);
    projects[project_index].carbon_credit = carbon_credit;
    projects[project_index].price = price;
    strcpy(projects[project_index].start_date, start_date);
    strcpy(projects[project_index].end_date, end_date);

    // บันทึกข้อมูลโครงการที่แก้ไข
    FILE *file = fopen("data.txt", "w");
    if (file == NULL) {
        printf("Error saving project data\n");
        return;
    }

    for (int i = 0; i < project_count; i++) {
        fprintf(file, "%s %.2f %.2f %s %s %s\n", projects[i].project_type, projects[i].carbon_credit,
                projects[i].price, projects[i].start_date, projects[i].end_date, projects[i].username);
    }

    fclose(file);
}

void load_users_from_file() {
    FILE *file = fopen("user_data.txt", "r");
    if (file == NULL) return;
    
    User temp;
    while (fscanf(file, "%s %s %f", temp.username, temp.password, &temp.carbon_credit) == 3) {
        if (user_count >= user_capacity) {
            user_capacity *= 2;
            users = realloc(users, user_capacity * sizeof(User));
        }
        users[user_count++] = temp;
    }
    fclose(file);
}

void load_projects_from_file() {
    FILE *file = fopen("data.txt", "r");
    if (file == NULL) return;
    
    Project temp;
    while (fscanf(file, "%s %f %f %s %s %s", temp.project_type, &temp.carbon_credit, &temp.price,
                   temp.start_date, temp.end_date, temp.username) == 6) {
        if (project_count >= project_capacity) {
            project_capacity *= 2;
            projects = realloc(projects, project_capacity * sizeof(Project));
        }
        projects[project_count++] = temp;
    }
    fclose(file);
}
