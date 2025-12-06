#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#define STUDENT_FILE    "student.txt"
#define CREDENTIAL_FILE "credential.txt"
#define LOG_FILE        "logs.txt"
#define MAX_USERS     50
#define MAX_STUDENTS  200
#define MAX_SUBJECTS  10
#define NAME_LEN      50
#define SUBJ_LEN      30
#define ROLE_LEN      12
typedef struct {
    char username[NAME_LEN];
    char password[NAME_LEN]; /* Note: for production, store hashes not plain text */
    char role[ROLE_LEN];
    char lastLogin[20];
} User;
typedef struct {
    int roll;
    char name[NAME_LEN];
    char branch[20];
    int semester;
    int numSubjects;
    char subjectNames[MAX_SUBJECTS][SUBJ_LEN];
    float marks[MAX_SUBJECTS];
    float attendance;
} Student;
static User users[MAX_USERS];
static int userCount = 0;
static Student students[MAX_STUDENTS];
static int studentCount = 0;
static User *currentUser = NULL;
/* -------------------- Utilities -------------------- */
static void nowString(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (tm_info) strftime(buf, n, "%Y%m%d%H%M", tm_info);
}

static void logAction(const char *user, const char *action) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;
    char ts[20] = "UNKNOWN";
    nowString(ts, sizeof(ts));
    fprintf(fp, "[%s] %s: %s\n", ts, user ? user : "UNKNOWN", action);
    fclose(fp);
}
/* safer line reading */
static void readLine(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len && buf[len-1] == '\n') buf[len-1] = '\0';
}
/* read int with validation */
static int readInt(const char *prompt, int *out) {
    char tmp[64];
    while (1) {
        if (prompt) printf("%s", prompt);
        readLine(tmp, sizeof(tmp));
        if (tmp[0] == '\0') return 0; /* caller decides */
        char *endptr;
        long v = strtol(tmp, &endptr, 10);
        if (*endptr == '\0') { *out = (int)v; return 1; }
        printf("Invalid number, try again.\n");
    }
}
/* simple password input (masked on non-win) */
static void inputPassword(char *buf, size_t maxLen) {
#ifdef _WIN32
    int idx = 0; int ch;
    while ((ch = getch()) != '\r' && ch != '\n') {
        if (ch == 8) { if (idx>0) { idx--; printf("\b \b"); } }
        else if (idx < (int)maxLen-1) { buf[idx++] = (char)ch; printf("*"); }
    }
    buf[idx] = '\0'; printf("\n");
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    readLine(buf, maxLen);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
#endif
}
/* -------------------- Persistence -------------------- */
static void loadUsers(void) {
    FILE *fp = fopen(CREDENTIAL_FILE, "r");
    if (!fp) return;
    userCount = 0;
    while (userCount < MAX_USERS) {
        User u;
        if (fscanf(fp, "%49s %49s %11s %19s",
                   u.username, u.password, u.role, u.lastLogin) != 4) break;
        users[userCount++] = u;
    }
    fclose(fp);
}
static void saveUsers(void) {
    FILE *fp = fopen(CREDENTIAL_FILE, "w");
    if (!fp) return;
    for (int i = 0; i < userCount; ++i) {
        fprintf(fp, "%s %s %s %s\n",
                users[i].username,
                users[i].password,
                users[i].role,
                users[i].lastLogin[0] ? users[i].lastLogin : "-");
    }
    fclose(fp);
}
static void loadStudents(void) {
    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) return;
    studentCount = 0;
    while (studentCount < MAX_STUDENTS) {
        Student s; int i;
        if (fscanf(fp, "%d %49s %19s %d %d %f",
                   &s.roll, s.name, s.branch, &s.semester,
                   &s.numSubjects, &s.attendance) != 6) break;
        if (s.numSubjects < 0) s.numSubjects = 0;
        if (s.numSubjects > MAX_SUBJECTS) s.numSubjects = MAX_SUBJECTS;
        for (i = 0; i < s.numSubjects; ++i) {
            if (fscanf(fp, "%29s %f", s.subjectNames[i], &s.marks[i]) != 2) {
                strcpy(s.subjectNames[i], "NA"); s.marks[i] = 0.0f;
            }
        }
        students[studentCount++] = s;
    }
    fclose(fp);
}
static void saveStudents(void) {
    FILE *fp = fopen(STUDENT_FILE, "w");
    if (!fp) return;
    for (int i = 0; i < studentCount; ++i) {
        Student *s = &students[i];
        fprintf(fp, "%d %s %s %d %d %.2f\n",
                s->roll, s->name, s->branch, s->semester,
                s->numSubjects, s->attendance);
        for (int j = 0; j < s->numSubjects; ++j)
            fprintf(fp, "%s %.2f\n", s->subjectNames[j], s->marks[j]);
    }
    fclose(fp);
}

