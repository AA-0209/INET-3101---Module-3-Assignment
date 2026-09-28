
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define SEATS     24
#define NAME_LEN  32   /* max 31 chars + '\0' */
#define LINE_LEN  128

struct seat {
    int  id;                 /* 1-24 */
    int  assigned;           /* 0 = empty, 1 = assigned */
    char last[NAME_LEN];
    char first[NAME_LEN];
};

enum { IN_OK = 0, IN_TOO_LONG = 1, IN_EOF = -1 };

static int eof_hit = 0;

/* ---------- input helpers ---------- */

/* Read one line into buf, strip newline/CR. Discards the rest of an overlong line. */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        eof_hit = 1;
        return IN_EOF;
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
        if (len > 0 && buf[len - 1] == '\r')
            buf[--len] = '\0';
        return IN_OK;
    }

    /* No newline: line was too long, or last line had no newline before EOF. */
    int c, extra = 0;
    while ((c = getchar()) != '\n' && c != EOF)
        extra = 1;
    if (len > 0 && buf[len - 1] == '\r')
        buf[--len] = '\0';
    return extra ? IN_TOO_LONG : IN_OK;
}

static void trim(char *s)
{
    size_t start = 0, len = strlen(s);
    while (start < len && isspace((unsigned char)s[start])) start++;
    while (len > start && isspace((unsigned char)s[len - 1])) len--;
    memmove(s, s + start, len - start);
    s[len - start] = '\0';
}

/* Prompt until the user enters a single character contained in `valid`.
 * Returns the lowercase choice, or 0 on EOF. */
static char get_choice(const char *valid)
{
    char buf[LINE_LEN];
    for (;;) {
        printf("Choice: ");
        fflush(stdout);
        int r = read_line(buf, sizeof buf);
        if (r == IN_EOF) return 0;
        trim(buf);
        if (r == IN_OK && strlen(buf) == 1) {
            char c = (char)tolower((unsigned char)buf[0]);
            if (strchr(valid, c)) return c;
        }
        printf("Invalid choice. Enter one of: %s\n", valid);
    }
}

/* y/n confirmation. Returns 1 yes, 0 no/EOF. */
static int confirm(const char *msg)
{
    char buf[LINE_LEN];
    for (;;) {
        printf("%s (y/n): ", msg);
        fflush(stdout);
        int r = read_line(buf, sizeof buf);
        if (r == IN_EOF) return 0;
        trim(buf);
        if (r == IN_OK && strlen(buf) == 1) {
            char c = (char)tolower((unsigned char)buf[0]);
            if (c == 'y') return 1;
            if (c == 'n') return 0;
        }
        printf("Please enter y or n.\n");
    }
}

/* Prompt for a seat number. Returns 1-24, or 0 if the user aborts / EOF. */
static int get_seat_number(void)
{
    char buf[LINE_LEN];
    for (;;) {
        printf("Enter seat number (1-%d), or 'q' / blank line to abort: ", SEATS);
        fflush(stdout);
        int r = read_line(buf, sizeof buf);
        if (r == IN_EOF) return 0;
        if (r == IN_TOO_LONG) { printf("Input too long.\n"); continue; }
        trim(buf);
        if (buf[0] == '\0' || ((buf[0] == 'q' || buf[0] == 'Q') && buf[1] == '\0'))
            return 0;

        char *end;
        errno = 0;
        long n = strtol(buf, &end, 10);
        if (errno != 0 || end == buf || *end != '\0' || n < 1 || n > SEATS) {
            printf("Invalid seat number.\n");
            continue;
        }
        return (int)n;
    }
}

static int valid_name(const char *s)
{
    if (!isalpha((unsigned char)s[0])) return 0;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (!(isalpha(c) || c == ' ' || c == '-' || c == '\''))
            return 0;
    }
    return 1;
}

/* Prompt for a name. Returns 1 on success, 0 if aborted / EOF. */
static int get_name(const char *label, char *out)
{
    char buf[LINE_LEN];
    for (;;) {
        printf("Enter passenger %s name (blank line to abort): ", label);
        fflush(stdout);
        int r = read_line(buf, sizeof buf);
        if (r == IN_EOF) return 0;
        if (r == IN_TOO_LONG) { printf("Name too long (max %d characters).\n", NAME_LEN - 1); continue; }
        trim(buf);
        if (buf[0] == '\0') return 0;
        if (strlen(buf) >= NAME_LEN) { printf("Name too long (max %d characters).\n", NAME_LEN - 1); continue; }
        if (!valid_name(buf)) { printf("Names may contain only letters, spaces, hyphens, and apostrophes.\n"); continue; }
        strcpy(out, buf);
        return 1;
    }
}

/* ---------- seat operations ---------- */

static void init_flight(struct seat f[])
{
    for (int i = 0; i < SEATS; i++) {
        f[i].id = i + 1;
        f[i].assigned = 0;
        f[i].last[0] = f[i].first[0] = '\0';
    }
}

static int count_empty(const struct seat f[])
{
    int n = 0;
    for (int i = 0; i < SEATS; i++)
        if (!f[i].assigned) n++;
    return n;
}

