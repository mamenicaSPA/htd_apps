#define MAX_CONFIG 32

#define OSCFLGS_INIT	0x00000000
#define OSCFLGS_FPGARST 0x00000001
#define OSCFLGS_STOP	0x00000002
#define OSCFLGS_ASTOP	0x00000004
#define OSCFLGS_WRITTEN 0x00000008
#define OSCFLGS_ERRDMA	0x00000010

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
}

struct confug *config_recv(struct ws_sock_t *ws_sock)
{
	FILE *fp;
	char buf[128];
	char item[128];
	char param[128];
	
	int i=0;
	
	if(conf == NULL)
		conf = malloc(sizeof(struct config));
	
	while(i++ < MAX_CONFIG){
		ws_read(ws_sock, buf, sizeof(buf), NULL);
		sscanf(buf,"%[^=]=%s",item,param);

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