/* -------------------- Core logic -------------------- */
static User *findUser(const char *username) {
    for (int i = 0; i < userCount; ++i)
        if (strcmp(users[i].username, username) == 0) return &users[i];
    return NULL;
}
static Student *findStudentByRoll(int roll) {
    for (int i = 0; i < studentCount; ++i)
        if (students[i].roll == roll) return &students[i];
    return NULL;
}
static float computeCGPA(const Student *s) {
    if (s->numSubjects <= 0) return 0.0f;
    float total = 0.0f;
    for (int i = 0; i < s->numSubjects; ++i) total += s->marks[i];
    return total / (s->numSubjects * 10.0f);
}
static int countBacklogs(const Student *s) {
    int c = 0;
    for (int i = 0; i < s->numSubjects; ++i) if (s->marks[i] < 40.0f) c++;
    return c;
}
/* apply grace ONLY at the time of adding or explicit admin action */
static void applyGrace(Student *s) {
    if (!s || s->numSubjects <= 0) return;
    int fails = 0, failIdx = -1;
    const float passMark = 40.0f;
    for (int i = 0; i < s->numSubjects; ++i) {
        if (s->marks[i] < passMark) { fails++; failIdx = i; }
    }
    if (fails == 1 && failIdx >= 0) {
        float needed = passMark - s->marks[failIdx];
        float avg = 0.0f;
        for (int i = 0; i < s->numSubjects; ++i) avg += s->marks[i];
        avg /= s->numSubjects;
        if ((needed > 0 && needed <= 5.0f) || (avg >= 75.0f && needed <= 3.0f)) {
            s->marks[failIdx] += needed;
            if (currentUser) logAction(currentUser->username, "Applied grace");
        }
    }
}
/* Estimate graduation based on backlog clearance rate */
static void estimateGraduation(const Student *s) {
    int totalBacklogs = countBacklogs(s);
    int totalSem = 8;
    int clearPerSem = 2;
    int remainingSem = totalSem - s->semester + 1;
    int neededSem = (totalBacklogs + clearPerSem - 1) / clearPerSem;
    printf("Backlogs: %d\n", totalBacklogs);
    if (totalBacklogs == 0) printf("On track: can graduate in time.\n");
    else {
        printf("If you clear %d backlog(s) per sem, you need ~%d sem(s).\n", clearPerSem, neededSem);
        if (neededSem <= remainingSem) printf("Can still graduate in time.\n");
        else printf("May not graduate in time at this pace.\n");
    }
}
/* -------------------- Reports & Analysis -------------------- */
static void detectHardSubjects(void) {
    typedef struct { char name[SUBJ_LEN]; float sum; int count; int fails; } Stat;
    Stat stats[100]; int statCount = 0;
    for (int i = 0; i < studentCount; ++i) {
        Student *s = &students[i];
        for (int j = 0; j < s->numSubjects; ++j) {
            int idx = -1;
            for (int k = 0; k < statCount; ++k) if (strcmp(stats[k].name, s->subjectNames[j]) == 0) { idx = k; break; }
            if (idx == -1 && statCount < 100) { idx = statCount++; strcpy(stats[idx].name, s->subjectNames[j]); stats[idx].sum = 0; stats[idx].count = 0; stats[idx].fails = 0; }
            stats[idx].sum += s->marks[j];
            stats[idx].count++;
            if (s->marks[j] < 40.0f) stats[idx].fails++;
        }
    }
    printf("\nHard Subjects (fail%%>30 and avg<40):\n");
    printf("%-20s %-10s %-10s\n", "Subject", "Fail(%)", "Avg");
    for (int i = 0; i < statCount; ++i) {
        float avg = stats[i].sum / stats[i].count;
        float failp = (stats[i].fails * 100.0f) / stats[i].count;
        if (failp > 30.0f && avg < 40.0f) printf("%-20s %-10.1f %-10.1f\n", stats[i].name, failp, avg);
    }
}
static void backupStudents(void) {
    char ts[20]; nowString(ts, sizeof(ts));
    char fname[64]; snprintf(fname, sizeof(fname), "backup_student_%s.txt", ts);
    FILE *src = fopen(STUDENT_FILE, "r");
    if (!src) { printf("No student file to backup.\n"); return; }
    FILE *dst = fopen(fname, "w");
    if (!dst) { fclose(src); printf("Backup failed.\n"); return; }
    int ch; while ((ch = fgetc(src)) != EOF) fputc(ch, dst);
    fclose(src); fclose(dst);
    printf("Backup created: %s\n", fname);
    if (currentUser) logAction(currentUser->username, "Backup created");
}
/* -------------------- CRUD -------------------- */
static void addStudent(void) {
    if (studentCount >= MAX_STUDENTS) { printf("Student limit reached.\n"); return; }
    Student s; memset(&s,0,sizeof(s));
    printf("Roll: "); if (!readInt(NULL, &s.roll)) return;
    if (findStudentByRoll(s.roll)) { printf("Roll already exists.\n"); return; }
    printf("Name: "); readLine(s.name, sizeof(s.name));
    printf("Branch: "); readLine(s.branch, sizeof(s.branch));
    printf("Semester (1-8): "); readInt(NULL, &s.semester);
    printf("Number of subjects (max %d): ", MAX_SUBJECTS); readInt(NULL, &s.numSubjects);
    if (s.numSubjects > MAX_SUBJECTS) s.numSubjects = MAX_SUBJECTS;
    printf("Attendance (0-100): "); char tmp[32]; readLine(tmp, sizeof(tmp)); s.attendance = (float)atof(tmp);
    for (int i = 0; i < s.numSubjects; ++i) {
        printf("Subject %d name: ", i+1); readLine(s.subjectNames[i], SUBJ_LEN);
        printf("Marks: "); readLine(tmp, sizeof(tmp)); s.marks[i] = (float)atof(tmp);
    }
    applyGrace(&s);
    students[studentCount++] = s;
    saveStudents();
    if (currentUser) logAction(currentUser->username, "Added student");
}
static void displayStudents(int withNames) {
    printf("\n%-5s %-12s %-8s %-4s %-4s %-6s\n", "Roll", "Name", "Branch", "Sem", "Bkls", "CGPA");
    for (int i = 0; i < studentCount; ++i) {
        Student *s = &students[i];
        float cg = computeCGPA(s); int b = countBacklogs(s);
        printf("%-5d %-12s %-8s %-4d %-4d %-6.2f\n",
               s->roll, withNames ? s->name : "HIDDEN", s->branch, s->semester, b, cg);
    }
}
static void viewStudentReport(const Student *s) {
    if (!s) return;
    printf("\nRoll: %d\nName: %s\nBranch: %s\nSemester: %d\nAttendance: %.1f\n",
           s->roll, s->name, s->branch, s->semester, s->attendance);
    printf("%-15s %-6s\n", "Subject", "Marks");
    for (int i = 0; i < s->numSubjects; ++i) printf("%-15s %-6.1f\n", s->subjectNames[i], s->marks[i]);
    printf("CGPA: %.2f\n", computeCGPA(s));
    estimateGraduation(s);
}
static void searchStudent(void) {
    int roll; printf("Enter roll: "); if (!readInt(NULL, &roll)) return;
    Student *s = findStudentByRoll(roll);
    if (!s) { printf("Not found.\n"); return; }
    viewStudentReport(s);
}
/* -------------------- User management -------------------- */
static void resetPassword(void) {
    char uname[NAME_LEN]; printf("Username to reset: "); readLine(uname, sizeof(uname));
    User *u = findUser(uname); if (!u) { printf("User not found.\n"); return; }
    char newpass[NAME_LEN]; printf("New password: "); readLine(newpass, sizeof(newpass));
    strncpy(u->password, newpass, NAME_LEN-1); saveUsers();
    if (currentUser) logAction(currentUser->username, "Password reset");
}
static void createUser(void) {
    if (userCount >= MAX_USERS) { printf("User limit reached.\n"); return; }
    char uname[NAME_LEN], pass[NAME_LEN], role[ROLE_LEN];
    printf("New username: "); readLine(uname, sizeof(uname));
    if (findUser(uname)) { printf("User exists.\n"); return; }
    printf("Password: "); readLine(pass, sizeof(pass));
    printf("Role (admin/teacher/student/guest): "); readLine(role, sizeof(role));
    strncpy(users[userCount].username, uname, NAME_LEN-1);
    strncpy(users[userCount].password, pass, NAME_LEN-1);
    strncpy(users[userCount].role, role, ROLE_LEN-1);
    strcpy(users[userCount].lastLogin, "-");
    ++userCount; saveUsers();
    if (currentUser) logAction(currentUser->username, "Created user");
}

