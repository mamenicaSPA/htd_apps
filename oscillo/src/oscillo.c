#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/select.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "websocket.h"

#define GPIO_START	0x00001001
#define GPIO_QUIRY	0x00001002
#define GPIO_READ	0x00001004
#define GPIO_STOP	0x00001008
#define GPIO_TRG1H	0x00001010
#define GPIO_TRG1L	0x00001020
#define GPIO_TRG2H	0x00001040
#define GPIO_TRG2L	0x00001080
#define GPIO_DIVSET	0x00001100
#define GPIO_M_SET	0x00001200
#define GPIO_ZERO	0x00001000
#define GPIO_MASK	0xffff0000
#define GPIO_DTSET(A,B) ((A)|((B)<<16)&0xffff0000)	

#define CMD_START 1
#define CMD_STOP_ 2
#define CMD_ASTOP 3
#define CMD_DTREQ 4
#define CMD_DTGET 5
#define CMD_STACK 6

union hist_data{
	unsigned int int32[4096];
	unsigned char byte[4096*4];
};

struct gloval{
	pthread_mutex_t lock;
	pthread_cond_t sig;
	struct config *conf;
	int sock;
	int command;
	uint32_t flags;
	char filename[128];
	union hist_data hist;
};

void debug_mem(void *cfg,int len)
{
	unsigned int i;
	for(i=0;i<len;i++)
		printf("%02x:%08x\n",4*i,*((uint32_t *)(cfg+4*i)));
}

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

int oscillo_init(){
	int fd;
	void *cfg;
	char *name = "/dev/mem";
	uint32_t wtdata;
	uint32_t clkdiv1 = 0;
	uint32_t clkdiv2 = 2;
	uint32_t tau = 0;
	uint32_t div = 6;
	uint32_t sel = 2;
	uint32_t datacnt;
	
	if((fd = open(name, O_RDWR)) < 0) {
    perror("open");
	return -1;
	}
	cfg = mmap(NULL, sysconf(_SC_PAGESIZE), 
             PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);
			 
	wtdata = 0x100;		 
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_TRG1H,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_ZERO ,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;

	wtdata = 0x10;
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_TRG1L,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_ZERO ,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;

	wtdata = clkdiv1 | clkdiv2<<4;
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_DIVSET,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_DTSET(GPIO_ZERO ,wtdata);
	*((uint32_t *)(cfg + 0)) = GPIO_ZERO;
	
	wtdata = tau | (div<<5) | (sel<<10);
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
	
	munmap(cfg,sysconf(_SC_PAGESIZE));
	close(fd);
	
	return datacnt;
}

void *wsthread(void *p){
	struct gloval *gloval = ((struct groval*)p);
	
	char buf[1024];		//バッファ(1kB)
	int n;
	
	union hist_data histgram;
	unsigned char roopflag =1u;
	int command;

	struct ws_sock_t *ws_sock;
	
	int i;
	
	ws_sock = ws_init(gloval->sock);
	
	sprintf(buf,"connected");
	ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
	
	memset(histgram.int32,0,4096);
	for(i=0;i<4096;i++)
		histgram.int32[i] = i;
	
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
				sprintf(buf,"datatake start");
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				pthread_mutex_lock(&gloval->lock);
				groval->conf = configload(ws_sock);
				if(gloval->flags & OSCFLGS_FPGARST ==0){
					gloval->flags |= OSCFLGS_FPGARST;
					pthread_cond_signal(gloval->sig);
				}
				pthread_mutex_unlock(&gloval->lock);
				break;
			case CMD_STOP_:
				sprintf(buf,"datatake stop");
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				break;
			case CMD_ASTOP:
				sprintf(buf,"program stop");
				ws_write(ws_sock,buf,strlen(buf),OPCD_TEXT);
				n = ws_read(ws_sock, buf, sizeof(buf)-1, NULL);
				printf("r:%d,d:%d,o:%d\n%s\n",ws_sock->readlen,ws_sock->datalen,n,buf);
				break;
			case CMD_DTREQ:
				ws_write(ws_sock,histgram.byte,4096*4,OPCD_BIN);
				break;
			case CMD_DTGET:
				sprintf(buf,"file send");
				for(i=0;i<1024;i++)
					histgram.int32[i]++;
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
	struct gloval *gloval = ((struct groval *)p);
	
	struct DMA_st *dma;
	int command;
	int i;
	uint32_t rdata;
	int data;
	
	uint32_t *gflags = &gloval->flags;
	pthread_mutex_t *glock = &gloval->lock;
	pthread_cond_t  *gsig = &gloval->sig;
	
	//DMA初期化
	dma = DMA_init();
	
	pthread_mutex_lock();
	while((*gflags & OSCFLGS_FPGARST)==0)
		pthread_cond_wait(gsig, glock);
	*gflags &= ~OSCFLGS_FPGARST;
	//TRG,CLK設定,FIFOリセット
	printf("fifo:%04d\n",oscillo_init());
	pthread_mutex_unlock;
	
	//初期化が完了

	

	dma->S2MM_DMACR->bit.RS = 1;
	*dma->S2MM_LENGTH = 4096;
	
	while(1){
		if(dma->S2MM_DMASR->bit.Idle == 1)
			break;
		if(dma->S2MM_DMASR->bit.DMAIntErr==1){
			printf("IntErr\n");
			break;
		}
			
		if(dma->S2MM_DMASR->bit.DMASlvErr==1){
			printf("SlvErr\n");
			break;
		}
	}
	
	printf("dataread:%d\n",*dma->S2MM_LENGTH);

	pthread_mutex_lock(glock);
	for(i=0;i < *dma->S2MM_LENGTH /4;i++){
		rdata=*((uint32_t *)(dma->dmabuf + 4*i));
		data = (rdata&0x00002000)? rdata|0xffffc000:rdata&0x00003fff;
		printf("%04d,0x%08x,%d\n",i,rdata,data);
	}
	pthread_mutex_unlock(glock);
	
	DMA_close(dma);
}

int main(int args, char *argv[])//arg1:sock,
{
	char buf[1024];		//バッファ(1kB)
	int n;
	
	union hist_data histgram;
	unsigned char roopflag =1u;
	int command;

	struct ws_sock_t *ws_sock;
	
	int i;
	
	//if unit test mode make socket
	if(args <2){
		printf("you mast args\n");
		return -1;
	}

	ws_sock = ws_init(atoi(argv[1]));
	return 0;
}