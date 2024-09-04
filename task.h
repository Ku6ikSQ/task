#ifndef TASK_H_SENTRY
#define TASK_H_SENTRY

#define TASK_EXT ".tsk"
#define TASK_CORE_FILE "main" TASK_EXT

struct task;

void task_free(struct task *task);
char task_write(const char *path, const struct task *task);
char task_make_file(int fd, char is_filter);
struct task *task_read(const char *path);
char task_print(const struct task *task, const char *taskpath);
char task_set_field(struct task *task, const char *name, const char *value,
	char rewrite);
#endif
