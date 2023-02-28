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



void debug_mem(void *cfg,int len)
{
	unsigned int i;
	for(i=0;i<len;i++)
		printf("%02x:%08x\n",4*i,*((uint32_t *)(cfg+4*i)));
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

int main(void)
{	
	struct DMA_st *dma;
	int command;
	int i;
	uint32_t rdata;
	int data;
	
	dma = DMA_init();
	
	printf("fifo:%04d\n",oscillo_init());

	*dma->S2MM_DA = 0x1e000000;
	//debug_mem(dma->dmacfg,30);
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
	
	//debug_mem(dma->dmacfg,30);
	
	for(i=0;i < *dma->S2MM_LENGTH /4;i++){
		rdata=*((uint32_t *)(dma->dmabuf + 4*i));
		data = (rdata&0x00002000)? rdata|0xffffc000:rdata&0x00003fff;
		printf("%04d,0x%08x,%d\n",i,rdata,data);
	}
	DMA_close(dma);
	
	return 0;
}