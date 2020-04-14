#ifndef CMD_LIST_H
#define CMD_LIST_H

#include <stdbool.h>
#include <stddef.h> // for size_t

/*
 * A simple doubly linked list implementation that is tailored to the specific
 * application of command line history. Methods for pushing most recent commands
 * to front of a list; oldest entries automatically "fall off" (freed) when
 * max size is exceeded.
 */

typedef struct cmd_t {
    char *cmd_str;
    size_t cmd_num;
    struct cmd_t *next;
    struct cmd_t *prev;
    bool modified;
} cmd_t;

typedef struct {
    cmd_t *head;
    cmd_t *tail;
    size_t len;
    size_t max_len;
    size_t total_pushed;
} cmd_list_t;

// Allocates new list on the heap. Returns NULL for max_len less than 3.
cmd_list_t *cmd_list_new(size_t max_len);

// Frees allocated list from the heap. Assumes non-NULL argument.
void cmd_list_delete(cmd_list_t *list);

// Pushes new command with text `cmd_str` and ID # `cmd_num` to the given list.
// Assumes valid null-terminated string and non-null list. Pushes new cmd node
// as new head of list. If list is already max size, last item in list
// is automatically discarded and freed. Returns false if push failed.
bool cmd_list_push(cmd_list_t *list, const char *cmd_str, size_t cmd_num);

// Finds the first command in the list (most recent commands first)
// that matches the given prefix, NULL if no commands match.
// Used for evaluating commands of the form "!foo". Assumes non-null list
// and valid null-terminated prefix.
const char *cmd_list_find(cmd_list_t *list, const char *prefix);

// Returns head cmd node of list for iteration. Client should only
// write to `cmd_str` field of cmd node. Assumes valid null terminated list arg.
cmd_t *cmd_list_head(cmd_list_t *list);

// Returns tail cmd node of list for iteration. Client should only
// write to `cmd_str` field of cmd node. Assumes valid null terminated list arg.
cmd_t *cmd_list_tail(cmd_list_t *list);

#endif
