/* Regression checks use only synthetic data in the runner's temporary directory. */
#define main carbon_credit_app_main
#include "../login.c"
#undef main
#include <assert.h>

static void close_to(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.001f);
}

static void seed(void)
{
    free(users);
    free(projects);
    users = NULL;
    projects = NULL;
    user_capacity = project_capacity = 0;
    user_count = 3;
    project_count = 1;
    tTransaction_count = 0;
    users = grow_array(users, &user_capacity, sizeof(User), user_count);
    projects = grow_array(projects, &project_capacity, sizeof(Project), project_count);
    memset(users, 0, (size_t)user_capacity * sizeof(User));
    memset(projects, 0, (size_t)project_capacity * sizeof(Project));
    const char *names[] = {"seller", "buyer", "observer"};
    for (int i = 0; i < user_count; ++i)
    {
        strcpy(users[i].username, names[i]);
        strcpy(users[i].password, "demo123");
        strcpy(users[i].answer_security_question1, "school");
        strcpy(users[i].answer_security_question2, "hospital");
        strcpy(users[i].answer_security_question3, "pet");
        strcpy(users[i].email, "demo@example.test");
        users[i].balance = 1000;
        assert(account_valid(&users[i]));
    }
    /* The listing has already reserved 30 of the seller's original 100 credits. */
    users[0].carbon_credit = 70;
    Project *p = &projects[0];
    p->project_id = 1;
    strcpy(p->project_name, "Demo, Forest");
    strcpy(p->project_type, "Forestry");
    strcpy(p->related_documents, "example.test/demo");
    strcpy(p->contact_info, "seller@example.test");
    strcpy(p->username, "seller");
    strcpy(p->status, "Available");
    p->carbon_credit = 30;
    p->price = 200;
    p->project_duration = 2;
    current_user_index = 0;
    project_id_counter = 2;
    save_state();
}

static void reload(void)
{
    load_users_from_file();
    load_projects_from_file();
    load_transactions();
    validate_pending_projects();
}

int main(void)
{
    seed();
    assert(!request_purchase(1, 0)); /* no self purchase */
    assert(!request_purchase(99, 1));
    assert(!request_purchase(1, -1));
    users[1].balance = 199;
    assert(!request_purchase(1, 1));
    close_to(users[1].balance, 199);
    assert(tTransaction_count == 0);
    users[1].balance = 1000;
    assert(request_purchase(1, 1));
    close_to(users[1].balance, 800);
    assert(!request_purchase(1, 2));
    assert(!resolve_order(1, 2, 1));
    assert(!resolve_order(1, 2, 2));
    assert(!resolve_order(1, 1, 1));
    assert(!resolve_order(1, 1, 3));
    assert(!can_edit_project(1));

    reload();
    load_transactions();
    load_transactions();
    assert(tTransaction_count == 1);
    assert(!strcmp(tTransactions[0].tProject_name, "Demo, Forest"));
    assert(resolve_order(1, 0, 1));
    close_to(users[0].balance, 1200);
    close_to(users[1].carbon_credit, 30);
    assert(!resolve_order(1, 0, 1)); /* exactly one settlement */
    assert(!resolve_order(1, 1, 2));
    assert(!resolve_order(1, 2, 3));
    reload();
    close_to(users[0].balance, 1200);
    close_to(users[1].carbon_credit, 30);
    assert(resolve_order(1, 1, 3));
    reload();
    assert(tTransaction_count == 0);
    assert(!strcmp(projects[0].status, "Complete"));
    assert(!request_purchase(1, 1));
    assert(!can_edit_project(1));
    puts("PASS: purchase, authorization, restart and exactly-once confirmation");

    seed();
    assert(request_purchase(1, 1));
    assert(resolve_order(1, 0, 2));
    close_to(users[1].balance, 1000);
    close_to(users[0].carbon_credit, 70); /* credits remain reserved for the listing */
    assert(!resolve_order(1, 0, 2));
    assert(!request_purchase(1, 2)); /* wait for buyer acknowledgement */
    assert(!can_edit_project(1));
    reload();
    assert(resolve_order(1, 1, 3));
    assert(!resolve_order(1, 1, 3));
    assert(can_edit_project(1));
    assert(request_purchase(1, 2));
    assert(resolve_order(1, 2, 2)); /* buyer cancellation removes own pending order */
    assert(tTransaction_count == 0);
    close_to(users[2].balance, 1000);
    assert(!resolve_order(1, 2, 2));
    reload();
    assert(tTransaction_count == 0);
    assert(!strcmp(projects[0].status, "Available"));
    puts("PASS: seller cancellation, acknowledgement, repurchase and buyer cancellation");

    seed();
    tTransaction_count = MAX_TRANSACTIONS;
    assert(!request_purchase(1, 1));
    close_to(users[1].balance, 1000);
    tTransaction_count = 0;
    assert(request_purchase(1, 1));
    users[0].balance = MAX_AMOUNT;
    assert(!resolve_order(1, 0, 1));
    close_to(users[1].carbon_credit, 0);
    assert(!strcmp(tTransactions[0].tStatus, "Pending"));
    assert(resolve_order(1, 1, 2));
    puts("PASS: transaction capacity and balance limits reject without partial changes");

    int integer = 0;
    float amount = 0;
    assert(!parse_int("12junk", &integer));
    assert(!parse_int("-1", &integer));
    assert(!parse_int("999999999999999999999999999", &integer));
    assert(!parse_amount("NaN", &amount));
    assert(!parse_amount("inf", &amount));
    assert(!parse_amount("-10", &amount));
    assert(!parse_amount("10abc", &amount));
    assert(parse_amount("12.345", &amount));
    close_to(amount, 12.35f);
    assert(!username_valid("two words"));
    assert(!token_valid("two words"));
    char fields[3][513];
    assert(csv_fields("1,\"forest, demo\",\"\"", fields, 3));
    assert(!strcmp(fields[1], "forest, demo") && !*fields[2]);
    assert(!csv_fields("1,\"unclosed,field", fields, 3));
    assert(!csv_fields("1,2,3,4", fields, 3));
    puts("PASS: numeric validation and bounded CSV parsing");

    seed();
    for (int i = 1; i < MAX_TRANSACTIONS; ++i)
    {
        projects = grow_array(projects, &project_capacity, sizeof(Project), i + 1);
        projects[i] = projects[0];
        projects[i].project_id = i + 1;
        projects[i].price = 1;
        strcpy(projects[i].status, "Pending");
    }
    project_count = MAX_TRANSACTIONS;
    strcpy(projects[0].status, "Pending");
    for (int i = 0; i < MAX_TRANSACTIONS; ++i)
    {
        Transaction *t = &tTransactions[i];
        memset(t, 0, sizeof(*t));
        t->tProject_id = i + 1;
        strcpy(t->tProject_name, projects[i].project_name);
        strcpy(t->tSeller_name, "seller");
        strcpy(t->tBuyer_name, "buyer");
        strcpy(t->tStatus, "Pending");
    }
    tTransaction_count = MAX_TRANSACTIONS;
    save_state();
    reload();
    reload();
    assert(project_count == MAX_TRANSACTIONS && tTransaction_count == MAX_TRANSACTIONS);
    puts("PASS: full transaction array and repeated reloads remain bounded");
    free(users);
    free(projects);
    return 0;
}
