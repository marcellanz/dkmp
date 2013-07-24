#include <linux/unistd.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/utsname.h>

#include <linux/dkm.h>

/*
 * dkm kernel interface for ikcd (kii)
 */
pid_t dkm_ikcd_pid=-1;			/* pid of the ikcd task */
struct wait_queue *wait_dkm_req = NULL;	/* dkm wait-queue */

/*
 * the working function for the dkmctl() systemcall
 */
int do_dkmctl(int cmd, pid_t pid, int req, struct req_argv* rq_argv)
{
 siginfo_t info;
 struct task_struct* task;
 int found, error;

 error = 0;

 switch(cmd) {
 case DKM_IKCD_REG:	/* ikcd: ikcd register itself with his pid */
			dkm_ikcd_pid = pid;
			error = 0;
			break;

 case DKM_IKCD_UNREG:	/* ikcd: the ikcd can unregister itself */
			dkm_ikcd_pid = -1;
			error = 0;
			break;

 case DKM_GET_REQ:	/* ikcd: find a task with a pending request and return pid and req type of this task*/
			printk("Begin of DKM_GET_REQ\n");
			found = 0;
			for_each_task(task) {
				if(task->dkm != NULL 
				   && task->dkm->pending_req == 1
				   && task->dkm->req_state == REQ_IN_DO_REQ
				   && task->dkm->req >= DKM_MIN_REQ 
				   && task->dkm->req <= DKM_MAX_REQ) {
					printk("DKM_GET_REQ: task->pid is: %d\n", task->pid);
					printk("DKM_GET_REQ: task->dkm->req is: %d\n", task->dkm->req);
					
					put_user(task->pid, (int *)rq_argv->arg0);
					put_user(task->dkm->req, (int *)rq_argv->arg1);
					task->dkm->req_state = REQ_IN_GET_REQ;
					found = 1;
					break;
				} /* if */
			} /* for_each_task */
			if(found == 1) {
				printk("DKM_GET_REQ: found is 1\n");
				printk("End of DKM_GET_REQ\n");
				error = 0;
				break; 
			} /* if */
			printk("DKM_GET_REQ: found is 0\n");
			put_user(-1, (int *)rq_argv->arg0); /* no pending request found */
			put_user(-1, (int *)rq_argv->arg1);
			/* no pending request found ? why did we receive a signal ? */
			printk("End of DKM_GET_REQ\n");
			error = 1; /* nothing found */
			break;

 case DKM_GET_ARGS:	/* ikcd: get the arguments for a request pending task, ikcd has the buffers */
			printk("Begin of DKM_GET_ARGS\n");
			task = find_task_by_pid(pid);		/* find the task with the pending request */
			printk("DKM_GET_ARGS: pid is: %d\n", pid);
			printk("DKM_GET_ARGS: req is: %d\n", req);
			printk("DKM_GET_ARGS: task->pid is: %d\n", task->pid);
			printk("DKM_GET_ARGS: task->dkm->req is: %d\n", task->dkm->req);
			
			if(!task || task->dkm->req != req)	/* ikcd perhaps isn't right; if it so, he has not the right buffers for us */
				return -1;
			switch(req) {
			 case MAP_REQ:		copy_to_user((char* )rq_argv->arg0, task->dkm->file_name, strlen(task->dkm->file_name)+1);	
						break;

			 case PROC_CREAT_REQ:	copy_to_user((char*)rq_argv->arg0, task->dkm->r_node, strlen(task->dkm->r_node)+1);
						copy_to_user((char*)rq_argv->arg1, task->dkm->file_name, strlen(task->dkm->file_name)+1);
						break;

			 default:		break;
			} /* switch */
			printk("End of DKM_GET_ARGS\n");
			error = 0;
			break;

 case DKM_PUT_RESP:	/* ikcd: put the result of the request, the task is prepared to hold the value */
			printk("Begin of DKM_PUT_RESP\n");
			printk("DKM_PUT_RESP: pid is: %d\n", pid);
			printk("DKM_PUT_RESP: req is: %d\n", req);
			task = find_task_by_pid(pid);
			if(!task || task->dkm->req != req)
				return -1;			/* FIXME: ret val to EAGAIN */
			printk("DKM_PUT_RESP: task->pid is: %d\n", task->pid);
			printk("DKM_PUT_RESP: task->dkm->req is: %d\n", task->dkm->req);
			switch(req) {
			 case MAP_REQ:	strncpy_from_user(task->dkm->r_node, (char *)rq_argv->arg0, strlen_user((char *)rq_argv->arg0)+1);
					task->dkm->pending_req = 0;
					task->dkm->req_state = REQ_NOT_IN_PROGRESS;
					wake_up_interruptible(&wait_dkm_req);
					break;

			 case PROC_CREAT_REQ:
					get_user(task->dkm->r_pid, (pid_t*)rq_argv->arg0);
					printk("DKM_PUT_RESP: r_pid is: %d\n", task->dkm->r_pid);
					task->dkm->pending_req = 0;
					task->dkm->req_state = REQ_NOT_IN_PROGRESS;
					wake_up_interruptible(&wait_dkm_req);
					break;

			 default:	return -1;
			} /* switch */
			printk("End of DKM_PUT_RESP\n");
			error = 0;
			break;

 case DKM_DO_REQ:	/* kernel: check if a request is pending, send signal to ikcd and wait */
			if(current->pid == dkm_ikcd_pid
			   || current->dkm->req < DKM_MIN_REQ
			   || current->dkm->req > DKM_MAX_REQ)
				return -1;

			if(dkm_ikcd_pid > 0) {
				info.si_signo = SIGUSR2;
				info.si_errno = 0;
				info.si_code = SI_KERNEL;
				info.si_pid = current->pid;
				info.si_uid = current->uid;
				task = find_task_by_pid(dkm_ikcd_pid);
 				current->dkm->pending_req = 1;
				current->dkm->req_state = REQ_IN_DO_REQ;
				if(task)
					send_sig_info(SIGUSR2, &info, task);
				else break;
				/* sleep on the dkm wait queue until the request is finished */
				while(current->dkm->pending_req != 0)
					interruptible_sleep_on(&wait_dkm_req);
				error = 0;
			} /* if */
			else
				return -1;
			break;

 default: 		break;
 } /* switch */

 return error;
}

