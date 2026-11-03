/* Recovered from the December 2024 more2.c project. See docs/SOURCE_REVIEW.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <float.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#define input_is_terminal() _isatty(0)
#else
#include <unistd.h>
#include <termios.h>
#define input_is_terminal() isatty(STDIN_FILENO)
#endif
#define INITIAL_CAPACITY 10
#define MAX_USERNAME_LENGTH 100
#define MAX_PASSWORD_LENGTH 100
#define MAX_ANSWER_LENGTH 100
#define MAX_PROJECT_TYPE_LENGTH 100
#define MAX_EMAIL_LENGTH 100
#define MAX_TRANSACTIONS 100
#define MAX_LINE_LENGTH 4096
#define MAX_AMOUNT 1000000.0f

typedef struct
{
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char answer_security_question1[MAX_ANSWER_LENGTH];
    char answer_security_question2[MAX_ANSWER_LENGTH];
    char answer_security_question3[MAX_ANSWER_LENGTH];
    float carbon_credit;
    float balance;
    char email[MAX_EMAIL_LENGTH];
} User;

typedef struct
{
    int project_id;
    char project_name[256];
    char project_description[512];
    char project_type[MAX_PROJECT_TYPE_LENGTH];
    float carbon_credit;
    float price;
    int project_duration;
    char environmental_impact[512];
    char related_documents[256];
    char contact_info[256];
    char username[MAX_USERNAME_LENGTH];
    char status[50];
} Project;

typedef struct
{
    int tProject_id;
    char tProject_name[256];
    char tSeller_name[MAX_USERNAME_LENGTH];
    char tBuyer_name[MAX_USERNAME_LENGTH];
    char tStatus[50];
} Transaction;


Transaction tTransactions[MAX_TRANSACTIONS];
int tTransaction_count = 0;
User *users = NULL;
Project *projects = NULL;
int user_count = 0, user_capacity = 0;
int project_count = 0, project_capacity = 0;
int project_id_counter = 1, current_user_index = -1;

void clear_screen(void);
void safe_input(char *buffer, int max_length);
void get_password(char *password, int max_length);
void save_user_to_file(void);
void save_all_projects_to_file(void);
void save_transaction(void);
void load_users_from_file(void);
void load_projects_from_file(void);
void load_transactions(void);
void add_new_project(void);
void edit_project(void);
void delete_project(void);
void edit_profile(void);
void check_order_status(void);
void main_menu(void);
int user_login(void);
int user_register(void);
int user_forget_password(char *username);
int reset_password(char *username, int count);
int search_projects(void);
int buyer_menu(void);
void seller_menu(void);
void buy_from_filtered(char *project_id_input);
bool can_edit_project(int project_id);

static void fatal(const char *message)
{
    fprintf(stderr, "Error: %s\n", message);
    exit(EXIT_FAILURE);
}

static void *grow_array(void *array, int *capacity, size_t element_size, int needed)
{
    if (needed <= *capacity) return array;
    int next = *capacity ? *capacity * 2 : INITIAL_CAPACITY;
    if (next < needed) next = needed;
    void *grown = realloc(array, (size_t)next * element_size);
    if (!grown) fatal("Not enough memory.");
    *capacity = next;
    return grown;
}

static int user_index(const char *name)
{
    for (int i = 0; i < user_count; ++i)
        if (strcmp(users[i].username, name) == 0) return i;
    return -1;
}

static int project_index(int id)
{
    for (int i = 0; i < project_count; ++i)
        if (projects[i].project_id == id) return i;
    return -1;
}

static int transaction_index(int id)
{
    for (int i = 0; i < tTransaction_count; ++i)
        if (tTransactions[i].tProject_id == id) return i;
    return -1;
}

static bool amount_valid(float amount)
{
    return isfinite(amount) && amount >= 0 && amount <= MAX_AMOUNT;
}

static bool token_valid(const char *value)
{
    if (!*value) return false;
    for (; *value; ++value)
        if (isspace((unsigned char)*value) || *value == '"' ||
            (unsigned char)*value < 32) return false;
    return true;
}

static bool username_valid(const char *value)
{
    if (!*value) return false;
    for (; *value; ++value)
        if (!isalnum((unsigned char)*value) && !strchr("_.-", *value)) return false;
    return true;
}

static bool account_valid(const User *u)
{
    return username_valid(u->username) && token_valid(u->password) &&
        token_valid(u->answer_security_question1) && token_valid(u->answer_security_question2) &&
        token_valid(u->answer_security_question3) && token_valid(u->email) &&
        strchr(u->email, '@') && strchr(u->email, '.') &&
        amount_valid(u->carbon_credit) && amount_valid(u->balance);
}

static bool parse_int(const char *value, int *out)
{
    char *end;
    errno = 0;
    long result = strtol(value, &end, 10);
    if (end == value || *end || errno || result < 0 || result > INT_MAX) return false;
    *out = (int)result;
    return true;
}

static bool parse_amount(const char *value, float *out)
{
    char *end;
    errno = 0;
    float result = strtof(value, &end);
    if (end == value || *end || errno || !amount_valid(result)) return false;
    /* The file format stores two decimal places. Normalize before reserving funds. */
    *out = (float)((long long)((double)result * 100.0 + 0.5) / 100.0);
    return true;
}

void safe_input(char *buffer, int max_length)
{
    for (;;)
    {
        fflush(stdout);
        if (!fgets(buffer, max_length, stdin))
        {
            if (ferror(stdin)) fatal("Unable to read input.");
            free(users);
            free(projects);
            exit(EXIT_SUCCESS);
        }
        size_t end = strcspn(buffer, "\r\n");
        if (!buffer[end] && !feof(stdin))
        {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            printf("Input is too long (maximum %d characters). Try again: ", max_length - 2);
            continue;
        }
        buffer[end] = '\0';
        if (strchr(buffer, '"'))
        {
            printf("Double quotes are not supported in fields. Try again: ");
            continue;
        }
        return;
    }
}

static int read_number(const char *format, void *out)
{
    char line[100];
    for (;;)
    {
        safe_input(line, sizeof(line));
        bool valid = strcmp(format, "%d") == 0 ? parse_int(line, out) : parse_amount(line, out);
        if (valid) return 1;
        printf("Enter a non-negative number (amounts: at most 1,000,000): ");
    }
}

