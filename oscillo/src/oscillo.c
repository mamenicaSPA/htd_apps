#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/select.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../../src/websocket.h"
#include "../../src/dma.h"
#include "oscillo.h"

#define GPIO_START	0x00000001
#define GPIO_QUIRY	0x00000002
#define GPIO_READ	0x00000004
#define GPIO_STOP	0x00000008
#define GPIO_TRG1H	0x00000010
#define GPIO_TRG1L	0x00000020
#define GPIO_TRG2H	0x00000040
#define GPIO_TRG2L	0x00000080
#define GPIO_DIVSET	0x00000100
#define GPIO_M_SET	0x00000200
#define GPIO_ZERO	0x00000000
#define GPIO_MASK	0xffff0000
#define GPIO_DTSET(A,B) ((A)|((B)<<16)&0xffff0000)	

#define CMD_START 1
#define CMD_STOP_ 2
#define CMD_ASTOP 3
#define CMD_DTREQ 4
#define CMD_DTGET 5
#define CMD_STACK 6

struct gloval{
	pthread_mutex_t lock;
	pthread_cond_t sig;
	struct config conf;
	int sock;
	int command;
	uint32_t flags;
	char filename[128];
	union hist_data hist;
};

int cmd_decode(char *buf){

	if(strcmp(buf, "START")==0)
		return CMD_START;
	if(strcmp(buf, "STOP_")==0)
		return CMD_STOP_;
	if(strcmp(buf, "ASTOP")==0)
		return CMD_ASTOP;
	if(strcmp(buf, "DTREQ")==0)
		return CMD_DTREQ;
	if(strcmp(buf, "DTGET")==0)
		return CMD_DTGET;
	if(strcmp(buf, "STACK")==0)
		return CMD_STACK;
	return -1;
}

