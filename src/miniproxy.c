/*****************************************************************************/
/*                                                                           */
/* miniproxy : mini proxy server                                             */
/*                                                                           */
/*  Copyright (C) 2021 Kuzmin Yaroslav <kuzmin.yaroslav@gmail.com>           */
/*                                                                           */
/* miniproxy is free software: you can redistribute it and/or modify it      */
/* under the terms of the GNU General Public License as published by the     */
/* Free Software Foundation, either version 3 of the License, or             */
/* (at your option) any later version.                                       */
/*                                                                           */
/* miniproxy is distributed in the hope that it will be useful, but          */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                      */
/* See the GNU General Public License for more details.                      */
/*                                                                           */
/* You should have received a copy of the GNU General Public License along   */
/* with this program.  If not, see <http://www.gnu.org/licenses/>.           */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "miniproxy.h"
#include "thread_poll.h"

/*****************************************************************************/

static int listen_socket;

static const char * programm_name = NULL;

static char PID_MINIPROXY_STR[] = "PID_MINIPROXY";

static const char * const short_options = "l:d:hkV";
static const struct option long_options[] = {{"local",        1, NULL, 'l'},
                                             {"destination",  1, NULL, 'd'},
                                             {"kill",         0, NULL, 'k'},
                                             {"help",         0, NULL, 'h'},
                                             {"version",      0, NULL, 'V'},
                                             {NULL,      0, NULL,  0 }};

static int print_help(FILE * stream)
{
	fprintf(stream, "Use : %s -l <local adress:local port> -d <destination adress:destination port>\n", programm_name);
	fprintf(stream, "  -l   --local        : local adress and port\n"
	                "  -d   --destination  : destination adress and port\n"
	                "  -k   --kill         : shutdown proxy\n"
	                "  -h   --help         : help\n"
	                "  -V   --version      : version programm\n");
	return SUCCESS;
}

#define VERSION_GIT     0xffffffff
static char VERSION[] = "0.00-ffffffff\n";

static int print_version(FILE * stream)
{
	sprintf(VERSION, "%01d.%02d-%08x\n", VERSION_MAJOR, VERSION_MINOR, VERSION_GIT);
	fprintf(stream, "%s", VERSION);
	return SUCCESS;
}

int set_nonblock(int fd)
{
	int val;

	val = fcntl(fd, F_GETFL);
	if (val == -1) {
		printf("fcntl(%d, F_GETFL): %s\n", fd, strerror(errno));
		return FAILURE;
	}
	if (val & O_NONBLOCK) {
		return SUCCESS;
	}
	val |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, val) == -1) {
		printf("fcntl(%d, F_SETFL, O_NONBLOCK): %s\n", fd, strerror(errno));
		return FAILURE;
	}
	return SUCCESS;
}

/*****************************************************************************/
#define BUF_SIZE    1024
#define TIMEOUT_DISCONNECT       (30 * 1000)   // 30 sec

static char * request_dest_addr;
static size_t request_dest_addr_size;
static char * request_local_addr;
static size_t request_local_addr_size;

static int init_request_dest(const char * dest_addres, const char * dest_port)
{
	size_t size_addr_str = strlen(dest_addres);
	size_t size_port_str = strlen(dest_port);

	request_dest_addr_size = 6 + size_addr_str + 1 + size_port_str + 2; // Host: <local addres>:<local port>\n\r
	request_dest_addr = calloc(request_dest_addr_size+1, sizeof(char));
	strcat(request_dest_addr, "Host: ");
	strcat(request_dest_addr, dest_addres);
	strcat(request_dest_addr, ":");
	strcat(request_dest_addr, dest_port);
	strcat(request_dest_addr, "\r\n");
	printf("Request dest (%ld):> %s\n", request_dest_addr_size, request_dest_addr);

	return SUCCESS;
}

static int init_request_local(const char * local_addres, const char * local_port)
{
	size_t size_addr_str = strlen(local_addres);
	size_t size_port_str = strlen(local_port);

	request_local_addr_size = 6 + size_addr_str + 1 + size_port_str + 2; // Host: <local addres>:<local port>\n\r
	request_local_addr = calloc(request_local_addr_size+1, sizeof(char));
	strcat(request_local_addr, "Host: ");
	strcat(request_local_addr, local_addres);
	strcat(request_local_addr, ":");
	strcat(request_local_addr, local_port);
	strcat(request_local_addr, "\r\n");

	printf("Request local (%ld):> %s\n", request_local_addr_size, request_local_addr);
	return SUCCESS;
}

