
#ifndef _MYFUNC_H_
#define _MYFUNC_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <log_utils.h>
#include <netinet/ip.h>
#include <file_opt.h>
#include <cJSON.h>
#include <list.h>
#include <shmem.h>
#include <pthread.h>
#include <stdbool.h>
#include <msg_queue_peer.h>


int conver_endian_long(unsigned char *dst, const unsigned char *src, int len);
int my_connect(char *ipaddr, int port);
float my_read_registers(int sockfd, int id, int function, int addr, int num);
int coil_status(int sockfd, int id, int function, int addr, int num);
int my_write_coil(int socket, int id, int function, int addr, int value);
int my_write_register(int sockfd, int id, int function, int addr, float value);

#endif