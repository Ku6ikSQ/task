#ifndef TASK_H_SENTRY
#define TASK_H_SENTRY

#define TASK_EXT ".tsk"
#define TASK_CORE_FILE "main" TASK_EXT

char task_write(int fd, char is_filter);
#if 0
char is_task(const char *dirname);
#endif
char print_task(const char *path);
#endif