static int deinit_request(void)
{
	if (request_dest_addr != NULL) {
		free(request_dest_addr);
		request_dest_addr = NULL;
	}
	if (request_local_addr != NULL) {
		free(request_local_addr);
		request_local_addr = NULL;
	}
	return SUCCESS;
}

static int change_request(uint8_t ** buf_thread, int len)
{
	char * buf = (char *)*buf_thread;
	char *str_begin;
	int diff_str = (int)request_local_addr_size - (int)request_dest_addr_size;
	size_t off_len;
	buf[len] = 0;
	printf("0---\n");
	printf("%s", buf);
	printf("1---\n");

	str_begin = strstr((const char*)buf, request_local_addr);
	if (str_begin != NULL) {
		printf("%s", str_begin);
	printf("2---\n");

		off_len = str_begin - (char*)buf;
	printf("%ld\n", off_len);
		if(diff_str == 0) {
			memmove(str_begin, request_dest_addr, request_dest_addr_size);
		}
		else {
			char * p_dest = (char*)(buf + off_len + request_dest_addr_size);
			char * p_src = (char*)(buf + off_len + request_local_addr_size);
	printf("3---\n");
	printf("%s", p_dest);
	printf("4---\n");
	printf("%s", p_src);
	printf("5---\n");

			size_t size = len;
			size -= off_len;
			size -= request_local_addr_size;
			printf("%ld\n", size);
	printf("6---\n");

			memmove(p_dest, p_src, size);
			memmove(str_begin, request_dest_addr, request_dest_addr_size);
		}

		len -= diff_str;
		buf[len] = 0;
	}
	return len;
}

static void * process_pipe(void * arg)
{
	int * array = arg;
	int local_fd = array[0];
	int dest_fd = array[1];
	//uint8_t buf[BUF_SIZE+1] = {0};
	int rc;
	struct pollfd sockets_fd[2];
	int timeout = TIMEOUT_DISCONNECT;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	uint8_t * buf = malloc(BUF_SIZE + 1);

	for(;;) {
		sockets_fd[0].fd = local_fd;
		sockets_fd[0].events = POLLIN;
		sockets_fd[0].revents = 0;

		sockets_fd[1].fd = dest_fd;
		sockets_fd[1].events = POLLIN;
		sockets_fd[1].revents = 0;

		timeout = TIMEOUT_DISCONNECT;
		rc = poll(sockets_fd, 2, timeout);
		if (rc == -1) {
			fprintf(stderr, "Error poll : %s\n", strerror(errno));
			break;
		}
		if (rc == 0) {
			printf("Lost connect\n");
			break;
		}

		int revents = sockets_fd[0].revents;

		if (revents & POLLIN) {
			rc = recv(local_fd, buf, BUF_SIZE, 0);
			if (rc == -1) {
				fprintf(stderr, "Error read local_fd\n");
				break;
			}
			if (rc == 0) {
				fprintf(stderr, "Lost local connect\n");
				break;
			}
			printf("read local_fd %d\n", rc);
			rc = change_request(&buf, rc);
			printf("+++\n");
			printf("%d\n", rc);
			printf("%s", buf);

			rc = send(dest_fd, buf, rc, 0);
			if (rc == -1 ) {
				fprintf(stderr, "Error write dest_fd\n");
				break;
			}
			if (rc == 0) {
				fprintf(stderr, "Lost dest connect\n");
				break;
			}
			//printf("write dest_fd %d\n", rc);
		}

		revents = sockets_fd[1].revents;
		if (revents & POLLIN) {
			rc = recv(dest_fd, buf, BUF_SIZE, 0);
			if (rc == -1) {
				fprintf(stderr, "Error read dest_fd\n");
				break;
			}
			if (rc == 0) {
				fprintf(stderr, "Lost dest connect\n");
				break;
			}
			rc = send(local_fd, buf, rc, 0);
			if (rc == -1 ) {
				fprintf(stderr, "Error write local_fd\n");
				break;
			}
			if (rc == 0) {
				fprintf(stderr, "Lost local connect\n");
				break;
			}
		}
	}
	free(buf);
	close(local_fd);
	close(dest_fd);
	printf("thread exit\n");
	fflush(stdout);
	fflush(stderr);
	pthread_exit(NULL);
}

