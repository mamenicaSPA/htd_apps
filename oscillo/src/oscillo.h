#define MAX_CONFIG 32

//OSCFLGS : スレッド間通信フラグ．フラグが立つまで待機，フラグが立ったらループ終了などの指令情報共有
#define OSCFLGS_INIT	0x00000000	//0にセット
#define OSCFLGS_FPGARST 0x00000001	//ws->FPGA FPGAリセット要求(TRG設定，FIFOクリア等)
#define OSCFLGS_STOP	0x00000002	//ws->FPGA ヒストグラム更新ループ停止
#define OSCFLGS_ASTOP	0x00000004	//ws->FPGA FPGA全体ループ停止
#define OSCFLGS_WRITTEN 0x00000008	//FPGA->ws DMA正常読み取り完了
#define OSCFLGS_ERRDMA	0x00000010	//FPGA->ws DMAエラー終了フラグ
#define OSCFLGS_DTREQ	0x00000020 

union hist_data{
	unsigned int int32[4096];
	unsigned char byte[4096*4];
};

struct config{
	char *bitstream;
	int sock;
	int Htrg1;
	int Ltrg1;
	int Htrg2;
	int Ltrg2;
	int clkdiv1;
	int clkdiv2;
	int tau;
	int div1;
	int div2;
	int sel;
};

int config_recv(struct ws_sock_t *ws_sock,struct config *conf)
{
	FILE *fp;
	char buf[128];
	char item[128];
	char param[128];
	
	int i=0;
	
	while(i++ < MAX_CONFIG){
		ws_read(ws_sock, buf, sizeof(buf), NULL);
		sscanf(buf,"%[^=]=%s",item,param);
		
		printf("item:%s,param:%s\n",item,param);

		if(strcmp(item,"bitstream")		==0){
			free(conf->bitstream);
			conf->bitstream = strdup(param);
		}
		
		if(strcmp(item,"Htrg1")			==0)
			conf->Htrg1 = atoi(param);
		if(strcmp(item,"Ltrg1")			==0)
			conf->Ltrg1 = atoi(param);
		if(strcmp(item,"Htrg2")			==0)
			conf->Htrg2 = atoi(param);
		if(strcmp(item,"Ltrg2")			==0)
			conf->Ltrg2 = atoi(param);
		
		if(strcmp(item,"clkdiv1")		==0)
			conf->clkdiv1 = atoi(param);
		if(strcmp(item,"clkdiv2")		==0)
			conf->clkdiv2 = atoi(param);
		if(strcmp(item,"trptau")		==0)
			conf->tau = atoi(param);
		if(strcmp(item,"trpdiv1")		==0)
			conf->div1 = atoi(param);
		if(strcmp(item,"trpdiv2")		==0)
			conf->div2 = atoi(param);

		if(strcmp(item,"#end")		==0)
			break;
	}
	
	return 0;
}
	
void rdata_decode(void *buf, uint32_t rdata, int i)
{
	union hist_data *data = (union hist_data*)buf;
	data->int32[i] = (rdata&0x00002000)? rdata|0xffffc000 : rdata&0x00003fff;
	printf("%d,%x,%d\n",i,rdata,data->int32[i]);
}