void get_password(char *password, int max_length)
{
    if (!input_is_terminal()) { safe_input(password, max_length); return; }
#ifdef _WIN32
    for (;;)
    {
        int length = 0, ch;
        bool overflow = false;
        while ((ch = _getch()) != '\r')
        {
            if (ch == 0 || ch == 224) { (void)_getch(); continue; }
            if (ch == '\b') { if (length > 0) { --length; printf("\b \b"); } }
            else if (ch >= 32 && ch < 127)
            {
                if (length < max_length - 1) { password[length++] = (char)ch; putchar('*'); }
                else overflow = true;
            }
        }
        password[length] = '\0';
        putchar('\n');
        if (!overflow && !strchr(password, '"')) return;
        printf("Password is too long or contains a double quote. Try again: ");
    }
#else
    struct termios original_terminal, hidden;
    if (tcgetattr(STDIN_FILENO, &original_terminal) != 0) fatal("Cannot read terminal settings.");
    hidden = original_terminal;
    hidden.c_lflag &= (tcflag_t)~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &hidden) != 0) fatal("Cannot hide password input.");
    /* Read directly so terminal echo is restored even on EOF. */
    char *result = fgets(password, max_length, stdin);
    bool overflow = result && !strchr(password, '\n') && !feof(stdin);
    if (overflow) { int ch; while ((ch = getchar()) != '\n' && ch != EOF) {} }
    int restored = tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
    putchar('\n');
    if (restored != 0) fatal("Cannot restore terminal settings.");
    if (!result) { if (ferror(stdin)) fatal("Cannot read password."); exit(EXIT_SUCCESS); }
    password[strcspn(password, "\r\n")] = '\0';
    if (overflow || strchr(password, '"'))
    {
        printf("Password is too long or contains a double quote. Try again: ");
        get_password(password, max_length);
    }
#endif
}

void clear_screen(void)
{
    /* Keep redirected output readable for CLI tests and saved demo sessions. */
    if (!input_is_terminal()) return;
#ifdef _WIN32
    (void)system("cls");
#else
    printf("\033[2J\033[H");
#endif
}

static FILE *open_data(const char *name)
{
    FILE *file = fopen(name, "r");
    if (!file && errno != ENOENT) fatal("Cannot read a data file. Check file permissions.");
    return file;
}

static void bad_record(const char *name, int line)
{
    fprintf(stderr, "Error: invalid or duplicate record in %s at line %d. No data was saved.\n", name, line);
    exit(EXIT_FAILURE);
}

static bool read_record(FILE *file, char *line, const char *name, int number)
{
    if (!fgets(line, MAX_LINE_LENGTH, file))
    {
        if (ferror(file)) fatal("Cannot finish reading a data file.");
        return false;
    }
    if (!strchr(line, '\n') && !feof(file)) bad_record(name, number);
    line[strcspn(line, "\r\n")] = '\0';
    return true;
}

/* Read both the original CSV records and quoted names containing commas. */
static bool csv_fields(const char *line, char fields[][513], int count)
{
    for (int field = 0; field < count; ++field)
    {
        int length = 0;
        bool quoted = *line == '"';
        if (quoted) ++line;
        while (*line)
        {
            if (quoted && *line == '"')
            {
                if (line[1] == '"') ++line;
                else break;
            }
            else if (!quoted && *line == ',') break;
            if (length >= 512) return false;
            fields[field][length++] = *line++;
        }
        fields[field][length] = '\0';
        if (quoted) { if (*line != '"') return false; ++line; }
        if (field + 1 < count) { if (*line != ',') return false; ++line; }
        else if (*line) return false;
    }
    return true;
}

static bool copy_field(char *destination, size_t capacity, const char *source)
{
    if (strlen(source) >= capacity || strchr(source, '"')) return false;
    strcpy(destination, source);
    return true;
}