static int thread_array[2];
static int create_pipe(int local_socket_fd, s_dest_info * dest_info)
{
	int rc;
	int dest_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (dest_socket_fd < 0) {
		fprintf(stderr, "Can not create socket\n");
		return FAILURE;
	}

	rc = connect(dest_socket_fd, (struct sockaddr*)&dest_info->addr, sizeof(dest_info->addr));
	if (rc != 0 ) {
		fprintf(stderr, "Can not connect %s : %s\n", dest_info->host_str, strerror(errno));
		return FAILURE;
	}

	rc = set_nonblock(dest_socket_fd);
	if(rc == -1){
		fprintf(stderr, "Can not setting dest socket : %s", strerror(errno));
		close(dest_socket_fd);
		close(local_socket_fd);
		return FAILURE;
	}

	thread_array[0] = local_socket_fd;
	thread_array[1] = dest_socket_fd;

	printf("connect success %s : %d : %d\n", dest_info->host_str, local_socket_fd, dest_socket_fd);
	add_thread(process_pipe, thread_array);

	return SUCCESS;
}

/*****************************************************************************/
static int check_dest(s_dest_info * dest_info)
{
	int rc;
	struct hostent * host_info;
	char ** host_addr_list;

	dest_info->host_str = dest_info->addr_str;
	dest_info->port_str = strchr(dest_info->host_str, ':');
	if (dest_info->port_str == NULL) {
		fprintf(stderr, "Port string not specified\n");
		return FAILURE;
	}
	*dest_info->port_str = 0;
	++dest_info->port_str;

	dest_info->port = strtoul(dest_info->port_str, NULL, 10);
	if (dest_info->port == 0) {
		fprintf(stderr, "Port not specified\n");
		return FAILURE;
	}

	init_request_dest((const char *)dest_info->host_str, (const char *)dest_info->port_str);

	dest_info->addr.sin_family = AF_INET;
	dest_info->addr.sin_port = htons(dest_info->port);

	host_info = gethostbyname(dest_info->host_str);
	if (host_info == NULL) {
		fprintf(stderr, "Can not get host ip adrress\n");
		return FAILURE;
	}

	host_addr_list = host_info->h_addr_list;

	printf("Name host : %s\n", host_info->h_name);

	for (;*host_addr_list != NULL;) {
		char * host_addr = *host_addr_list;
		char * host_addr_ip = inet_ntoa(*((struct in_addr*)host_addr));

		rc = inet_pton(AF_INET, host_addr_ip, &dest_info->addr.sin_addr);
		if (rc > 0)
			break;
		else
			fprintf(stderr, "Can not convert addres %s\n", dest_info->addr_str);

		++host_addr_list;
	}
	if (rc <= 0)
		return FAILURE;

	printf("Convert host %s : port %s\n", dest_info->host_str, dest_info->port_str);

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		fprintf(stderr, "Can not create socket\n");
		return FAILURE;
	}

	rc = connect(socket_fd, (struct sockaddr*)&dest_info->addr, sizeof(dest_info->addr));
	if (rc != 0 ) {
		fprintf(stderr, "Can not connect %s : %s\n", dest_info->host_str, strerror(errno));
		return FAILURE;
	}

	printf("connect success %s\n", dest_info->host_str);

	close(socket_fd);
	return SUCCESS;
}

