#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/sched.h>

SYSCALL_DEFINE2(smunch, int, pid, unsigned long, sig_mask) {
   struct task_struct *p;
   unsigned long flags;
   sigset_t sig_set;

   rcu_read_lock();
   p = pid_task(find_vpid(pid), PIDTYPE_PID);
   rcu_read_unlock();

   if(p == NULL || (!thread_group_empty(p)) || p->trace)
      return -1;

   siginitset(&sig_set, sig_mask);
   if(p->exit_state == EXIT_ZOMBIE) {
      if(sigismember(&sig_set, SIGKILL)) release_task(p);
      return 0;
   }

   if(!lock_task_sighand(p, &flags))
      return -1;

   sigaddsetmask(&(p->signal->shared_pending.signal), sig_mask);
   set_tsk_thread_flag(p, TIF_SIGPENDING);
   unlock_task_sighand(p, &flags);
   wake_up_process(p);

   return 0;
}
