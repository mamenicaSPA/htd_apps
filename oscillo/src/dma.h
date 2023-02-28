#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>

struct S2MM_DMACR{
	unsigned RS 			:1;
	unsigned rsv0			:1;
	unsigned RESET			:1;
	unsigned KEYHOLE		:1;
	unsigned BD_ENABLE		:1;
	unsigned rsv1			:7;
	unsigned IOC_IRqEn		:1;
	unsigned Dly_IrqEn		:1;
	unsigned Err_IrqEn		:1;
	unsigned rsv2			:1;
	unsigned IRQThreshold 	:8;
	unsigned IRQDelay 		:8;	
};//offset 30h

union S2MM_DMACR_uni{
	unsigned int int32;
	struct S2MM_DMACR bit;
};

struct S2MM_DMASR{
	unsigned Halted		:1;//0
	unsigned Idle		:1;//1
	unsigned rsv0		:1;//2
	unsigned SGIncld	:1;//3
	unsigned DMAIntErr	:1;//4
	unsigned DMASlvErr	:1;//5
	unsigned DMADecErr	:1;//6
	unsigned rsv1		:1;//7
	unsigned SGIntErr	:1;//8
	unsigned SGSlvErr	:1;//9
	unsigned SGDecErr	:1;//10
	unsigned rsv2		:1;//11
	unsigned IOC_Irq	:1;//12
	unsigned Dly_Irq	:1;//13
	unsigned Err_Irq	:1;//14
	unsigned rsv3		:1;//15
	unsigned IRQThresholdSts	:8;//16-23
	unsigned IRQDelaySts		:8;//24-31
};//offset 34h

union S2MM_DMASR_uni{
	unsigned int int32;
	struct S2MM_DMASR bit;
};

struct DMA_st{
	union S2MM_DMACR_uni *S2MM_DMACR;
	union S2MM_DMASR_uni *S2MM_DMASR;
	unsigned int *S2MM_DA;
	unsigned int *S2MM_LENGTH;
	int dmafd;
	void *dmacfg;
	void *dmabuf;
};
/*
//struct S2MM_DA{
//};//offset 48h

//struct S2MM_LENGTH{
//};//offset 58h	MAX22bit
*/

struct DMA_st *DMA_init(){
	
	char *name = "/dev/mem";
	struct DMA_st *dma;
	dma = malloc(sizeof(struct DMA_st));
	
	if((dma->dmafd = open(name, O_RDWR)) < 0) {
    perror("open");
    return -1;
	}
	/* map the memory */
	dma->dmacfg = mmap(NULL, sysconf(_SC_PAGESIZE), 
             PROT_READ|PROT_WRITE, MAP_SHARED, dma->dmafd, 0x80400000);
			 
	dma->dmabuf = mmap(NULL, sysconf(_SC_PAGESIZE), 
             PROT_READ|PROT_WRITE, MAP_SHARED, dma->dmafd, 0x1e000000);
	
	dma->S2MM_DMACR  = dma->dmacfg + 0x30;
	dma->S2MM_DMASR  = dma->dmacfg + 0x34;
	dma->S2MM_DA    = (unsigned int*)(dma->dmacfg + 0x48);
	dma->S2MM_LENGTH = (unsigned int*)(dma->dmacfg + 0x58);
	
	dma->S2MM_DMACR->int32 = 0x00000000;
	
	*dma->S2MM_DA = 0x1e000000;
	
	dma->S2MM_DMACR->bit.RESET = 1;
	while(dma->S2MM_DMACR->bit.RESET==1);
	printf("DMA idle\n");

	return dma;
}

int DMA_close(struct DMA_st *dma){
	munmap(dma->dmacfg,sysconf(_SC_PAGESIZE));
	munmap(dma->dmabuf,sysconf(_SC_PAGESIZE));
	close(dma->dmafd);
	free(dma);
	return 0;
}