static int listen_local(char * local_addres)
{
	int rc;
	char * host_str = local_addres;
	char * port_str;
	int port;
	struct hostent * host_info;
	char ** host_addr_list;
	struct sockaddr_in addr = {0};

	port_str = strchr(host_str, ':');
	if (port_str == NULL) {
		fprintf(stderr, "Port string not specified\n");
		return FAILURE;
	}
	*port_str = 0;
	++port_str;

	port = strtoul(port_str, NULL, 10);
	if (port == 0) {
		fprintf(stderr, "Port not specified\n");
		return FAILURE;
	}
	init_request_local((const char*)host_str, (const char*)port_str);

	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == -1) {
		fprintf(stderr, "Can not create socket : %s\n", strerror(errno));
		return FAILURE;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	host_info = gethostbyname(host_str);
	if (host_info == NULL) {
		fprintf(stderr, "Can not get host ip adrress\n");
		close(listen_socket);
		return FAILURE;
	}

	host_addr_list = host_info->h_addr_list;

	printf("Name host : %s\n", host_info->h_name);

	for (;*host_addr_list != NULL;) {
		char * host_addr = *host_addr_list;
		char * host_addr_ip = inet_ntoa(*((struct in_addr*)host_addr));

		rc = inet_pton(AF_INET, host_addr_ip, &addr.sin_addr);
		if (rc > 0)
			break;
		else
			fprintf(stderr, "Can not convert addres %s\n", local_addres);

		++host_addr_list;
	}
	if (rc <= 0) {
		close(listen_socket);
		return FAILURE;
	}

	rc = bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr));
	if (rc == -1) {
		fprintf(stderr, "Can not bind local socket : %s\n", strerror(errno));
		close(listen_socket);
		return FAILURE;
	}

	rc = listen(listen_socket, 3);
	if (rc == -1) {
		fprintf(stderr, "Can not listen local socket : %s\n", strerror(errno));
		close(listen_socket);
		return FAILURE;
	}
/*
	rc = set_nonblock(listen_socket);
	if(rc == -1){
		fprintf(stderr, "Can not setting listen socket : %s\n", strerror(errno));
		close(listen_socket);
		return FAILURE;
	}
	*/
	printf("Listen socket\n");
	return SUCCESS;
}

/*****************************************************************************/
static void close_proxy(int act)
{
	printf("Close proxy\n");
	if (listen_socket != 0)
		close(listen_socket);

	deinit_thread_poll();
	deinit_request();

	unsetenv(PID_MINIPROXY_STR);
	printf("unset %s;\n", PID_MINIPROXY_STR);
}

/*****************************************************************************/
static int set_signals(void)
{
	struct sigaction act;
	sigset_t set;

	/*signal action handler setup*/
	memset(&act, 0x0, sizeof(act));
	if(sigfillset(&set) < 0){
		perror("sigfillset failed");
		return FAILURE;
	}

	act.sa_handler = SIG_IGN;
	if(sigaction(SIGHUP, &act, NULL) < 0){
		perror("sigaction failed SIGHUP");
		return FAILURE;
	}
	if(sigaction(SIGPIPE, &act, NULL) < 0){
		perror("sigaction failed SIGPIPE");
		return FAILURE;
	}
	if(sigaction(SIGCHLD, &act, NULL) < 0){
		perror("sigaction failed SIGCHLD");
		return FAILURE;
	}

	if(sigaction(SIGALRM, &act, NULL) < 0){
		perror("sigaction failed SIGALRM");
		return FAILURE;
	}

	act.sa_handler = close_proxy;
	if(sigaction(SIGQUIT, &act, NULL) < 0){
		perror("sigaction failed SIGQUIT");
		return FAILURE;
	}
	if(sigaction(SIGINT, &act , NULL) < 0){
		perror("sigaction failed SIGINT");
		return FAILURE;
	}
	if(sigaction(SIGTERM, &act, NULL) < 0){
		perror("sigaction failed SIGTERM");
		return FAILURE;
	}
	if(sigaction(SIGABRT, &act, NULL) < 0){
		perror("sigaction failed SIGTERM");
		return FAILURE;
	}

	act.sa_handler = SIG_DFL;
	if(sigaction(SIGBUS, &act, NULL) < 0){
		perror("sigaction failed SIGBUS");
		return FAILURE;
	}
	if(sigaction(SIGFPE, &act, NULL) < 0){
		perror("sigaction failed SIGFPE");
		return FAILURE;
	}
	if(sigaction(SIGILL, &act, NULL) < 0){
		perror("sigaction failed SIGILL");
		return FAILURE;
	}
	if(sigaction(SIGSEGV, &act, NULL) < 0){
		perror("sigaction failed SIGSEGV");
		return FAILURE;
	}
	if(sigaction(SIGXCPU, &act, NULL) < 0){
		perror("sigaction failed SIGCPU");
		return FAILURE;
	}
	if(sigaction(SIGXFSZ, &act, NULL) < 0){
		perror("sigaction failed SIGFSZ");
		return FAILURE;
	}
	if(sigaction(SIGPWR, &act, NULL) < 0){
		perror("sigaction failed SIGPWR");
		return FAILURE;
	}
	if(sigaction(SIGSYS, &act, NULL) < 0){
		perror("sigaction failed SIGSYS");
		return FAILURE;
	}
	if(sigaction(SIGIO, &act, NULL) < 0){
		perror("sigaction failed SIGIO");
		return FAILURE;
	}

	if(sigemptyset(&set) < 0){
		perror("sigemptyset failed");
		return FAILURE;
	}
	return SUCCESS;
}