/*
 * the following functions are wrappers for different system-calls and functions which
 * dkm uses to communicate with other kernels
 */


/*
 * find the best node to execute the binary given by filename
 */
int dkm_execve(char* filename, char** argv)
{
 int ret;

 if(dkm_ikcd_pid < 0) /* FIXME: while init and other low pid processes are starting, kmalloc() failed */
	return -1;
 current->dkm->file_name = (char*) kmalloc(strlen(filename)+1, GFP_KERNEL);
 if(current->dkm->file_name == NULL)
	panic("dkm_execve: unable to kmalloc for current->dkm->file_name\n");
 current->dkm->r_node = (char*) kmalloc(NODE_LEN, GFP_KERNEL);
 if(current->dkm->r_node == NULL) 
	panic("dkm_execve: unable to kmalloc for current->dkm->r_node\n");
 current->dkm->o_node = (char*) kmalloc(NODE_LEN, GFP_KERNEL);
 if(current->dkm->o_node == NULL) 
	panic("dkm_execve: unable to kmalloc for current->dkm->o_node\n");

 strncpy(current->dkm->file_name, filename, strlen(filename)+1);
 down(&uts_sem);
 strncpy(current->dkm->o_node, system_utsname.nodename, strlen(system_utsname.nodename)+1);
 up(&uts_sem);
 /* 
  * check if the ikcd is registered or the current process isn't the ikcd itself.
  */
 if(dkm_ikcd_pid < 0 || current->pid == dkm_ikcd_pid) {
	strncpy(current->dkm->r_node, current->dkm->o_node, strlen(current->dkm->o_node)+1);
	current->dkm->distributed = 0;
	
	return 1;
 } /* if */

 current->dkm->req = MAP_REQ;
 ret = do_dkmctl(DKM_DO_REQ, 0, 0, NULL);
 current->dkm->req = DKM_NOP;
 /*
  * handle error in do_dkmctl for MAP_REQ, don't distribute it
  */
 if(ret < 0) {
	strncpy(current->dkm->r_node, current->dkm->o_node, strlen(current->dkm->o_node)+1);
	current->dkm->distributed = 0;
	current->dkm->pending_req = 0;
 } /* if */
 else {
	current->dkm->distributed = 1;
	if(strcmp(current->dkm->r_node, current->dkm->o_node) != 0) {
	 	current->dkm->req = PROC_CREAT_REQ;
 		ret = do_dkmctl(DKM_DO_REQ, 0, 0, NULL);
		current->dkm->req = DKM_NOP;
	} /* if */
 } /* else */

 return 0;
}


/* 
 * syscall sys_dkmctl 
 */
asmlinkage int sys_dkmctl(int cmd, pid_t pid, int req, struct req_argv* rq_argv)
{
 int error;
 error = do_dkmctl(cmd, pid, req, rq_argv);

 return error;
}
