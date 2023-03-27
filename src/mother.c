#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <pthread.h>

#define PROG_TEST 	 "/opt/redpitaya/www/apps/htd_apps/test/test"
#define PROG_OSCILLO "/opt/redpitaya/www/apps/htd_apps/oscillo/oscillo"
#define PROG_MCAMCS  "/opt/redpitaya/www/apps/htd_apps/mcamcs/mcamcs"

#define PORT_DEBUG	 10000
#define PORT_TEST	 10001
#define PORT_OSCILLO 10002
#define PORT_MCAMCS  10003

struct thread_args{
	pthread_t th;
	int id;
	char *prog;
	int   port;
};

void *soc_fork_thread(void *p)
{
	
	struct thread_args *thread_args = (struct thread_args*)p;

	struct sockaddr_in server;
	struct sockaddr_in client;
	int len;
	int sock;
	int sock_m;
	int pid;
	
	char data[16];

	char *argv[3];
	

	sock_m = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(thread_args->port);
	server.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_m, (struct sockaddr *)&server, sizeof(server));
	listen(sock_m, 5);

	len = sizeof(client);
	
	while(1){
		sock = accept(sock_m, (struct sockaddr *)&client, &len);
		
		sprintf(data,"%d",sock);
	
		argv[0] = thread_args->prog;
		argv[1] = data;
		argv[2] = NULL;
		printf("sock:%d,%s\n",sock,argv[1]);
	
		pid = fork();
		printf("myID:%d\n",pid);

		if(pid==0){
			printf("pid is 0\n");
			//execv("/bin/echo",argv);
			execv(thread_args->prog,argv);
			printf("exec erroer\n");
		}
		thread_args->id = pid;
		waitpid(pid,NULL,0);

		printf("end\n");
		/*ソケットの終了*/
	}
	
	close(sock_m);	
}

int debug()	//ソケットへのアクセスをシグナルとして利用(即close,親スレッドが死ぬので子スレッドも即死)
{
	struct sockaddr_in server;
	struct sockaddr_in client;
	int len;
	int sock;
	int sock_m;
	char buf[] = "mother stop\n";
	
	printf("debug start\n");

	sock_m = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(PORT_DEBUG);
	server.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_m, (struct sockaddr *)&server, sizeof(server));
	listen(sock_m, 5);
	len = sizeof(client);
	sock = accept(sock_m, (struct sockaddr *)&client, &len);
	
	printf("accepted kill sig\n");
	
	write(sock,buf,strlen(buf));
	
	close(sock);
	close(sock_m);
	return 0;
}

int main()
{
	struct thread_args th_test;
	struct thread_args th_osci;
	struct thread_args th_mcas;
	
	//daemon()
	/*
	if (daemon(0, 0) != 0) {
    return 1;
	}
	*/
	
	th_test.prog = strdup(   PROG_TEST);
	th_osci.prog = strdup(PROG_OSCILLO);
	th_mcas.prog = strdup( PROG_MCAMCS);
	
	th_test.port = PORT_TEST;
	th_osci.port = PORT_OSCILLO;
	th_mcas.port = PORT_MCAMCS;
	
	printf("start thread\n");
	pthread_create(&th_test.th,NULL,&soc_fork_thread,&th_test);
	pthread_create(&th_osci.th,NULL,&soc_fork_thread,&th_osci);
	pthread_create(&th_mcas.th,NULL,&soc_fork_thread,&th_mcas);
	
	debug();
	
	//pthread_join(th_test.th, NULL);
	//pthread_join(th_osci.th, NULL);
	//pthread_join(th_mcas.th, NULL);
	
	return 0;
}