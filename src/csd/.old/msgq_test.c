#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include "csda.h"


int main(void)
{
 int client_id, server_id; 
 struct bn_request req;
 struct bn_response resp;


 server_id = msgget(7768, 0);
 client_id = msgget(6877, S_IRWXU | S_IWGRP | S_IWOTH | IPC_CREAT);

 req.mtype = 1;
 sprintf(req.msg, "%d %s", client_id, "/bin/sh");
 msgsnd(server_id, &req, 256, 0);

 msgrcv(client_id, &resp, 65, 0, 0);

 printf("best node for app: /bin/sh is: %s\n", resp.msg); 

 msgctl(client_id, IPC_RMID, NULL);

 return 0;
}
