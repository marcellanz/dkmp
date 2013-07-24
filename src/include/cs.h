/*
 * cs.h: the main cs header file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 4096
#define CSD_PORT 7678
#define CSD_MCAST_GROUP	"224.0.1.178"
#define CSD_UPDATERATE 10000000
#define CSD_HELLORATE 5000000
#define KSIZE	256
#define VSIZE	256
#define NODE_SIZE	65
#define APP_SIZE	256

#define CSP_MAXPACKET_SIZE	65535

#define CS_DEF_DB_REQ_PATH "./def_req.rc"
#define CS_REGISTERED_APPS_PATH "./reg_apps.rc"
#define CS_DB_DUMP_PATH		"/tmp/db_dump"

#define CS_HELLO	1
#define CS_REQ		2
#define CS_CAP		3
#define CS_VER		1

#define CS_SET		1
#define CS_GET		2
#define CS_DEL		3
#define CS_LS		4
#define CS_NOP		5
#define CS_CAPEXEC	6

#define CS_RESP_SUCC	1
#define CS_RESP_ERR	2
#define CS_RESP_WDATA	3
#define CS_RESP_NDATA	4

#define CS_PACKET_HDR_SIZE	2
#define CSP_CAPREQ_HDR_SIZE	5
#define CSP_DBREQ_HDR_SIZE	5

#define ECS_NOENTRY 	1
#define ECS_NOCHILDS 	2
#define ECS_NOVALUE	3
#define ECS_NOLEAF	4
#define ECS_INVALIDKEY	5

#define MSQ_MAX_NAME            257
#define MSQ_MAX_NODE            65

#define CS_LOG		0
#define CS_VER		1
