/*
* Programming Assignment 02: ls-v1.4.0
* v1.1.0: -l long listing format
* v1.2.0: default column display (down then across)
* v1.3.0: -x horizontal display (across)
* v1.4.0: alphabetical sort (qsort)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MODE_DEFAULT 0
#define MODE_LONG    1
#define MODE_HORIZ   2

void do_ls(const char *dir, int mode);
char **read_names(const char *dir, size_t *count, size_t *maxlen);
void free_names(char **names, size_t count);
int cmp_names(const void *a, const void *b);
void print_long(const char *dir, char **names, size_t count);
void mode_to_string(mode_t mode, char *str);
void get_user(uid_t uid, char *buf, size_t n);
void get_group(gid_t gid, char *buf, size_t n);
int get_term_width(void);
void print_columns(char **names, size_t count, size_t maxlen);
void print_horizontal(char **names, size_t count, size_t maxlen);

int main(int argc, char *argv[])
{
    int opt;
    int mode = MODE_DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
            case 'l':
                mode = MODE_LONG;
                break;
            case 'x':
                if (mode != MODE_LONG)   /* -l wins over -x, like real ls */
                    mode = MODE_HORIZ;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [dir...]\n", argv[0]);
                return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], mode);
            puts("");
        }
    }
    return 0;
}

/* Comparison function for qsort: qsort passes pointers to the array
   elements, and each element is a char *, so we get char ** here. */
int cmp_names(const void *a, const void *b)
{
    const char *x = *(const char * const *)a;
    const char *y = *(const char * const *)b;
    return strcmp(x, y);
}

/* Read all visible names into a dynamic array and sort them */
char **read_names(const char *dir, size_t *count, size_t *maxlen)
{
    *count = 0;
    *maxlen = 0;

    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return NULL;
    }

    char **names = NULL;
    size_t capacity = 0;
    struct dirent *entry;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (*count == capacity)
        {
            capacity = capacity ? capacity * 2 : 64;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                perror("realloc");
                break;
            }
            names = tmp;
        }
        names[*count] = strdup(entry->d_name);
        if (names[*count] == NULL)
        {
            perror("strdup");
            break;
        }
        size_t len = strlen(entry->d_name);
        if (len > *maxlen)
            *maxlen = len;
        (*count)++;
    }
    if (errno != 0)
        perror("readdir failed");
    closedir(dp);

    if (*count > 1)
        qsort(names, *count, sizeof(char *), cmp_names);

    return names;
}

void free_names(char **names, size_t count)
{
    for (size_t i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* Read + sort names, then print in the selected display mode */
void do_ls(const char *dir, int mode)
{
    size_t count, maxlen;
    char **names = read_names(dir, &count, &maxlen);

    if (mode == MODE_LONG)
        print_long(dir, names, count);
    else if (mode == MODE_HORIZ)
        print_horizontal(names, count, maxlen);
    else
        print_columns(names, count, maxlen);

    free_names(names, count);
}

/* Ask the terminal for its width; use 80 if that fails */
int get_term_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
        return 80;
    return ws.ws_col;
}

/* Print names "down then across" */
void print_columns(char **names, size_t count, size_t maxlen)
{
    if (count == 0)
        return;

    size_t col_width = maxlen + 2;
    size_t cols = get_term_width() / col_width;
    if (cols < 1)
        cols = 1;
    if (cols > count)
        cols = count;
    size_t rows = (count + cols - 1) / cols;   /* ceiling division */

    for (size_t r = 0; r < rows; r++)
    {
        for (size_t c = 0; c < cols; c++)
        {
            size_t idx = r + c * rows;
            if (idx >= count)
                break;
            if (c < cols - 1 && idx + rows < count)
                printf("%-*s", (int)col_width, names[idx]);
            else
                printf("%s", names[idx]);
        }
        putchar('\n');
    }
}

/* Print names left to right, wrap when the line is full */
void print_horizontal(char **names, size_t count, size_t maxlen)
{
    size_t col_width = maxlen + 2;
    size_t width = get_term_width();
    size_t pos = 0;

    for (size_t i = 0; i < count; i++)
    {
        if (pos + col_width > width && pos > 0)
        {
            putchar('\n');
            pos = 0;
        }
        printf("%-*s", (int)col_width, names[i]);
        pos += col_width;
    }
    if (count > 0)
        putchar('\n');
}

/* Convert st_mode into a string like "drwxr-xr-x" */
void mode_to_string(mode_t mode, char *str)
{
    /* file type */
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '?';

    /* owner */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID)
        str[3] = (mode & S_IXUSR) ? 's' : 'S';
    else
        str[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* group */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID)
        str[6] = (mode & S_IXGRP) ? 's' : 'S';
    else
        str[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* others */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX)
        str[9] = (mode & S_IXOTH) ? 't' : 'T';
    else
        str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
}

void get_user(uid_t uid, char *buf, size_t n)
{
    struct passwd *pw = getpwuid(uid);
    if (pw)
        snprintf(buf, n, "%s", pw->pw_name);
    else
        snprintf(buf, n, "%u", (unsigned)uid);
}

void get_group(gid_t gid, char *buf, size_t n)
{
    struct group *gr = getgrgid(gid);
    if (gr)
        snprintf(buf, n, "%s", gr->gr_name);
    else
        snprintf(buf, n, "%u", (unsigned)gid);
}

/* Long listing (-l) using the already sorted names */
void print_long(const char *dir, char **names, size_t count)
{
    struct stat st;
    char path[PATH_MAX];
    char user[64], group[64];
    int w_link = 1, w_user = 1, w_group = 1, w_size = 1;
    long long total_blocks = 0;

    /* PASS 1: find the widest value in each column and the block total */
    for (size_t i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (lstat(path, &st) == -1)
        {
            perror(path);
            continue;
        }
        char tmp[64];
        int len;

        len = snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)st.st_nlink);
        if (len > w_link) w_link = len;
        get_user(st.st_uid, user, sizeof(user));
        len = strlen(user);
        if (len > w_user) w_user = len;
        get_group(st.st_gid, group, sizeof(group));
        len = strlen(group);
        if (len > w_group) w_group = len;
        len = snprintf(tmp, sizeof(tmp), "%lld", (long long)st.st_size);
        if (len > w_size) w_size = len;

        total_blocks += st.st_blocks;
    }

    /* st_blocks counts 512-byte units; ls prints 1K units */
    printf("total %lld\n", total_blocks / 2);

    /* PASS 2: print every entry */
    for (size_t i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (lstat(path, &st) == -1)
            continue;

        char perms[11];
        mode_to_string(st.st_mode, perms);
        get_user(st.st_uid, user, sizeof(user));
        get_group(st.st_gid, group, sizeof(group));

        time_t mtime = st.st_mtime;
        char *timestr = ctime(&mtime);   /* "Mon Oct 12 10:12:00 2026\n" */

        printf("%s %*lu %-*s %-*s %*lld %.12s %s",
               perms,
               w_link, (unsigned long)st.st_nlink,
               w_user, user,
               w_group, group,
               w_size, (long long)st.st_size,
               timestr + 4,              /* skip day name, keep "Oct 12 10:12" */
               names[i]);

        /* for symlinks, ls shows "name -> target" */
        if (S_ISLNK(st.st_mode))
        {
            char target[PATH_MAX];
            ssize_t n = readlink(path, target, sizeof(target) - 1);
            if (n != -1)
            {
                target[n] = '\0';
                printf(" -> %s", target);
            }
        }
        putchar('\n');
    }
}