/*****************************************************************************/
#if 0
TODO добавление переменой окружения
#define SIZE_PID_STR      11
static int set_pid_str(pid_t pid)
{
	char pid_str[SIZE_PID_STR] = {0};
	snprintf(pid_str, SIZE_PID_STR, "%d", pid);
	//setenv(PID_MINIPROXY_STR, pid_str, 1);
	printf("%s=%s; export %s;\n", PID_MINIPROXY_STR, pid_str, PID_MINIPROXY_STR);
	return SUCCESS;
}
#endif

static void kill_proxy(void)
{
	pid_t pid;
	char * value = getenv(PID_MINIPROXY_STR);
	if (value != NULL) {
		pid = strtoul(value, NULL, 10);
		kill(pid, SIGTERM);
		unsetenv(PID_MINIPROXY_STR);
	}
	exit(SUCCESS);
}

/*****************************************************************************/
static char * local_addres = NULL;

static s_dest_info dest_info;

static int proxy_stop = 0;

int main(int argc,char * argv[])
{
	int rc;
	int next_option = 0;

	programm_name = argv[0];

	for (;next_option != -1;) {
		next_option = getopt_long(argc, argv, short_options, long_options, NULL);
		switch(next_option){
			case 'l':
				local_addres = optarg;
				break;
			case 'd':
				dest_info.addr_str = optarg;
				break;
			case 'k':
				proxy_stop = 1;
				break;
			case -1:
				break;
			case 'V':
				print_version(stdout);
				return SUCCESS;
			case 'h':
			case '?':
			default:
				print_help(stdout);
				return SUCCESS;
				break;
		}
	}

	if (proxy_stop)
		kill_proxy();
#if 0
	TODO
	char * value_pid = getenv(PID_MINIPROXY_STR);
	if (value_pid != NULL) {
		fprintf(stderr, "Miniproxy is already running!\n");
		return FAILURE;
	}
#endif

	if (local_addres == NULL) {
		print_help(stdout);
		return FAILURE;
	}

	if (dest_info.addr_str == NULL) {
		print_help(stdout);
		return FAILURE;
	}

	rc = set_signals();
	if (rc != SUCCESS) {
		//TODO add error code
		fprintf(stderr, "can not run proxy : 001\n");
		return FAILURE;
	}

	rc = init_thread_poll();
	if (rc != SUCCESS)
		return rc;

	rc = check_dest(&dest_info);
	if (rc != SUCCESS)
		return rc;

	rc = listen_local(local_addres);
	if (rc != SUCCESS)
		return rc;
/*
	pid_t pid = fork();
	if ( pid != 0) {
		printf("Start miniproxy : pid %d\n", pid);
		set_pid_str(pid);
		exit(SUCCESS);
	}
	// TODO add log file
*/
	for(;;) {

		struct pollfd poll_socket = {.fd = listen_socket, .events = POLLIN, .revents = 0};
		rc = poll(&poll_socket, 1, -1);
		if (rc == -1) {
			if (errno != EINTR)
				fprintf(stderr, "Error wait connect : %s\n", strerror(errno));
			break;
		}

		struct sockaddr_in client_name;
		socklen_t client_name_len = sizeof(client_name);
		//printf("revents %x\n", poll_socket.revents);
		if (poll_socket.revents & POLLIN) {
			int sock_fd = accept(listen_socket, (struct sockaddr*)&client_name, &client_name_len);
			if (sock_fd == -1) {
				fprintf(stderr, "Can not accept client : %s\n", strerror(errno));
				break;
			}
			rc = set_nonblock(sock_fd);
			if(rc == -1){
				fprintf(stderr, "Can not setting local socket : %s", strerror(errno));
				close(sock_fd);
				continue;
			}
			create_pipe(sock_fd, &dest_info);
		}
	}

	return SUCCESS;
}

/*****************************************************************************/
