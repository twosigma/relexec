#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <libgen.h>
#include <linux/limits.h>


ssize_t joinpath(char *root, const char *rel, size_t rootsize) {
    size_t rel_len = strlen(rel);

    /* If rel is absolute, replace root */
    if (rel[0] == '/') {
        if (rel_len + 1 > rootsize) {
            return -1;
        }
        strcpy(root, rel);
        return rel_len;
    }

    // dirname may modify the argument or return static memory, so use a copy
    char *buf = strdup(root);
    char *rootdir = dirname(buf);

    ssize_t ret = -1;
    size_t end = strlen(rootdir);
    if (root[end - 1] != '/') {
        if (end + rel_len + 2 > rootsize) {
            goto end;
        }
        strcpy(root, rootdir);
        root[end++] = '/';
    } else {
        if (end + rel_len + 1 > rootsize) {
            goto end;
        }
        strcpy(root, rootdir);
    }

    strcpy(root + end, rel);
    ret = end + rel_len;

    end:
        free(buf);
        return ret;
}

static char _linktarget[PATH_MAX];


/* Follow symlinks until we find a real file or another error occurs.
 *
 * Return a pointer to static char data on success, or NULL on error, in
 * which case errno will be set.
 */
const char *follow_link(const char *link) {
    ssize_t res_len;
    char linkbuf[PATH_MAX];
    strcpy(_linktarget, link);

    for (;;) {
        res_len = readlink(_linktarget, linkbuf, PATH_MAX);
        if (res_len != -1) {
            linkbuf[res_len] = '\0';

            if (linkbuf[0] == '/') {
                strcpy(_linktarget, linkbuf);
            } else {
                if (joinpath(_linktarget, linkbuf, PATH_MAX) < 0) {
                    errno = EOVERFLOW;
                    return NULL;
                }
            }
        } else if (errno == EINVAL) {
            // Was not a link, therefore we're done
            return _linktarget;
        } else {
            // Another error
            return NULL;
        }
    }
}


int main(int argc, const char **argv) {
    char *newargv[ARG_MAX];
    char newinterp[PATH_MAX];
    size_t bs;

    if (argc < 3) {
        fprintf(stderr, "usage: relexec <interpreter> <script>\n");
        return 2;
    }

    if (strlen(argv[2]) > PATH_MAX) {
        fprintf(stderr, "relexec: path too long\n");
        return 2;
    }

    /* Assemble the path to run */
    const char *linkdest = follow_link(argv[2]);
    if (!linkdest) {
        fprintf(
            stderr,
            "Error reading %s: %s\n",
            argv[2], strerror(errno)
        );
        return 2;
    }

    strcpy(newinterp, linkdest);

    if (joinpath(newinterp, argv[1], PATH_MAX) < 0) {
        fprintf(stderr, "relexec: path too long\n");
        return 2;
    }

    /* Build argv, dropping the interpreter argument */
    newargv[0] = newinterp;
    memmove(newargv + 1, argv + 2, sizeof(char **) * (argc - 2));
    newargv[argc - 1] = NULL;

    /* Exec the target */
    execv(newinterp, newargv);

    /* If exec failed, return an error */
    fprintf(stderr, "%s: %s\n", newinterp, strerror(errno));
    return 127;
}
