// ST5004CEM - Task 3: File System Operations and Security
// secure_fs.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_USERS      10
#define MAX_FILES      20
#define MAX_NAME       64
#define MAX_CONTENT    1024
#define AUDIT_LOG_PATH "audit.log"

// ---------- Users & Auth ----------

typedef struct {
    char username[MAX_NAME];
    unsigned long pass_hash;
    unsigned long salt;
} user_t;

// ---------- Permissions ----------

typedef struct {
    int owner_r, owner_w, owner_x;
    int group_r, group_w, group_x;
    int other_r, other_w, other_x;
} permissions_t;

// ---------- Files ----------

typedef struct {
    char name[MAX_NAME];
    char owner[MAX_NAME];
    unsigned char content[MAX_CONTENT];
    int  length;
    permissions_t perms;
    int  in_use;
} file_t;

static user_t users[MAX_USERS];
static int    user_count = 0;

static file_t files[MAX_FILES];
static int    file_count = 0;

// ---------- Audit Logging ----------

static void audit_log(const char *user, const char *action,
                       const char *target, int success) {
    FILE *f = fopen(AUDIT_LOG_PATH, "a");
    if (!f) return;
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(f, "[%s] user=%-10s action=%-10s target=%-20s result=%s\n",
            ts, user, action, target, success ? "SUCCESS" : "DENIED");
    fclose(f);
}

// ---------- Password Hashing ----------

static unsigned long hash_password(const char *password, unsigned long salt) {
    unsigned long hash = salt;
    int c;
    while ((c = *password++))
        hash = ((hash << 5) + hash) + (unsigned long)c;
    return hash;
}

static int create_user(const char *username, const char *password) {
    if (user_count >= MAX_USERS) return -1;
    for (int i = 0; i < user_count; i++)
        if (strcmp(users[i].username, username) == 0) return -1;

    user_t *u = &users[user_count++];
    strncpy(u->username, username, MAX_NAME - 1);
    u->salt = (unsigned long)rand();
    u->pass_hash = hash_password(password, u->salt);
    return 0;
}

// ---------- Authentication ----------

static int authenticate(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            int ok = (hash_password(password, users[i].salt) == users[i].pass_hash);
            audit_log(username, "LOGIN", "-", ok);
            return ok;
        }
    }
    audit_log(username, "LOGIN", "-", 0);
    return 0;
}

// ---------- Permission Checks ----------

static int check_permission(file_t *f, const char *user, char op) {
    int is_owner = (strcmp(f->owner, user) == 0);
    int allowed;
    if (is_owner) {
        allowed = (op == 'r') ? f->perms.owner_r :
                  (op == 'w') ? f->perms.owner_w : f->perms.owner_x;
    } else {
        allowed = (op == 'r') ? f->perms.other_r :
                  (op == 'w') ? f->perms.other_w : f->perms.other_x;
    }
    return allowed;
}

static file_t *find_file(const char *name) {
    for (int i = 0; i < file_count; i++)
        if (files[i].in_use && strcmp(files[i].name, name) == 0)
            return &files[i];
    return NULL;
}

// ---------- File Operations ----------

static int fs_create(const char *user, const char *name, const char *content) {
    if (find_file(name) != NULL) {
        audit_log(user, "CREATE", name, 0);
        return -1;
    }
    if (file_count >= MAX_FILES) return -1;

    file_t *f = &files[file_count++];
    strncpy(f->name, name, MAX_NAME - 1);
    strncpy(f->owner, user, MAX_NAME - 1);
    f->length = (int)strlen(content);
    if (f->length >= MAX_CONTENT) f->length = MAX_CONTENT - 1;
    memcpy(f->content, content, f->length);

    f->perms = (permissions_t){1,1,0, 1,0,0, 1,0,0};
    f->in_use = 1;

    audit_log(user, "CREATE", name, 1);
    return 0;
}

static int fs_read(const char *user, const char *name,
                    unsigned char *out_buf, int *out_len) {
    file_t *f = find_file(name);
    if (!f) { audit_log(user, "READ", name, 0); return -1; }
    if (!check_permission(f, user, 'r')) { audit_log(user, "READ", name, 0); return -2; }

    memcpy(out_buf, f->content, f->length);
    *out_len = f->length;
    audit_log(user, "READ", name, 1);
    return 0;
}

static int fs_write(const char *user, const char *name, const char *new_content) {
    file_t *f = find_file(name);
    if (!f) { audit_log(user, "WRITE", name, 0); return -1; }
    if (!check_permission(f, user, 'w')) { audit_log(user, "WRITE", name, 0); return -2; }

    f->length = (int)strlen(new_content);
    if (f->length >= MAX_CONTENT) f->length = MAX_CONTENT - 1;
    memcpy(f->content, new_content, f->length);

    audit_log(user, "WRITE", name, 1);
    return 0;
}

static int fs_delete(const char *user, const char *name) {
    file_t *f = find_file(name);
    if (!f) { audit_log(user, "DELETE", name, 0); return -1; }
    if (strcmp(f->owner, user) != 0) { audit_log(user, "DELETE", name, 0); return -2; }
    f->in_use = 0;
    audit_log(user, "DELETE", name, 1);
    return 0;
}

// ---------- Main ----------

int main(void) {
    srand((unsigned)time(NULL));
    remove(AUDIT_LOG_PATH);

    printf("=== User setup ===\n");
    create_user("alice", "AlicePass123");
    create_user("bob",   "BobSecret456");
    printf("Created users: alice, bob\n");

    printf("\n=== Authentication ===\n");
    printf("alice correct password : %s\n", authenticate("alice", "AlicePass123") ? "OK" : "DENIED");
    printf("alice wrong password   : %s\n", authenticate("alice", "wrongpass")    ? "OK" : "DENIED");
    printf("bob correct password   : %s\n", authenticate("bob",   "BobSecret456") ? "OK" : "DENIED");

    printf("\n=== File operations (owner: alice) ===\n");
    fs_create("alice", "secret.txt", "Top secret exam answers");
    printf("alice created secret.txt\n");

    unsigned char buf[MAX_CONTENT];
    int len;
    if (fs_read("alice", "secret.txt", buf, &len) == 0) {
        buf[len] = '\0';
        printf("alice reads secret.txt -> \"%s\"\n", buf);
    }

    printf("\n=== Permission enforcement (bob tries to write alice's file) ===\n");
    int rc = fs_write("bob", "secret.txt", "hacked!");
    printf("bob write attempt result code: %d\n", rc);

    printf("\n=== bob reads (others have read permission by default) ===\n");
    if (fs_read("bob", "secret.txt", buf, &len) == 0) {
        buf[len] = '\0';
        printf("bob reads secret.txt -> \"%s\"\n", buf);
    }

    printf("\n=== Delete (only owner allowed) ===\n");
    printf("bob delete attempt: rc=%d\n", fs_delete("bob", "secret.txt"));
    printf("alice delete attempt: rc=%d\n", fs_delete("alice", "secret.txt"));

    printf("\n=== Audit log contents ===\n");
    FILE *log = fopen(AUDIT_LOG_PATH, "r");
    if (log) {
        char line[256];
        while (fgets(line, sizeof(line), log)) printf("%s", line);
        fclose(log);
    }

    return 0;
}
