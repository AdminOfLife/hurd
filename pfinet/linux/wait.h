#ifndef _HACK_WAIT_H_
#define _HACK_WAIT_H_

struct wait_queue
{
  struct task_struct *proc;
  struct wait_queue *next;
};

typedef struct select_table_struct
{
} select_table;

#endif
