struct task_struct
{
    uint32_t pid;
    enum task_state
    {
        TASK_RUNNING,
        TASK_READY,
        TASK_BLOCKED
    } state;
    uint32_t esp; // 任务栈顶指针
    uint32_t cr3; // 页目录基址
    struct task_struct *next;
};
// ...
void task_init();
void schedule();