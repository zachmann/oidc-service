#define _XOPEN_SOURCE 700
#include "file_io.h"
#include "list/src/list.h"
#include "utils/memory.h"
#include "utils/stringUtils.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

/** @fn char* readFile(const char* path)
 * @brief reads a file and returns a pointer to the content
 * @param path the file to be read
 * @return a pointer to the file content. Has to be freed after usage. On
 * failure NULL is returned and oidc_errno is set.
 */
char* readFile(const char* path) {
  syslog(LOG_AUTHPRIV | LOG_DEBUG, "Reading file: %s", path);
  FILE* fp;
  long  lSize;
  char* buffer;

  fp = fopen(path, "rb");
  if (!fp) {
    syslog(LOG_AUTHPRIV | LOG_NOTICE, "%m\n");
    oidc_errno = OIDC_EFOPEN;
    return NULL;
  }

  fseek(fp, 0L, SEEK_END);
  lSize = ftell(fp);
  rewind(fp);

  buffer = secAlloc(lSize + 1);
  if (!buffer) {
    fclose(fp);
    syslog(LOG_AUTHPRIV | LOG_ALERT,
           "memory alloc failed in function readFile '%s': %m\n", path);
    oidc_errno = OIDC_EALLOC;
    return NULL;
  }

  if (1 != fread(buffer, lSize, 1, fp)) {
    if (feof(fp)) {
      oidc_errno = OIDC_EEOF;
    } else {
      oidc_errno = OIDC_EFREAD;
    }
    fclose(fp);
    secFree(buffer);
    syslog(LOG_AUTHPRIV | LOG_ALERT,
           "entire read failed in function readFile '%s': %m\n", path);
    return NULL;
  }
  fclose(fp);
  return buffer;
}

/** @fn void writeFile(const char* path, const char* text)
 * @brief writes text to a file
 * @note \p text has to be nullterminated and must not contain nullbytes.
 * @param path the file to be written
 * @param text the nullterminated text to be written
 * @return OIDC_OK on success, OID_EFILE if an error occured. The system sets
 * errno.
 */
oidc_error_t writeFile(const char* path, const char* text) {
  if (path == NULL || text == NULL) {
    oidc_setArgNullFuncError(__func__);
    return oidc_errno;
  }
  FILE* f = fopen(path, "w");
  if (f == NULL) {
    syslog(LOG_AUTHPRIV | LOG_ALERT,
           "Error opening file '%s' in function writeToFile().\n", path);
    return OIDC_EFOPEN;
  }
  fprintf(f, "%s", text);
  fclose(f);
  return OIDC_SUCCESS;
}

/** @fn int fileDoesExist(const char* path)
 * @brief checks if a file exists
 * @param path the path to the file to be checked
 * @return 1 if the file does exist, 0 if not
 */
int fileDoesExist(const char* path) { return access(path, F_OK) == 0 ? 1 : 0; }

/** @fn int dirExists(const char* path)
 * @brief checks if a directory exists
 * @param path the path to the directory to be checked
 * @return 1 if the directory does exist, 0 if not, -1 if an error occured
 */
int dirExists(const char* path) {
  DIR* dir = opendir(path);
  if (dir) { /* Directory exists. */
    closedir(dir);
    return 1;
  } else if (ENOENT == errno) { /* Directory does not exist. */
    return 0;
  } else { /* opendir() failed for some other reason. */
    syslog(LOG_AUTHPRIV | LOG_ALERT, "opendir: %m");
    exit(EXIT_FAILURE);
    return -1;
  }
}

oidc_error_t createDir(const char* path) {
  if (mkdir(path, 0777) != 0) {
    oidc_setErrnoError();
    return oidc_errno;
  }
  return OIDC_SUCCESS;
}

/** @fn int removeFile(const char* path)
 * @brief removes a file
 * @param path the path to the file to be removed
 * @return On success, 0 is returned.  On error, -1 is returned, and errno is
 * set appropriately.
 */
int removeFile(const char* path) { return unlink(path); }

list_t* getLinesFromFile(const char* path) {
  if (path == NULL) {
    oidc_setArgNullFuncError(__func__);
    return NULL;
  }
  syslog(LOG_AUTHPRIV | LOG_DEBUG, "Getting Lines from file: %s", path);
  FILE* fp = fopen(path, "r");
  if (fp == NULL) {
    oidc_setErrnoError();
    return NULL;
  }

  list_t* lines = list_new();
  lines->free   = _secFree;
  lines->match  = (int (*)(void*, void*))strequal;

  char*   line = NULL;
  size_t  len  = 0;
  ssize_t read = 0;
  while ((read = getline(&line, &len, fp)) != -1) {
    if (line[strlen(line) - 1] == '\n') {
      line[strlen(line) - 1] = '\0';
    }
    list_rpush(lines, list_node_new(oidc_strcopy(line)));
    secFreeN(line, len);
  }
  fclose(fp);
  return lines;
}