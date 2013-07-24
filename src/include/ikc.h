/*
 * ikc.h: the main ikc header file
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <asm/ioctls.h>		/* for FIONREAD */
#include <linux/dkm.h>

#define E_IKC_NOMEM		1
#define E_IKC_NOQUEUE		2
#define E_IKC_NOREQ		3

#define IKC_REQ_PENDING		1
#define IKC_REQ_INPROGRESS 	2
#define IKC_REQ_DONE		3

#define IKC_PORT		7876
#define IKC_LOG			0
#define IKC_VERSION		1
#define IKC_MAX_PACKET		64*1024
#define IKC_PACKET_HDR_LEN	2*sizeof(char)

#define IKC_PROC_CREAT_REQ	10
#define IKC_SYSCALL_REQ		11

#define IKC_PROC_CREAT_RESP	30

#define IKC_MIN_PACKET_SIZE	2*sizeof(char)

#define IKC_RESP_OUT_SELF	1
#define IKC_RESP_IN_SELF	1

#define IKC_MSQ_ALARM		1

#define IKC_MAX_NODE_LEN	65
#define IKC_MAX_FILE_LEN	257

#define IKC_MAX_ATTEMPTS	2


/*
 * a system call request for a remote node
 */
struct ikc_request {
	struct sockaddr_in* cliaddr;
	socklen_t cli_len;
	unsigned int request_nbr;
	unsigned short int host_id;
	unsigned char thread_id;
	unsigned short int seq_nbr;

	int req;
	char o_node[IKC_MAX_NODE_LEN];
	char r_node[IKC_MAX_NODE_LEN];
	pid_t o_pid;
	pid_t r_pid;

	struct req_argv rq_argv;
};

int __ikc_free_request(struct ikc_request* req);

/*
 * this is the ikc request queue for all outgoing requests
 * different threads have access on this queue, so all 
 * operations on this queue are synchronized
 */
struct ikc_request_queue {
	struct ikc_request* req_struct;
	struct ikc_request_queue* next;
	struct ikc_request_queue* prev;
};

struct ikc_queue_head {
	struct ikc_request_queue* queue;
	pthread_mutex_t* mutex;
};