/* -------------------- Menus -------------------- */
static void adminMenu(void) {
    int ch = 0;
    do {
        printf("\n--- ADMIN MENU (%s, last login: %s) ---\n", currentUser->username, currentUser->lastLogin);
        printf("1. Add student\n2. Display students\n3. Search student\n4. Hard subjects report\n5. Create user\n6. Reset password\n7. Backup students\n8. Logout\nChoice: ");
        if (!readInt(NULL, &ch)) break;
        switch (ch) {
            case 1: addStudent(); break;
            case 2: displayStudents(1); break;
            case 3: searchStudent(); break;
            case 4: detectHardSubjects(); break;
            case 5: createUser(); break;
            case 6: resetPassword(); break;
            case 7: backupStudents(); break;
            case 8: if (currentUser) logAction(currentUser->username, "Logout"); break;
            default: printf("Invalid.\n");
        }
    } while (ch != 8);
}

static void teacherMenu(void) {
    int ch = 0;
    do {
        printf("\n--- TEACHER MENU (%s, last login: %s) ---\n", currentUser->username, currentUser->lastLogin);
        printf("1. Display students\n2. Search student\n3. Hard subjects report\n4. Logout\nChoice: ");
        if (!readInt(NULL, &ch)) break;
        switch (ch) {
            case 1: displayStudents(1); break;
            case 2: searchStudent(); break;
            case 3: detectHardSubjects(); break;
            case 4: if (currentUser) logAction(currentUser->username, "Logout"); break;
            default: printf("Invalid.\n");
        }
    } while (ch != 4);
}

