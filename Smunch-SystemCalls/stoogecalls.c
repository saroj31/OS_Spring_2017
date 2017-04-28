#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/sched.h>


__sched sleep_on(wait_queue_head_t *q)
{
	unsigned long flags;
	wait_queue_t wait;
	
	init_waitqueue_entry(&wait,current);
	__set_current_state(TASK_UNINTERRUPTIBLE);
	spin_lock_irqsave(&q->lock,flags);
	__add_wait_queue(q,&wait);
	spin_unlock(&q->lock);
	schedule();
	__remove_wait_queue(q,&wait);
	spin_unlock_irqrestore(&q->lock,flags);
	return;
}

SYSCALL_DEFINE1(goober,int,myarg)
{
	printk(KERN_ALERT "Hello from %d\n",myarg);
	return(1);
}

SYSCALL_DEFINE1(init_sigcounter,int,pid){

	unsigned long flags;
	struct task_struct *p;
	int i;
	p = pid_task(find_vpid(pid),PIDTYPE_PID);
	if(NULL == p) 	return 0;

	lock_task_sighand(p,&flags);
	for(i=0;i<64;i++)
		p->sighand->isigcnt[i] = 0;
	
	unlock_task_sighand(p,&flags);
	return 1; 
}

SYSCALL_DEFINE1(get_sigcounter,int,signumber){

	unsigned long flags;
	int cnt = 0;
	struct task_struct *pCurr = current;
	if(NULL == pCurr)	return 0;
	
	lock_task_sighand(pCurr,&flags);
	cnt = pCurr->sighand->isigcnt[signumber];		
		
	unlock_task_sighand(pCurr,&flags);
	return cnt;
}

DECLARE_WAIT_QUEUE_HEAD(gone);

SYSCALL_DEFINE0(deepsleep)
{
	sleep_on(&gone);
}


SYSCALL_DEFINE2(smunch,int,pid,unsigned long,bit_pattern)
{
	//declaration section
	struct task_struct *p= NULL;
	unsigned long flags;
	int ret;
	sigset_t new_set;
	
	//Extract the Process structure from given PID number
	rcu_read_lock();
	p = pid_task(find_vpid(pid),PIDTYPE_PID);
	rcu_read_unlock();
	if(NULL == p) {
		pr_warn("Task not found for pid=%d",pid);
		return -1;
	}
		
	//2. Multithreaded or not traced process checking
	if((NULL == thread_group_empty(p)) || (1 == p->ptrace)){
		pr_warn("Multithreaded or traced process, So ignored\n");
		return -1;
	}

	//3. Check SIGKILL signal and EXIT_ZOMBIE state
	siginitset(&new_set,bit_pattern);//making a set of our given bit_pattern
	
	if(sigismember(&new_set,SIGKILL))
		siginitset(&new_set,sigmask(SIGKILL)); //reinitialized to only KILL
	if(p->exit_state== EXIT_ZOMBIE){
		pr_info("EXIT_ZOMBIE state\n");
		if(sigismember(&new_set,SIGKILL))
			release_task(p);
		return 0;	
	}
	
	if( NULL == lock_task_sighand(p,&flags)){
		pr_warn("smunch: lock_task_sighand failed");
		return -1;
	}	
	
	sigaddsetmask(&(p->signal->shared_pending.signal),bit_pattern);
	set_tsk_thread_flag(p,TIF_SIGPENDING);
	unlock_task_sighand(p,&flags);
	wake_up_process(p);

	return 0;
}