static void finish_file(FILE *file, const char *temporary, const char *destination)
{
    int failed = ferror(file);
    if (fclose(file) != 0 || failed) fatal("Cannot save data; the previous file was kept.");
#ifdef _WIN32
    if (!MoveFileExA(temporary, destination, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        fatal("Cannot replace data file; check file permissions.");
#else
    if (rename(temporary, destination) != 0) fatal("Cannot replace data file; check file permissions.");
#endif
}

void save_user_to_file(void)
{
    FILE *file = fopen("users.txt.tmp", "w");
    if (!file) fatal("Cannot save users.txt.");
    for (int i = 0; i < user_count; ++i)
    {
        User *u = &users[i];
        fprintf(file, "%s %s %s %s %s %.2f %.2f %s\n", u->username, u->password,
            u->answer_security_question1, u->answer_security_question2,
            u->answer_security_question3, u->carbon_credit, u->balance, u->email);
    }
    finish_file(file, "users.txt.tmp", "users.txt");
}

void save_all_projects_to_file(void)
{
    FILE *file = fopen("projects.txt.tmp", "w");
    if (!file) fatal("Cannot save projects.txt.");
    for (int i = 0; i < project_count; ++i)
    {
        Project *p = &projects[i];
        fprintf(file, "\"%04d\",\"%s\",\"%s\",\"%s\",%.2f,%.2f,%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
            p->project_id, p->project_name, p->project_description, p->project_type,
            p->carbon_credit, p->price, p->project_duration, p->environmental_impact,
            p->related_documents, p->contact_info, p->username, p->status);
    }
    finish_file(file, "projects.txt.tmp", "projects.txt");
}

void save_transaction(void)
{
    FILE *file = fopen("transaction.txt.tmp", "w");
    if (!file) fatal("Cannot save transaction.txt.");
    for (int i = 0; i < tTransaction_count; ++i)
    {
        Transaction *t = &tTransactions[i];
        fprintf(file, "%d,\"%s\",\"%s\",\"%s\",\"%s\"\n", t->tProject_id,
            t->tProject_name, t->tSeller_name, t->tBuyer_name, t->tStatus);
    }
    finish_file(file, "transaction.txt.tmp", "transaction.txt");
}

static void save_state(void)
{
    save_user_to_file();
    save_all_projects_to_file();
    save_transaction();
}

void load_users_from_file(void)
{
    user_count = 0;
    FILE *file = open_data("users.txt");
    if (!file) return;
    char line[MAX_LINE_LENGTH];
    for (int number = 1; read_record(file, line, "users.txt", number); ++number)
    {
        if (!*line) continue;
        User u = {0};
        char fields[8][100];
        const char *cursor = line;
        int count = 0;
        while (*cursor && count < 8)
        {
            while (isspace((unsigned char)*cursor)) ++cursor;
            if (!*cursor) break;
            int length = 0;
            while (*cursor && !isspace((unsigned char)*cursor))
            {
                if (length >= 99) bad_record("users.txt", number);
                fields[count][length++] = *cursor++;
            }
            fields[count++][length] = '\0';
        }
        while (isspace((unsigned char)*cursor)) ++cursor;
        bool valid = count == 8 && !*cursor;
        if (valid)
        {
            strcpy(u.username, fields[0]);
            strcpy(u.password, fields[1]);
            strcpy(u.answer_security_question1, fields[2]);
            strcpy(u.answer_security_question2, fields[3]);
            strcpy(u.answer_security_question3, fields[4]);
            strcpy(u.email, fields[7]);
            valid = parse_amount(fields[5], &u.carbon_credit) && parse_amount(fields[6], &u.balance);
        }
        if (!valid || !account_valid(&u) || user_index(u.username) >= 0)
            bad_record("users.txt", number);
        users = grow_array(users, &user_capacity, sizeof(User), user_count + 1);
        users[user_count++] = u;
    }
    fclose(file);
}

static bool project_valid(const Project *p)
{
    return *p->project_name && *p->project_type && *p->related_documents && *p->contact_info &&
        amount_valid(p->carbon_credit) && p->carbon_credit >= 0.01f &&
        amount_valid(p->price) && p->price >= 0.01f && p->project_duration > 0;
}

void load_projects_from_file(void)
{
    project_count = 0;
    project_id_counter = 1;
    FILE *file = open_data("projects.txt");
    if (!file) return;
    char line[MAX_LINE_LENGTH], f[12][513];
    for (int number = 1; read_record(file, line, "projects.txt", number); ++number)
    {
        if (!*line) continue;
        Project p = {0};
        bool valid = csv_fields(line, f, 12) && parse_int(f[0], &p.project_id) &&
            p.project_id > 0 && p.project_id < INT_MAX &&
            copy_field(p.project_name, sizeof(p.project_name), f[1]) &&
            copy_field(p.project_description, sizeof(p.project_description), f[2]) &&
            copy_field(p.project_type, sizeof(p.project_type), f[3]) &&
            parse_amount(f[4], &p.carbon_credit) && parse_amount(f[5], &p.price) &&
            parse_int(f[6], &p.project_duration) &&
            copy_field(p.environmental_impact, sizeof(p.environmental_impact), f[7]) &&
            copy_field(p.related_documents, sizeof(p.related_documents), f[8]) &&
            copy_field(p.contact_info, sizeof(p.contact_info), f[9]) &&
            copy_field(p.username, sizeof(p.username), f[10]) &&
            copy_field(p.status, sizeof(p.status), f[11]) &&
            (!strcmp(p.status, "Available") || !strcmp(p.status, "Pending") || !strcmp(p.status, "Complete"));
        if (!valid || !project_valid(&p) || user_index(p.username) < 0 || project_index(p.project_id) >= 0)
            bad_record("projects.txt", number);
        projects = grow_array(projects, &project_capacity, sizeof(Project), project_count + 1);
        projects[project_count++] = p;
        if (p.project_id >= project_id_counter) project_id_counter = p.project_id + 1;
    }
    fclose(file);
}

void load_transactions(void)
{
    tTransaction_count = 0;
    FILE *file = open_data("transaction.txt");
    if (!file) return;
    char line[MAX_LINE_LENGTH], f[5][513];
    for (int number = 1; read_record(file, line, "transaction.txt", number); ++number)
    {
        if (!*line) continue;
        Transaction t = {0};
        bool valid = csv_fields(line, f, 5) && parse_int(f[0], &t.tProject_id) &&
            copy_field(t.tProject_name, sizeof(t.tProject_name), f[1]) &&
            copy_field(t.tSeller_name, sizeof(t.tSeller_name), f[2]) &&
            copy_field(t.tBuyer_name, sizeof(t.tBuyer_name), f[3]) &&
            copy_field(t.tStatus, sizeof(t.tStatus), f[4]) &&
            (!strcmp(t.tStatus, "Pending") || !strcmp(t.tStatus, "Complete") || !strcmp(t.tStatus, "Cancelled"));
        int p = valid ? project_index(t.tProject_id) : -1;
        if (!valid || p < 0 || user_index(t.tBuyer_name) < 0 ||
            strcmp(projects[p].username, t.tSeller_name) || !strcmp(t.tBuyer_name, t.tSeller_name) ||
            strcmp(projects[p].status, !strcmp(t.tStatus, "Cancelled") ? "Available" : t.tStatus) ||
            transaction_index(t.tProject_id) >= 0 || tTransaction_count >= MAX_TRANSACTIONS)
            bad_record("transaction.txt", number);
        tTransactions[tTransaction_count++] = t;
    }
    fclose(file);
}

static void validate_pending_projects(void)
{
    for (int i = 0; i < project_count; ++i)
        if (!strcmp(projects[i].status, "Pending") && transaction_index(projects[i].project_id) < 0)
            fatal("A pending project has no transaction. Restore a consistent data backup.");
}

/* Order state is derived from persisted transactions, never an uninitialized User cache. */
static bool request_purchase(int id, int buyer)
{
    int p = project_index(id);
    if (p < 0 || buyer < 0 || buyer >= user_count || tTransaction_count >= MAX_TRANSACTIONS) return false;
    Project *project = &projects[p];
    int seller = user_index(project->username);
    if (seller < 0 || buyer == seller || strcmp(project->status, "Available") ||
        transaction_index(id) >= 0 || !project_valid(project) ||
        users[buyer].balance < project->price) return false;
    Transaction t = {0};
    t.tProject_id = id;
    strcpy(t.tProject_name, project->project_name);
    strcpy(t.tSeller_name, project->username);
    strcpy(t.tBuyer_name, users[buyer].username);
    strcpy(t.tStatus, "Pending");
    tTransactions[tTransaction_count++] = t;
    users[buyer].balance -= project->price;
    strcpy(project->status, "Pending");
    save_state();
    return true;
}

static void remove_transaction(int index)
{
    for (int i = index; i + 1 < tTransaction_count; ++i) tTransactions[i] = tTransactions[i + 1];
    --tTransaction_count;
}

static bool resolve_order(int id, int actor, int action)
{
    /* 1: seller confirms; 2: participant cancels pending; 3: buyer acknowledges. */
    int t = transaction_index(id), p = project_index(id);
    if (t < 0 || p < 0 || actor < 0 || actor >= user_count) return false;
    Transaction *order = &tTransactions[t];
    Project *project = &projects[p];
    int buyer = user_index(order->tBuyer_name), seller = user_index(order->tSeller_name);
    if (buyer < 0 || seller < 0 || strcmp(project->username, order->tSeller_name)) return false;
    if (action == 3)
    {
        if (actor != buyer || (strcmp(order->tStatus, "Complete") && strcmp(order->tStatus, "Cancelled"))) return false;
        remove_transaction(t);
    }
    else
    {
        if (strcmp(order->tStatus, "Pending") || strcmp(project->status, "Pending")) return false;
        if (action == 1)
        {
            if (actor != seller || !amount_valid(users[seller].balance + project->price) ||
                !amount_valid(users[buyer].carbon_credit + project->carbon_credit)) return false;
            users[seller].balance += project->price;
            users[buyer].carbon_credit += project->carbon_credit;
            strcpy(order->tStatus, "Complete");
            strcpy(project->status, "Complete");
        }
        else if (action == 2)
        {
            if ((actor != seller && actor != buyer) || !amount_valid(users[buyer].balance + project->price)) return false;
            users[buyer].balance += project->price;
            strcpy(project->status, "Available");
            if (actor == buyer) remove_transaction(t);
            else strcpy(order->tStatus, "Cancelled");
        }
        else return false;
    }
    save_state();
    return true;
}

bool can_edit_project(int id)
{
    int p = project_index(id);
    bool allowed = p >= 0 && current_user_index >= 0 &&
        !strcmp(projects[p].username, users[current_user_index].username) &&
        !strcmp(projects[p].status, "Available") && transaction_index(id) < 0;
    if (!allowed) printf("Only your available projects with no outstanding order can be changed.\n");
    return allowed;
}


int filter_projects(const char *project_type, float min_carbon_credit, float max_carbon_credit,
                    float min_price, float max_price, int min_duration, int max_duration)
{
    printf("\nSearching projects...\n");
    bool found = false;
    printf("Total projects loaded: %d\n", project_count);
    for (int i = 0; i < project_count; i++)
    {
        bool matches = true;
        if (strcmp(projects[i].status, "Available") != 0 || transaction_index(projects[i].project_id) >= 0)
            matches = false;
        if (strlen(project_type) > 0 && strstr(projects[i].project_type, project_type) == NULL)
        {
            matches = false;
        }
        if ((min_carbon_credit >= 0 && projects[i].carbon_credit < min_carbon_credit) ||
            (max_carbon_credit >= 0 && projects[i].carbon_credit > max_carbon_credit))
        {
            matches = false;
        }
        if ((min_price >= 0 && projects[i].price < min_price) ||
            (max_price >= 0 && projects[i].price > max_price))
        {
            matches = false;
        }
        if ((min_duration >= 0 && projects[i].project_duration < min_duration) ||
            (max_duration >= 0 && projects[i].project_duration > max_duration))
        {
            matches = false;
        }
        if (matches)
        {
            found = true;
            printf("----------------------------------------\n");
            printf("ID: %04d\n", projects[i].project_id);
            printf("Name: %s\n", projects[i].project_name);
            printf("Description: %s\n", projects[i].project_description);
            printf("Type: %s\n", projects[i].project_type);
            printf("Carbon Credit: %.2f tons of CO2\n", projects[i].carbon_credit);
            printf("Price: %.2f\n", projects[i].price);
            printf("Duration: %d years\n", projects[i].project_duration);
            printf("Impact: %s\n", projects[i].environmental_impact);
            printf("Documents: %s\n", projects[i].related_documents);
            printf("Contact: %s\n", projects[i].contact_info);
            printf("----------------------------------------\n\n");
        }
    }
    if (!found)
    {
        printf("No projects found that match the criteria.\n");
        getchar();
        return 0;
    }
    return 1;
}

int search_projects()
{
    char project_type[MAX_PROJECT_TYPE_LENGTH] = "";
    float min_carbon_credit = 0, max_carbon_credit = FLT_MAX;
    float min_price = 0, max_price = FLT_MAX;
    int min_duration = 0, max_duration = INT_MAX;
    int search_option;

    printf("Search Options:\n");
    printf("1. Project Type\n");
    printf("2. Carbon Credit Size (tons of CO2)\n");
    printf("3. Price\n");
    printf("4. Project Duration (years)\n");
    printf("5. Start Search\n");
    printf("6. Back to Main Menu\n");

    while (1)
    {
        printf("\nChoose the option you want to filter (1-6): ");
        read_number("%d", &search_option);

        if (search_option == 1)
        {

            printf("Select project type:\n");
            printf("1. Renewable Energy\n");
            printf("2. Carbon Sequestration\n");
            printf("3. Waste Management\n");
            printf("4. Forestry\n");
            printf("5. Other\n");
            printf("Choose: ");
            int type_choice;
            read_number("%d", &type_choice);

            if (type_choice == 1)
            {
                strcpy(project_type, "Renewable Energy");
            }
            else if (type_choice == 2)
            {
                strcpy(project_type, "Carbon Sequestration");
            }
            else if (type_choice == 3)
            {
                strcpy(project_type, "Waste Management");
            }
            else if (type_choice == 4)
            {
                strcpy(project_type, "Forestry");
            }
            else if (type_choice == 5)
            {
                printf("Enter custom project type: ");
                safe_input(project_type, sizeof(project_type));
                project_type[strcspn(project_type, "\n")] = '\0';
            }
            else
            {
                printf("Invalid choice. Please try again.\n");
                continue;
            }
        }
        else if (search_option == 2)
        {

            printf("Enter minimum carbon credit (or leave blank for no limit): ");
            char min_cc_input[50];
            safe_input(min_cc_input, sizeof(min_cc_input));
            if (strlen(min_cc_input) > 0)
                if (!parse_amount(min_cc_input, &min_carbon_credit)) printf("Invalid filter ignored.\n");

            printf("Enter maximum carbon credit (or leave blank for no limit): ");
            char max_cc_input[50];
            safe_input(max_cc_input, sizeof(max_cc_input));
            if (strlen(max_cc_input) > 0)
                if (!parse_amount(max_cc_input, &max_carbon_credit)) printf("Invalid filter ignored.\n");
        }
        else if (search_option == 3)
        {

            printf("Enter minimum price (or leave blank for no limit): ");
            char min_price_input[50];
            safe_input(min_price_input, sizeof(min_price_input));
            if (strlen(min_price_input) > 0)
                if (!parse_amount(min_price_input, &min_price)) printf("Invalid filter ignored.\n");

            printf("Enter maximum price (or leave blank for no limit): ");
            char max_price_input[50];
            safe_input(max_price_input, sizeof(max_price_input));
            if (strlen(max_price_input) > 0)
                if (!parse_amount(max_price_input, &max_price)) printf("Invalid filter ignored.\n");
        }
        else if (search_option == 4)
        {

            printf("Enter minimum duration (years, or leave blank for no limit): ");
            char min_duration_input[50];
            safe_input(min_duration_input, sizeof(min_duration_input));
            if (strlen(min_duration_input) > 0)
                if (!parse_int(min_duration_input, &min_duration)) printf("Invalid filter ignored.\n");

            printf("Enter maximum duration (years, or leave blank for no limit): ");
            char max_duration_input[50];
            safe_input(max_duration_input, sizeof(max_duration_input));
            if (strlen(max_duration_input) > 0)
                if (!parse_int(max_duration_input, &max_duration)) printf("Invalid filter ignored.\n");
        }
        else if (search_option == 5)
        {
            int y;
            y = filter_projects(project_type, min_carbon_credit, max_carbon_credit, min_price, max_price, min_duration, max_duration);
            if (y == 0)
            {
                return 7;
            }
            return 0;
        }
        else if (search_option == 6)
        {

            printf("Returning to Main Menu...\n");
            return 7;
        }
        else
        {

            printf("Invalid choice. Please try again.\n");
        }
    }
}

void edit_project()
{
    int type_choice;
    clear_screen();
    printf("=== Edit Project ===\n");
    bool has_projects = false;

    for (int i = 0; i < project_count; i++)
    {
        if (strcmp(projects[i].username, users[current_user_index].username) == 0)
        {
            has_projects = true;
            printf("\n=== Project ID: %04d ===\n", projects[i].project_id);
            printf("Name: %s\n", projects[i].project_name);
            printf("Description: %s\n", projects[i].project_description[0] ? projects[i].project_description : "N/A");
            printf("Type: %s\n", projects[i].project_type);
            printf("Carbon Credit: %.2f tons of CO2\n", projects[i].carbon_credit);
            printf("Price: %.2f\n", projects[i].price);
            printf("Duration: %d years\n", projects[i].project_duration);
            printf("Environmental Impact: %s\n", projects[i].environmental_impact[0] ? projects[i].environmental_impact : "N/A");
            printf("Related Documents: %s\n", projects[i].related_documents[0] ? projects[i].related_documents : "N/A");
            printf("Contact Information: %s\n", projects[i].contact_info[0] ? projects[i].contact_info : "N/A");
            printf("Status: %s\n", projects[i].status);
        }
    }

    if (!has_projects)
    {
        printf("You have no projects to edit. Press Enter to return to the previous menu.\n");
        getchar();
        return;
    }

    int project_id;
    printf("\nEnter the project ID to edit (or 0 to cancel): ");
    read_number("%d", &project_id);

    if (project_id == 0)
    {
        return;
    }

    // Check project edit permissions AFTER getting the project_id
    if (!can_edit_project(project_id))
    {
        getchar(); // Wait for user to press Enter
        return;
    }

    int index = -1;
    for (int i = 0; i < project_count; i++)
    {
        if (projects[i].project_id == project_id &&
            strcmp(projects[i].username, users[current_user_index].username) == 0)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        printf("Project not found or you do not have permission to edit this project.\n");
        getchar();
        return;
    }

    Project original_project = projects[index];

    while (1)
    {
        clear_screen();
        printf("=== Project Details Summary ===\n");
        printf("1. Name: %s\n", projects[index].project_name);
        printf("2. Description: %s\n", projects[index].project_description[0] ? projects[index].project_description : "N/A");
        printf("3. Type: %s\n", projects[index].project_type);
        printf("4. Carbon Credit: %.2f tons of CO2\n", projects[index].carbon_credit);
        printf("5. Price: %.2f\n", projects[index].price);
        printf("6. Duration: %d years\n", projects[index].project_duration);
        printf("7. Environmental Impact: %s\n", projects[index].environmental_impact[0] ? projects[index].environmental_impact : "N/A");
        printf("8. Related Documents: %s\n", projects[index].related_documents[0] ? projects[index].related_documents : "N/A");
        printf("9. Contact Information: %s\n", projects[index].contact_info);
        printf("10. Confirm and Save\n");
        printf("11. Cancel Editing\n");
        printf("\nChoose the detail you want to edit: ");
        int choice;
        read_number("%d", &choice);
        switch (choice)
        {
        case 1:
            printf("Enter new project name: ");
            safe_input(projects[index].project_name, sizeof(projects[index].project_name));
            break;
        case 2:
            printf("Enter new project description: ");
            safe_input(projects[index].project_description, sizeof(projects[index].project_description));
            break;
        case 3:
            do
            {
                printf("Select project type [required]:\n");
                printf("1. Renewable Energy\n");
                printf("2. Carbon Sequestration\n");
                printf("3. Waste Management\n");
                printf("4. Forestry\n");
                printf("5. Other\n");
                printf("Choose: ");
                read_number("%d", &type_choice);

                if (type_choice == 1)
                {
                    strcpy(projects[index].project_type, "Renewable Energy");
                    break;
                }
                else if (type_choice == 2)
                {
                    strcpy(projects[index].project_type, "Carbon Sequestration");
                    break;
                }
                else if (type_choice == 3)
                {
                    strcpy(projects[index].project_type, "Waste Management");
                    break;
                }
                else if (type_choice == 4)
                {
                    strcpy(projects[index].project_type, "Forestry");
                    break;
                }
                else if (type_choice == 5)
                {
                    printf("Enter custom project type [required]: ");
                    safe_input(projects[index].project_type, sizeof(projects[index].project_type));
                    break;
                }
                else
                {
                    printf("Invalid choice. Please choose again.\n");
                    getchar();
                }

            } while (1);

            break;
        case 4:
            printf("New carbon credit size (tons of CO2): ");
            read_number("%f", &projects[index].carbon_credit);
            break;

        case 5:
            printf("Enter new price: ");
            read_number("%f", &projects[index].price);
            break;
        case 6:
            printf("Enter new project duration (years): ");
            read_number("%d", &projects[index].project_duration);
            break;
        case 7:
            printf("Enter new environmental impact: ");
            safe_input(projects[index].environmental_impact, sizeof(projects[index].environmental_impact));
            break;
        case 8:
            printf("Enter new related documents: ");
            safe_input(projects[index].related_documents, sizeof(projects[index].related_documents));
            break;
        case 9:
            printf("Enter new contact information: ");
            safe_input(projects[index].contact_info, sizeof(projects[index].contact_info));
            break;
        case 10:
        {
            float delta = projects[index].carbon_credit - original_project.carbon_credit;
            if (!project_valid(&projects[index]) || delta > users[current_user_index].carbon_credit ||
                !amount_valid(users[current_user_index].carbon_credit - delta))
            {
                printf("Check required fields, positive amounts/duration and your unlisted credit balance.\n");
                break;
            }
            users[current_user_index].carbon_credit -= delta;
            save_state();
            printf("Project updated successfully.\n");
            return;
        }
        case 11:

            projects[index] = original_project;
            printf("Editing cancelled. All changes reverted.\n");
            return;
        default:
            printf("Invalid choice. Please try again.\n");
            getchar();
            break;
        }
    }
}

int user_login()
{
    if (user_count == 0)
    {
        printf("No accounts yet. Register a sample account first. Press Enter to return.\n");
        getchar();
        return -1;
    }
    char username[MAX_USERNAME_LENGTH], password[MAX_PASSWORD_LENGTH];
    while (1)
    {
        clear_screen();
        printf("*** Login ***\n");
        printf("Enter 'back' to return to the main menu.\n");
        printf("Username: ");
        safe_input(username, sizeof(username));
        if (strcmp(username, "back") == 0)
        {
            clear_screen();
            return -1;
        }
        printf("Type 'forget' if you forget the password.\n");
        printf("Password: ");
        get_password(password, sizeof(password));
        if (strcmp(password, "back") == 0)
        {
            clear_screen();
            return -1;
        }
        if (strcmp(password, "forget") == 0)
        {
            user_forget_password(username);
            continue;
        }
        for (int i = 0; i < user_count; i++)
        {
            if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0)
            {
                current_user_index = i;
                printf("Login successful! Welcome, %s.\n", username);
                return 0;
            }
        }
        printf("Invalid username or password. Please try again.\n");
        getchar();
    }
}

int user_forget_password(char *username)
{
    int choose;
    int incorrect = 0;
    char answer[MAX_ANSWER_LENGTH];
    clear_screen();
    printf("Please choose a security question\n");
    printf("1: What was the name of your elementary school?\n");
    printf("2: What is the name of the hospital where you were born?\n");
    printf("3: What was the name of your first pet?\n");
    printf("Choose: ");
    read_number("%d", &choose);
    for (int i = 0; i < user_count; i++)
    {
        if (strcmp(username, users[i].username) == 0)
        {
            while (incorrect < 3)
            {
                switch (choose)
                {
                case 1:
                    printf("What was the name of your elementary school?\nAns: ");
                    safe_input(answer, sizeof(answer));
                    if (strcmp(answer, users[i].answer_security_question1) == 0)
                    {
                        printf("Correct answer. Proceeding to reset password.\n");
                        reset_password(username, user_count);
                        getchar();
                        return 0;
                    }
                    break;
                case 2:
                    printf("What is the name of the hospital where you were born?\nAns: ");
                    safe_input(answer, sizeof(answer));
                    if (strcmp(answer, users[i].answer_security_question2) == 0)
                    {
                        printf("Correct answer. Proceeding to reset password.\n");
                        reset_password(username, user_count);
                        return 0;
                    }
                    break;
                case 3:
                    printf("What was the name of your first pet?\nAns: ");
                    safe_input(answer, sizeof(answer));
                    if (strcmp(answer, users[i].answer_security_question3) == 0)
                    {
                        printf("Correct answer. Proceeding to reset password.\n");
                        reset_password(username, user_count);
                        return 0;
                    }
                    break;
                default:
                    printf("Invalid choice.\n");
                    getchar();
                    return -1;
                }
                printf("Incorrect answer. Please try again.\n");
                incorrect++;
                if (incorrect == 3)
                {
                    printf("Too many incorrect attempts. Please contact support.\n");
                    return -1;
                }
            }
        }
    }
    printf("Username not found.\n");
    getchar();
    return -1;
}

int user_register()
{
    char username[MAX_USERNAME_LENGTH], password[MAX_PASSWORD_LENGTH],
        confirm_password[MAX_PASSWORD_LENGTH],
        answer1[MAX_ANSWER_LENGTH],
        answer2[MAX_ANSWER_LENGTH],
        answer3[MAX_ANSWER_LENGTH],
        email[MAX_EMAIL_LENGTH]; // New field for email
    float carbon_credit = 0.0, balance = 0.0;

    while (1)
    {
        clear_screen();
        printf("*** Register ***\n");
        printf("Enter 'back' to return to the main menu.\n");

        carbon_credit = balance = 0;
        printf("Use sample details. Account fields use one word; underscores may replace spaces.\n");

        // Username input
        printf("Username: ");
        safe_input(username, sizeof(username));
        if (strcmp(username, "back") == 0)
            return -1;

        // Check username uniqueness
        bool username_exists = false;
        for (int i = 0; i < user_count; i++)
        {
            if (strcmp(users[i].username, username) == 0)
            {
                printf("Username already exists. Please try a different one.");
                getchar();
                username_exists = true;
                break;
            }
        }
        if (username_exists)
            continue;

        // Password input with confirmation
        while (1)
        {
            printf("Password: ");
            get_password(password, sizeof(password));
            if (strcmp(password, "back") == 0)
                return -1;

            printf("Confirm Password: ");
            get_password(confirm_password, sizeof(confirm_password));
            if (strcmp(confirm_password, "back") == 0)
                return -1;

            if (strcmp(password, confirm_password) != 0)
            {
                clear_screen();
                printf("Passwords do not match. Please try again.\n");
                continue;
            }
            break;
        }

        // Security Questions
        printf("Security Questions:\n");
        printf("Q1: What was the name of your elementary school?\nAns: ");
        safe_input(answer1, sizeof(answer1));
        printf("Q2: What is the name of the hospital where you were born? \nAns: ");
        safe_input(answer2, sizeof(answer2));
        printf("Q3: What was the name of your first pet? \nAns: ");
        safe_input(answer3, sizeof(answer3));

        // Email Input with Validation
        while (1)
        {
            printf("Email Address: ");
            safe_input(email, sizeof(email));

            // Basic email validation
            if (strchr(email, '@') == NULL || strchr(email, '.') == NULL)
            {
                printf("Invalid email address. Please try again.\n");
                continue;
            }

            break;
        }

        // Existing initial credit and balance input
        printf("Enter initial carbon credits (or press enter to set to 0): ");
        char carbon_credit_input[50];
        safe_input(carbon_credit_input, sizeof(carbon_credit_input));
        if (strlen(carbon_credit_input) > 0)
        {
            if (!parse_amount(carbon_credit_input, &carbon_credit)) { printf("Invalid credits; using zero.\n"); carbon_credit = 0; }
        }

        printf("Enter initial balance (or press enter to set to 0): ");
        char balance_input[50];
        safe_input(balance_input, sizeof(balance_input));
        if (strlen(balance_input) > 0)
        {
            if (!parse_amount(balance_input, &balance)) { printf("Invalid balance; using zero.\n"); balance = 0; }
        }

        // Confirmation and editing loop
        while (1)
        {
            clear_screen();
            printf("Confirm your registration details:\n");
            printf("1. Username: %s\n", username);
            printf("2. Password: ********\n");
            printf("3. Security Question 1 Answer: %s\n", answer1);
            printf("4. Security Question 2 Answer: %s\n", answer2);
            printf("5. Security Question 3 Answer: %s\n", answer3);
            printf("6. Email: %s\n", email);
            printf("7. Carbon Credit: %.2f\n", carbon_credit);
            printf("8. Balance: %.2f\n", balance);
            printf("\nDo you want to make any changes?\n");
            printf("Enter the number to edit, or 0 to confirm and register: ");

            int choice;
            read_number("%d", &choice);

            if (choice == 0)
                break;

            switch (choice)
            {
            case 1:
                printf("Enter new username: ");
                safe_input(username, sizeof(username));
                break;
            case 2:
                while (1)
                {
                    printf("Enter new password: ");
                    get_password(password, sizeof(password));
                    printf("Confirm new password: ");
                    get_password(confirm_password, sizeof(confirm_password));

                    if (strcmp(password, confirm_password) == 0)
                        break;
                    else
                    {
                        printf("Passwords do not match. Try again.\n");
                        getchar();
                    }
                }
                break;
            case 3:
                printf("Enter new answer for security question 1: ");
                safe_input(answer1, sizeof(answer1));
                break;
            case 4:
                printf("Enter new answer for security question 2: ");
                safe_input(answer2, sizeof(answer2));
                break;
            case 5:
                printf("Enter new answer for security question 3: ");
                safe_input(answer3, sizeof(answer3));
                break;
            case 6:
                printf("Enter new email: ");
                safe_input(email, sizeof(email));
                break;
            case 7:
                printf("Enter new carbon credits: ");
                read_number("%f", &carbon_credit);
                break;
            case 8:
                printf("Enter new balance: ");
                read_number("%f", &balance);
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                getchar();
                break;
            }
        }

        User candidate = {0};
        strcpy(candidate.username, username);
        strcpy(candidate.password, password);
        strcpy(candidate.answer_security_question1, answer1);
        strcpy(candidate.answer_security_question2, answer2);
        strcpy(candidate.answer_security_question3, answer3);
        strcpy(candidate.email, email);
        candidate.carbon_credit = carbon_credit;
        candidate.balance = balance;
        if (!account_valid(&candidate) || user_index(username) >= 0)
        {
            printf("Registration not saved. Use a unique username (letters, numbers, _, . or -), single-word fields and a valid email.\n");
            continue;
        }
        users = grow_array(users, &user_capacity, sizeof(User), user_count + 1);

        // Save new user details
        User *new_user = &users[user_count];
        strcpy(new_user->username, username);
        strcpy(new_user->password, password);
        strcpy(new_user->answer_security_question1, answer1);
        strcpy(new_user->answer_security_question2, answer2);
        strcpy(new_user->answer_security_question3, answer3);
        strcpy(new_user->email, email); // Save email
        new_user->carbon_credit = carbon_credit;
        new_user->balance = balance;

        user_count++;
        save_user_to_file();
        printf("Registration successful! Press Enter to continue...\n");
        getchar();
        return 0;
    }
}

int buyer_menu()
{
    char project_id_input[10];
    int search;
    clear_screen();
    printf("=== Buyer Menu ===\n");

    search = search_projects();

    while (1)
    {
        if (search == 7)
            break;
        printf("\nEnter the project ID to purchase (or 0 to cancel): ");
        safe_input(project_id_input, sizeof(project_id_input));

        if (strcmp(project_id_input, "0") == 0)
        {
            printf("Cancelled. Returning to search menu.\n");
            return 0;
        }

        int parsed_id;
        if (parse_int(project_id_input, &parsed_id) && parsed_id > 0)
        {
            int project_id = parsed_id;
            bool project_found = false;

            for (int i = 0; i < project_count; i++)
            {
                if (projects[i].project_id == project_id)
                {
                    project_found = true;

                    if (strcmp(projects[i].username, users[current_user_index].username) == 0)
                    {

                        printf("Error: You cannot purchase your own project. Please choose another project.\n");
                        break;
                    }
                    else
                    {

                        buy_from_filtered(project_id_input);
                        getchar();
                        return 0;
                    }
                }
            }

            if (!project_found)
            {
                printf("Project ID not found. Please enter a valid project ID.\n");
            }
        }
        else
        {

            printf("Invalid ID format. Enter a positive project ID.\n");
            getchar();
        }
    }
    return 0;
}

void seller_menu()
{
    int choice;
    do
    {
        clear_screen();
        printf("=== Seller Menu ===\n");
        printf("1. Add New Project\n");
        printf("2. Edit Existing Project\n");
        printf("3. Delete Project\n");
        printf("4. Back to Main Menu\n");
        printf("====================\n");
        printf("Choose an option: ");
        read_number("%d", &choice);
        switch (choice)
        {
        case 1:
            add_new_project();
            getchar();
            break;
        case 2:
            edit_project();
            break;
        case 3:
            delete_project();
            break;
        case 4:
            printf("Returning to Main Menu...\n");
            break;
        default:
            printf("Invalid choice. Press enter to try again.\n");
            getchar();
            break;
        }
    } while (choice != 4);
}


int reset_password(char *username, int count)
{
    (void)count;
    int index = user_index(username);
    if (index < 0) return -1;
    char password[MAX_PASSWORD_LENGTH], confirm[MAX_PASSWORD_LENGTH];
    for (;;)
    {
        printf("New password (one word, or back to cancel): ");
        get_password(password, sizeof(password));
        if (!strcmp(password, "back")) return -1;
        printf("Confirm password: ");
        get_password(confirm, sizeof(confirm));
        if (!token_valid(password) || strcmp(password, confirm))
        { printf("Use matching, non-empty passwords without spaces.\n"); continue; }
        strcpy(users[index].password, password);
        save_user_to_file();
        printf("Password updated.\n");
        return 0;
    }
}

void main_menu(void)
{
    for (;;)
    {
        printf("\n=== Account: %s | Credits: %.2f | Demo balance: %.2f ===\n",
            users[current_user_index].username, users[current_user_index].carbon_credit,
            users[current_user_index].balance);
        printf("1. Buyer\n2. Seller\n3. Order status\n4. Edit profile / demo balances\n5. Logout\nChoose: ");
        int choice;
        read_number("%d", &choice);
        switch (choice)
        {
        case 1: buyer_menu(); break;
        case 2: seller_menu(); break;
        case 3: check_order_status(); break;
        case 4: edit_profile(); break;
        case 5: current_user_index = -1; return;
        default: printf("Choose a number from 1 to 5.\n"); break;
        }
    }
}

void check_order_status(void)
{
    const char *name = users[current_user_index].username;
    bool found = false;
    for (int i = 0; i < tTransaction_count; ++i)
    {
        Transaction *t = &tTransactions[i];
        if (strcmp(name, t->tBuyer_name) && strcmp(name, t->tSeller_name)) continue;
        printf("%04d | %s | Seller: %s | Buyer: %s | %s\n", t->tProject_id,
            t->tProject_name, t->tSeller_name, t->tBuyer_name, t->tStatus);
        found = true;
    }
    if (!found) { printf("No outstanding orders.\n"); return; }
    int id, action;
    printf("Project ID (0 to return): ");
    read_number("%d", &id);
    if (!id) return;
    printf("1. Confirm as seller\n2. Cancel pending order\n3. Acknowledge completed/cancelled order as buyer\n0. Back\nChoose: ");
    read_number("%d", &action);
    if (!action) return;
    if (resolve_order(id, current_user_index, action)) printf("Order updated.\n");
    else printf("Action is not allowed for this account or order status.\n");
}

void buy_from_filtered(char *project_id_input)
{
    int id;
    if (!parse_int(project_id_input, &id)) return;
    int p = project_index(id);
    if (p < 0) { printf("Project not found.\n"); return; }
    printf("Request %s for %.2f demo currency? (1 = yes, 0 = cancel): ", projects[p].project_name, projects[p].price);
    int confirm;
    read_number("%d", &confirm);
    if (confirm != 1) return;
    if (request_purchase(id, current_user_index)) printf("Purchase requested. Funds reserved until the seller confirms or the order is cancelled.\n");
    else printf("Cannot purchase: check balance, ownership, availability and outstanding orders.\n");
}

void edit_profile(void)
{
    User edited = users[current_user_index];
    for (;;)
    {
        printf("\n1. Password\n2. Security answer 1\n3. Security answer 2\n4. Security answer 3\n5. Email\n6. Demo carbon credits\n7. Demo balance\n8. Save\n9. Cancel\nChoose: ");
        int choice;
        read_number("%d", &choice);
        switch (choice)
        {
        case 1: printf("New password: "); get_password(edited.password, sizeof(edited.password)); break;
        case 2: printf("New answer (one word): "); safe_input(edited.answer_security_question1, sizeof(edited.answer_security_question1)); break;
        case 3: printf("New answer (one word): "); safe_input(edited.answer_security_question2, sizeof(edited.answer_security_question2)); break;
        case 4: printf("New answer (one word): "); safe_input(edited.answer_security_question3, sizeof(edited.answer_security_question3)); break;
        case 5: printf("Email: "); safe_input(edited.email, sizeof(edited.email)); break;
        case 6: printf("Unlisted demo carbon credits: "); read_number("%f", &edited.carbon_credit); break;
        case 7: printf("Demo balance: "); read_number("%f", &edited.balance); break;
        case 8:
            if (!account_valid(&edited)) { printf("Use non-empty, single-word account fields and a valid email.\n"); break; }
            /* Pending purchases must remain refundable within the demo balance limit. */
            for (int i = 0; i < tTransaction_count; ++i)
                if (!strcmp(tTransactions[i].tStatus, "Pending") &&
                    !strcmp(tTransactions[i].tBuyer_name, edited.username) &&
                    edited.balance != users[current_user_index].balance)
                { printf("Resolve pending purchases before changing your demo balance.\n"); return; }
            users[current_user_index] = edited;
            save_user_to_file();
            printf("Profile saved. Usernames stay fixed so project and order ownership remains consistent.\n");
            return;
        case 9: printf("Changes cancelled.\n"); return;
        default: printf("Choose a number from 1 to 9.\n"); break;
        }
    }
}

static float prompt_amount(const char *prompt, bool positive)
{
    float result;
    do { printf("%s", prompt); read_number("%f", &result); }
    while (positive && result < 0.01f);
    return result;
}

void add_new_project(void)
{
    Project p = {0};
    printf("Project name [required]: "); safe_input(p.project_name, sizeof(p.project_name));
    printf("Description [optional]: "); safe_input(p.project_description, sizeof(p.project_description));
    int choice;
    for (;;)
    {
        printf("1. Renewable Energy\n2. Carbon Sequestration\n3. Waste Management\n4. Forestry\n5. Other\nProject type: ");
        read_number("%d", &choice);
        const char *types[] = {"Renewable Energy", "Carbon Sequestration", "Waste Management", "Forestry"};
        if (choice >= 1 && choice <= 4) { strcpy(p.project_type, types[choice - 1]); break; }
        if (choice == 5) { printf("Custom type: "); safe_input(p.project_type, sizeof(p.project_type)); break; }
    }
    p.carbon_credit = prompt_amount("Carbon credits (tons of CO2): ", true);
    p.price = prompt_amount("Total project price (demo currency): ", true);
    printf("Duration (years): "); read_number("%d", &p.project_duration);
    printf("Environmental impact [optional]: "); safe_input(p.environmental_impact, sizeof(p.environmental_impact));
    printf("Related documents (reference/link only): "); safe_input(p.related_documents, sizeof(p.related_documents));
    strcpy(p.contact_info, users[current_user_index].email);
    strcpy(p.username, users[current_user_index].username);
    strcpy(p.status, "Available");
    if (!project_valid(&p) || users[current_user_index].carbon_credit < p.carbon_credit || project_id_counter == INT_MAX)
    { printf("Project not saved. Check required fields, positive duration and unlisted credit balance.\n"); return; }
    printf("List %s: %.2f credits for %.2f total? (1 = save, 0 = cancel): ", p.project_name, p.carbon_credit, p.price);
    read_number("%d", &choice);
    if (choice != 1) return;
    p.project_id = project_id_counter++;
    projects = grow_array(projects, &project_capacity, sizeof(Project), project_count + 1);
    projects[project_count++] = p;
    users[current_user_index].carbon_credit -= p.carbon_credit;
    save_state();
    printf("Project %04d saved. Listed credits are reserved from your demo inventory.\n", p.project_id);
}

void delete_project(void)
{
    for (int i = 0; i < project_count; ++i)
        if (!strcmp(projects[i].username, users[current_user_index].username))
            printf("%04d | %s | %s\n", projects[i].project_id, projects[i].project_name, projects[i].status);
    int id;
    printf("Project ID to delete (0 to cancel): "); read_number("%d", &id);
    if (!id || !can_edit_project(id)) return;
    int p = project_index(id);
    if (!amount_valid(users[current_user_index].carbon_credit + projects[p].carbon_credit))
    { printf("Credit balance would exceed the demo limit.\n"); return; }
    printf("Delete this project and return reserved credits? (1 = yes, 0 = cancel): ");
    int confirm; read_number("%d", &confirm);
    if (confirm != 1) return;
    users[current_user_index].carbon_credit += projects[p].carbon_credit;
    for (int i = p; i + 1 < project_count; ++i) projects[i] = projects[i + 1];
    --project_count;
    save_state();
    printf("Project deleted; reserved credits returned.\n");
}

int main(void)
{
    load_users_from_file();
    load_projects_from_file();
    load_transactions();
    validate_pending_projects();
    for (;;)
    {
        printf("\n=== Carbon Credit Trading System (academic demo) ===\n1. Login\n2. Register\n3. Exit\nChoose: ");
        int choice;
        read_number("%d", &choice);
        switch (choice)
        {
        case 1: if (user_login() == 0) main_menu(); break;
        case 2: user_register(); break;
        case 3: free(users); free(projects); return 0;
        default: printf("Choose a number from 1 to 3.\n"); break;
        }
    }
}
