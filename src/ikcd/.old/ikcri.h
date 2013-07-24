int ikcri_handle_process_creation_request(int sockfd, struct sockaddr* cliaddr, socklen_t cli_len, char* packet);
int ikcri_response_to_client(struct ikc_request* request);
int ikcri_handle_remote_syscall_request(int sockfd, struct msghdr* msg_prep);
int ikcri_proceed_incoming_request(struct ikc_request* request);