static void show_empty_count(const struct seat f[])
{
    printf("Number of empty seats: %d of %d\n", count_empty(f), SEATS);
}

static void show_empty_list(const struct seat f[])
{
    if (count_empty(f) == 0) { printf("No empty seats.\n"); return; }
    printf("Empty seats:");
    for (int i = 0; i < SEATS; i++)
        if (!f[i].assigned) printf(" %d", f[i].id);
    printf("\n");
}

static int ci_cmp(const char *a, const char *b)
{
    while (*a && *b) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d) return d;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static int seat_cmp(const void *pa, const void *pb)
{
    const struct seat *a = *(const struct seat * const *)pa;
    const struct seat *b = *(const struct seat * const *)pb;
    int d = ci_cmp(a->last, b->last);
    if (d) return d;
    d = ci_cmp(a->first, b->first);
    if (d) return d;
    return a->id - b->id;
}

static void show_alpha_list(const struct seat f[])
{
    const struct seat *list[SEATS];
    int n = 0;
    for (int i = 0; i < SEATS; i++)
        if (f[i].assigned) list[n++] = &f[i];

    if (n == 0) { printf("No seats are assigned.\n"); return; }

    qsort(list, (size_t)n, sizeof list[0], seat_cmp);
    printf("%-5s %-*s %-*s\n", "Seat", NAME_LEN, "Last Name", NAME_LEN, "First Name");
    for (int i = 0; i < n; i++)
        printf("%-5d %-*s %-*s\n", list[i]->id, NAME_LEN, list[i]->last, NAME_LEN, list[i]->first);
}

static void assign_seat(struct seat f[])
{
    char first[NAME_LEN], last[NAME_LEN];

    if (count_empty(f) == 0) { printf("Flight is full.\n"); return; }
    show_empty_list(f);

    int n;
    for (;;) {
        n = get_seat_number();
        if (n == 0) { printf("Assignment aborted.\n"); return; }
        if (f[n - 1].assigned) { printf("Seat %d is already assigned.\n", n); continue; }
        break;
    }

    if (!get_name("first", first) || !get_name("last", last)) {
        printf("Assignment aborted.\n");
        return;
    }

    char msg[LINE_LEN * 2];
    snprintf(msg, sizeof msg, "Assign %s %s to seat %d?", first, last, n);
    if (!confirm(msg)) { printf("Assignment aborted.\n"); return; }

    f[n - 1].assigned = 1;
    strcpy(f[n - 1].first, first);
    strcpy(f[n - 1].last, last);
    printf("Seat %d assigned to %s %s.\n", n, first, last);
}

static void delete_seat(struct seat f[])
{
    if (count_empty(f) == SEATS) { printf("No seats are assigned.\n"); return; }

    printf("Assigned seats:\n");
    for (int i = 0; i < SEATS; i++)
        if (f[i].assigned)
            printf("  %2d: %s, %s\n", f[i].id, f[i].last, f[i].first);

    int n;
    for (;;) {
        n = get_seat_number();
        if (n == 0) { printf("Deletion aborted.\n"); return; }
        if (!f[n - 1].assigned) { printf("Seat %d is not assigned.\n", n); continue; }
        break;
    }

    char msg[LINE_LEN * 2];
    snprintf(msg, sizeof msg, "Delete %s %s from seat %d?", f[n - 1].first, f[n - 1].last, n);
    if (!confirm(msg)) { printf("Deletion aborted.\n"); return; }

    f[n - 1].assigned = 0;
    f[n - 1].first[0] = f[n - 1].last[0] = '\0';
    printf("Seat %d is now empty.\n", n);
}

/* ---------- menus ---------- */

static void flight_menu(struct seat f[], const char *name)
{
    for (;;) {
        printf("\n--- %s Flight ---\n", name);
        printf("a) Show number of empty seats\n");
        printf("b) Show list of empty seats\n");
        printf("c) Show alphabetical list of seats\n");
        printf("d) Assign a customer to a seat assignment\n");
        printf("e) Delete a seat assignment\n");
        printf("f) Return to Main Menu\n");

        char c = get_choice("abcdef");
        switch (c) {
            case 'a': show_empty_count(f); break;
            case 'b': show_empty_list(f);  break;
            case 'c': show_alpha_list(f);  break;
            case 'd': assign_seat(f);      break;
            case 'e': delete_seat(f);      break;
            case 'f': return;
            default:  return;              /* EOF */
        }
        if (eof_hit) return;
    }
}

int main(void)
{
    struct seat outbound[SEATS], inbound[SEATS];
    init_flight(outbound);
    init_flight(inbound);

    printf("Colossus Airlines Reservation System\n");
    for (;;) {
        printf("\n=== Main Menu ===\n");
        printf("a) Outbound Flight\n");
        printf("b) Inbound Flight\n");
        printf("c) Quit\n");

        char c = get_choice("abc");
        if (c == 'a')      flight_menu(outbound, "Outbound");
        else if (c == 'b') flight_menu(inbound, "Inbound");
        else               break;          /* 'c' or EOF */

        if (eof_hit) break;
    }

    if (eof_hit) printf("\nEnd of input reached.\n");
    printf("Goodbye.\n");
    return 0;
}