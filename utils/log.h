#ifndef __LOG_H_INCLUDED__
#define __LOG_H_INCLUDED__

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LOG_ON
// #undef LOG_ON

char* GetTimestamp();

extern const char* path_tree;
extern const char* path_diff;

extern int fd_tree;
extern int fd_diff;

// yes, this copy-paste can be evaded using pre-processor # directive, but meh, it's just logging
// also, because tree is a singleton in this program, it's unnecessary to do deparate logs for
// separate trees

// ====================
// TREE

#ifdef LOG_ON
#define PRINT_TREE()                                                                               \
  if (tree == NULL) {                                                                              \
    dprintf(fd_tree, "ERROR: NULL pointer to the structure!\n");                                   \
  } else {                                                                                         \
    char* timestamp = GetTimestamp();                                                              \
                                                                                                   \
    dprintf(fd_tree,                                                                               \
            "%s\n"                                                                                 \
            "  hash: %ld\n"                                                                        \
            "  tree capacity: %d\n"                                                                \
            "  tree size: %d\n"                                                                    \
            "  first free element's physical address: %d\n"                                        \
            "  root address: %d\n"                                                                 \
            "  error: %d \n",                                                                      \
            timestamp,                                                                             \
            tree->hash,                                                                            \
            tree->max,                                                                             \
            tree->cur,                                                                             \
            tree->free,                                                                            \
            tree->root,                                                                            \
            tree->err);                                                                            \
    free(timestamp);                                                                               \
                                                                                                   \
    if (tree->nodes != NULL) {                                                                     \
      dprintf(fd_tree, "  | &|   val|               &L|  &R|  &par|\n");                           \
      for (int i = 0; i < tree->max; i++) {                                                        \
        if (((tree->nodes[i]).parent == 0 && i != tree->root) || tree->cur == 0)                   \
          dprintf(fd_tree,                                                                         \
                  "  [%2d ] (type: %d value: %6d [%3d, %3d] <%3d>)\n",                             \
                  i,                                                                               \
                  tree->nodes[i].data.type,                                                        \
                  tree->nodes[i].data.value,                                                       \
                  tree->nodes[i].branches[Left],                                                   \
                  tree->nodes[i].branches[Right],                                                  \
                  tree->nodes[i].parent);                                                          \
        else                                                                                       \
          dprintf(fd_tree,                                                                         \
                  "  [%2d*] (type: %d value: %6d [%3d, %3d] <%3d>)\n",                             \
                  i,                                                                               \
                  tree->nodes[i].data.type,                                                        \
                  tree->nodes[i].data.value,                                                       \
                  tree->nodes[i].branches[Left],                                                   \
                  tree->nodes[i].branches[Right],                                                  \
                  tree->nodes[i].parent);                                                          \
      }                                                                                            \
                                                                                                   \
      for (int i = 0; i < tree->max; i++)                                                          \
        dprintf(fd_tree, "[%3d]", i);                                                              \
                                                                                                   \
      dprintf(fd_tree, "\n");                                                                      \
                                                                                                   \
      for (int i = 0; i < tree->max; i++)                                                          \
        dprintf(fd_tree, "[%3d]", tree->memmap[i]);                                                \
                                                                                                   \
      dprintf(fd_tree, "\n\n");                                                                    \
    }                                                                                              \
  }
#endif

#ifdef LOG_ON
#define LOG_LVL_TREE_INIT()                                                                        \
  fd_tree = open(path_tree, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);                      \
  close(fd_tree)
#else
#define LOG_LVL_TREE_INIT()
#endif

#ifdef LOG_ON
#define LOG_LVL_TREE_ROUTINE(msg, ...)                                                             \
  fd_tree = open(path_tree, O_APPEND | O_WRONLY);                                                  \
  dprintf(fd_tree, ">           [%s] at <%d> ", __FUNCTION__, __LINE__);                           \
  dprintf(fd_tree, msg, ##__VA_ARGS__);                                                            \
  /*PRINT_TREE()*/                                                                                 \
  close(fd_tree);
#else
#define LOG_LVL_TREE_ROUTINE()
#endif

#ifdef LOG_ON
#define LOG_LVL_TREE_FAILURE(msg, ...)                                                             \
  fd_tree = open(path_tree, O_APPEND | O_WRONLY);                                                  \
  dprintf(fd_tree, "> {FAILURE} [%s] at <%d> ", __FUNCTION__, __LINE__);                           \
  dprintf(fd_tree, msg, ##__VA_ARGS__);                                                            \
  /*PRINT_TREE() */                                                                                \
  close(fd_tree);
#else
#define LOG_LVL_TREE_FAILURE()
#endif

#ifdef LOG_ON
#define LOG_LVL_TREE_ERROR(msg, ...)                                                               \
  fd_tree = open(path_tree, O_APPEND | O_WRONLY);                                                  \
  dprintf(fd_tree, "> {ERROR}   [%s] at <%d> ", __FUNCTION__, __LINE__);                           \
  dprintf(fd_tree, msg, ##__VA_ARGS__);                                                            \
  close(fd_tree);
#else
#define LOG_LVL_TREE_ERROR()
#endif

// ====================
// DIFF

#ifdef LOG_ON
#define LOG_LVL_DIFF_INIT()                                                                        \
  fd_diff = open(path_diff, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);                      \
  close(fd_diff)
#else
#define LOG_LVL_DIFF_INIT()
#endif

#ifdef LOG_ON
#define LOG_LVL_DIFF_ROUTINE(msg, ...)                                                             \
  fd_diff = open(path_diff, O_APPEND | O_WRONLY);                                                  \
  dprintf(fd_diff, ">           [%s] at <%d> ", __FUNCTION__, __LINE__);                           \
  dprintf(fd_diff, msg, ##__VA_ARGS__);                                                            \
  PRINT_TREE()                                                                                     \
  close(fd_diff);
#else
#define LOG_LVL_DIFF_ROUTINE()
#endif

#ifdef LOG_ON
#define LOG_LVL_DIFF_FAILURE(msg, ...)                                                             \
  fd_diff = open(path_diff, O_APPEND | O_WRONLY);                                                  \
  dprintf(fd_diff, "> {FAILURE} [%s] at <%d> ", __FUNCTION__, __LINE__);                           \
  dprintf(fd_diff, msg, ##__VA_ARGS__);                                                            \
  PRINT_TREE()                                                                                     \
  close(fd_diff);
#else
#define LOG_LVL_DIFF_FAILURE()
#endif

#ifdef LOG_ON
#define LOG_LVL_DIFF_ERROR(msg, ...)                                                               \
  fd_diff = open(path_diff, O_APPEND | O_WRONLY);                                                  \
  dprintf(fd_diff, "> {ERROR}   [%s] at <%d> ", __FUNCTION__, __LINE__);                           \
  dprintf(fd_diff, msg, ##__VA_ARGS__);                                                            \
  PRINT_TREE()                                                                                     \
  close(fd_diff);
#else
#define LOG_LVL_DIFF_ERROR()
#endif

#endif