int oscillo_init(struct config conf){
	int fd;
	void *cfg;
	char *name = "/dev/mem";
	uint32_t wtdata;
	uint32_t datacnt;
	char sysarg[128];
	
	//bitstreamの書き込み
	sprintf(sysarg,"cat %s > /dev/xdevcfg",conf.bitstream);
	system(sysarg);
	
	//"dev/mem"(メモリ直セスアクセスファイル)のopen->mmap
	if((fd = open(name, O_RDWR)) < 0) {
    perror("open");
	return -1;
	}
	cfg = mmap(NULL, sysconf(_SC_PAGESIZE), 
             PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);
	
	//各種設定パラメータの書き込み
	wtdata = conf.Htrg1;
	printf("wtdata:%x\n",wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_TRG1H,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_ZERO ,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;

	wtdata = conf.Ltrg1;
	printf("wtdata:%x\n",wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_TRG1L,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_ZERO ,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;

	wtdata = conf.clkdiv1 | conf.clkdiv2<<4;
	printf("wtdata:%x\n",wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_DIVSET,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_ZERO ,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;
	
	wtdata = conf.tau | (conf.div1<<5) | (conf.div2<<10);
	printf("wtdata:%x\n",wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_M_SET,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_ZERO ,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;

	//reset
	*((uint32_t *)(cfg + 0)) = GPIO_START;
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;
	
	usleep(1000);
	
	*((uint32_t *)(cfg + 0)) = GPIO_QUIRY;
	usleep(100);
	datacnt = *((uint32_t *)(cfg + 8))&(~GPIO_MASK);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;
	
	//"dev/mem"のクローズ(初期設定時しか使用しないため)
	munmap(cfg,sysconf(_SC_PAGESIZE));
	close(fd);
	
	return datacnt;
}

void *wsthread(void *p){
	struct gloval *gloval = ((struct gloval*)p);
	
	char buf[1024];		//バッファ(1kB)
	int n;
	
	unsigned char roopflag =1u;
	int command;

	struct ws_sock_t *ws_sock;
	
	int i;
	
	ws_sock = ws_init(gloval->sock);
	
	sprintf(buf,"connected");
	ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
	
	memset(gloval->hist.int32,0,4096);
	/*
	for(i=0;i<4096;i++)
		gloval->hist.int32[i] = i;
	*/
	
	//*******main loop*******//
	while(roopflag){
		command =0;
		//receive
		n = ws_read(ws_sock, buf, sizeof(buf)-1, NULL);
		//printf("n:%08x,%d\n",n,n);
	
		if(n == READ_ERR_CLOSE)
			break;
		
		command = cmd_decode(buf);
		
		switch(command){
			case CMD_START:
				gloval -> flags = 0;
				sprintf(buf,"datatake start");
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				pthread_mutex_lock(&gloval->lock);
				config_recv(ws_sock,&gloval->conf);
				printf("flg:%x\n",gloval->flags);
				if((gloval->flags & OSCFLGS_FPGARST) ==0){
					printf("FPGARSTsig\n");
					gloval->flags |= OSCFLGS_FPGARST;
					pthread_cond_signal(&gloval->sig);
				}
				printf("flg:%x\n",gloval->flags);
				pthread_mutex_unlock(&gloval->lock);
				break;
			case CMD_STOP_:
				sprintf(buf,"datatake stop");
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				pthread_mutex_lock(&gloval->lock);
				gloval->flags |= OSCFLGS_STOP;
				pthread_cond_signal(&gloval->sig);
				pthread_mutex_unlock(&gloval->lock);
				break;
			case CMD_ASTOP:
				sprintf(buf,"program stop");
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				n = ws_read(ws_sock, buf, sizeof(buf)-1, NULL);
				printf("r:%d,d:%d,o:%d\n%s\n",ws_sock->readlen,ws_sock->datalen,n,buf);
				pthread_mutex_lock(&gloval->lock);
				gloval->flags |= OSCFLGS_ASTOP | OSCFLGS_STOP;
				pthread_cond_signal(&gloval->sig);
				pthread_mutex_unlock(&gloval->lock);
				break;
			case CMD_DTREQ:
				pthread_mutex_lock(&gloval->lock);
				if((gloval->flags & OSCFLGS_DTREQ) ==0){
					printf("DTREQsig\n");
					gloval->flags |= OSCFLGS_DTREQ;
					pthread_cond_signal(&gloval->sig);
				}
				ws_write(ws_sock,gloval->hist.byte,4096*4,OPCD_BIN);
				pthread_mutex_unlock(&gloval->lock);
				break;
			case CMD_DTGET:
				sprintf(buf,"file send");
				for(i=0;i<1024;i++)
					gloval->hist.int32[i]++;
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				break;
			case CMD_STACK:
				sprintf(buf,"status OK");
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				break;
			default:
				command = CMD_ASTOP;
			break;
		}
		if(command == CMD_ASTOP)
			roopflag = 0;
	}
	
	ws_close(ws_sock);
	printf("socket close\n");
	return 0;
	
}

void *fpgathread(void*p){
	struct gloval *g = ((struct gloval *)p);
	
	struct DMA_st *dma;
	int command;
	int i;
	uint32_t rdata;
	int data;
	
	uint32_t *gflags = &g->flags;
	pthread_mutex_t *glock = &g->lock;
	pthread_cond_t  *gsig = &g->sig;
	
	//DMA初期化
	dma = DMA_init();
	
	while((*gflags & OSCFLGS_ASTOP) == 0){
		
		pthread_mutex_lock(glock);
		printf("gflg%x\n",*gflags);
		while((*gflags & OSCFLGS_FPGARST)==0)
			pthread_cond_wait(gsig, glock);	//リセットフラグが1になるまで待機(configload中)
		*gflags &= ~OSCFLGS_FPGARST;		//リセットフラグを0にセット
		printf("gflg%x\n",*gflags);
		//TRG,CLK設定,FIFOリセット
		printf("fifo:%04d\n",oscillo_init(g->conf));
		pthread_mutex_unlock(glock);
	
		//初期化が完了

		while((*gflags & OSCFLGS_STOP) == 0){
			pthread_mutex_lock(glock);
			if(DMA_read(dma,4096,(void*)&g->hist,rdata_decode)<0)
				*gflags |= OSCFLGS_STOP|OSCFLGS_ASTOP|OSCFLGS_ERRDMA;
			while((*gflags & (OSCFLGS_DTREQ|OSCFLGS_STOP|OSCFLGS_ASTOP))==0)
				pthread_cond_wait(gsig, glock);	//DTREQがくるまで待機
			*gflags &= ~OSCFLGS_DTREQ;			//フラグを0にセット
			pthread_mutex_unlock(glock);
		}
	}
	DMA_close(dma);
}

int main(int args, char *argv[])//arg1:sock,
{
	pthread_t th_fpga;
	pthread_t th_wsock;
	unsigned char roopflag =1u;
	int command;

	struct gloval g;
	
	int i;
	
	if(args <2){
		printf("you need args\n");
		return -1;
	}
	
	pthread_cond_init(&g.sig,NULL);
	pthread_mutex_init(&g.lock,NULL);
	g.sock = atoi(argv[1]);
	
	pthread_create(&th_wsock,NULL,&wsthread,&g);
	pthread_create(&th_fpga,NULL,&fpgathread,&g);
	
	pthread_join(th_wsock,NULL);
	pthread_join(th_fpga,NULL);
	
	printf("joind child\n");
	
	return 0;
}