static void studentMenu(void) {
    int ch = 0;
    do {
        printf("\n--- STUDENT MENU (%s, last login: %s) ---\n", currentUser->username, currentUser->lastLogin);
        printf("1. View my report (by roll)\n2. Logout\nChoice: ");
        if (!readInt(NULL, &ch)) break;
        if (ch == 1) {
            int roll; printf("Enter your roll: "); if (readInt(NULL, &roll)) {
                Student *s = findStudentByRoll(roll);
                if (!s) printf("Not found.\n"); else viewStudentReport(s);
            }
        } else if (ch == 2) { if (currentUser) logAction(currentUser->username, "Logout"); }
        else printf("Invalid.\n");
    } while (ch != 2);
}

static void guestMenu(void) {
    int ch = 0;
    do {
        printf("\n--- GUEST MENU (%s, last login: %s) ---\n", currentUser->username, currentUser->lastLogin);
        printf("1. View overall stats (no names)\n2. Hard subjects report\n3. Logout\nChoice: ");
        if (!readInt(NULL, &ch)) break;
        switch (ch) {
            case 1: displayStudents(0); break;
            case 2: detectHardSubjects(); break;
            case 3: if (currentUser) logAction(currentUser->username, "Logout"); break;
            default: printf("Invalid.\n");
        }
    } while (ch != 3);
}

/* -------------------- Login -------------------- */
static int loginSystem(void) {
    int attempts = 0;
    while (attempts < 3) {
        char uname[NAME_LEN], pass[NAME_LEN];
        printf("\nLogin\nUsername: "); readLine(uname, sizeof(uname));
        printf("Password: "); inputPassword(pass, sizeof(pass));
        User *u = findUser(uname);
        if (u && strcmp(u->password, pass) == 0) {
            char oldLogin[20]; strcpy(oldLogin, u->lastLogin[0] ? u->lastLogin : "-");
            if (oldLogin[0] == '\0' || strcmp(oldLogin, "-") == 0) strcpy(oldLogin, "FIRST");
            currentUser = u;
            printf("Login successful. Role: %s\nLast login: %s\n", u->role, oldLogin);
            strncpy(currentUser->lastLogin, oldLogin, sizeof(currentUser->lastLogin)-1);
            nowString(u->lastLogin, sizeof(u->lastLogin));
            saveUsers(); logAction(u->username, "Login success");
            return 1;
        }
        printf("Invalid credentials.\n"); attempts++; logAction(uname, "Login failed");
    }
    printf("Too many attempts. Locked out.\n"); return 0;
}

int main(void) {
    loadUsers(); loadStudents();
    if (!loginSystem()) return 0;
    if (strcmp(currentUser->role, "admin") == 0) adminMenu();
    else if (strcmp(currentUser->role, "teacher") == 0) teacherMenu();
    else if (strcmp(currentUser->role, "student") == 0) studentMenu();
    else if (strcmp(currentUser->role, "guest") == 0) guestMenu();
    else printf("No menu for this role.\n");
    return 0;
}
