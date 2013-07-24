#ifndef _LINUX_DKM_H
#define _LINUX_DKM_H


#define DKM_MAP_REQ     1
#define DKM_IKCD_REG     2
#define DKM_IKCD_UNREG   3
#define DKM_DOSIG       4
#define DKM_MAP_RESP    5

#define READ_REQ        22

#define NODE_LEN	65

/* 
 * dkmctl commands 
 */
#define DKM_GET_REQ     10
#define DKM_GET_ARGS	11
#define DKM_PUT_RESP    12
#define DKM_DO_REQ      13

#define DKM_MIN_REQ 	1
#define DKM_MAX_REQ	10

/* 
 * request types 
 */
#define DKM_NOP		0
#define MAP_REQ		1
#define PROC_CREAT_REQ      10

/*
 * request states
 */
#define REQ_NOT_IN_PROGRESS	0
#define REQ_IN_GET_REQ		1
#define REQ_IN_GET_ARGS		2
#define REQ_IN_PUT_RESP		3
#define REQ_IN_DO_REQ		4
#define REQ_FINISHED		5

struct req_argv {
	void* arg0;
	void* arg1;
	void* arg2;
	void* arg3;
	void* arg4;
	void* arg5;
};

int do_dkmctl(int cmd, pid_t pid, int req, struct req_argv* rq_argv);

struct dkm_struct {
        int distributed;
        int pending_req;
	int req_state;

        int req;
	struct req_argv rq_argv;

	pid_t o_pid;
	pid_t r_pid;

        char* file_name;
        char* o_node;
	char* r_node;
};

#endif
