#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <err.h>


ssize_t joinpath(char *dst, size_t dstsize, const char *rel) {
    size_t rel_len = strlen(rel);

    /* If rel is absolute, replace root */
    if (rel[0] == '/') {
        if (rel_len + 1 > dstsize) {
            return -1;
        }
        strcpy(dst, rel);
        return rel_len;
    }

    // dirname may modify the argument or return static memory, so use a copy
    char pathbuf[PATH_MAX];
    strcpy(pathbuf, dst);
    char *rootdir = dirname(pathbuf);

    // copy rootdir into and ensure a trailing slash
    // also check that there is space to append rel to dst, return -1
    // if insufficient, with errno set.
    size_t end = strlen(rootdir);
    if (dst[end - 1] != '/') {
        if (end + rel_len + 2 > dstsize) {
            errno = EOVERFLOW;
            return -1;
        }
        strcpy(dst, rootdir);
        dst[end++] = '/';
    } else {
        if (end + rel_len + 1 > dstsize) {
            errno = EOVERFLOW;
            return -1;
        }
        strcpy(dst, rootdir);
    }

    strcpy(dst + end, rel);
    return end + rel_len;
}

static char linktarget[PATH_MAX];


/* Follow symlinks until we find a real file or another error occurs.
 *
 * Return a pointer to static char data on success, or NULL on error, in
 * which case errno will be set.
 */
static const char *follow_link(const char *link) {
    ssize_t res_len;
    char linkbuf[PATH_MAX];
    strcpy(linktarget, link);

    for (;;) {
        res_len = readlink(linktarget, linkbuf, PATH_MAX);
        if (res_len != -1) {
            linkbuf[res_len] = '\0';

            if (linkbuf[0] == '/') {
                strcpy(linktarget, linkbuf);
            } else {
                if (joinpath(linktarget, PATH_MAX, linkbuf) < 0) {
                    return NULL;
                }
            }
        } else if (errno == EINVAL) {
            // Was not a link, therefore we're done
            return linktarget;
        } else {
            // Another error
            return NULL;
        }
    }
}


int main(int argc, const char **argv) {
    char **newargv;
    char newinterp[PATH_MAX];
    size_t bs;

    if (argc < 3) {
        strcpy(newinterp, argv[0]);
        char *base = basename(newinterp);
        fprintf(stderr, "usage: %s <interpreter> <script>\n", base);
        return 2;
    }

    if (strlen(argv[2]) > PATH_MAX) {
        err(2, "path too long\n");
    }

    /* Assemble the path to run */
    const char *linkdest = follow_link(argv[2]);
    if (!linkdest) {
        err(2, "error reading %s", argv[2]);
    }

    strcpy(newinterp, linkdest);

    if (joinpath(newinterp, PATH_MAX, argv[1]) < 0) {
        err(2, "path too long\n");
    }

    /* Build argv, dropping the interpreter argument */
    newargv = malloc(argc * sizeof(char *));
    if (NULL == newargv) {
        err(1, "Failed to allocate memory");
    }
    newargv[0] = newinterp;
    memcpy(newargv + 1, argv + 2, sizeof(char *) * (argc - 2));
    newargv[argc - 1] = NULL;

    /* Exec the target */
    execv(newinterp, newargv);

    /* If exec failed, return an error */
    err(127, "failed to execute %s", newinterp);
}
