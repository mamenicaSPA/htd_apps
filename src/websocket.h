#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define OPCD_CONTNE	0x00
#define OPCD_TEXT	0x01
#define OPCD_BIN	0x02
#define OPCD_CLOSE	0x08
#define OPCD_PING	0x09
#define OPCD_PONG	0x0A

#define READ_ERR_CLOSE -OPCD_CLOSE
#define READ_ERR_PING -OPCD_PING
#define READ_ERR_PONG -OPCD_PONG
#define READ_ERR_OPCDMISS -256
#define READ_ERR_TIMEOUT  -257

#define TIMEOUT_SEC 59

#define SHA1CircularShift(bits,word) \
            (((word) << (bits)) | ((word) >> (32-(bits))))

union long_char4{
	char		byte[4];
	uint32_t	ln;
};

struct frame_header_s{
	char opcode 	: 4;
	char resrved 	: 3;
	char fin 		: 1;
	
	char payload_len: 7;
	char mask		: 1;
	char mskey_exlen[12];
};

union payload_frm{
	struct frame_header_s st;
	char *byte;
};

uint32_t SHA1f00(uint32_t B,uint32_t C,uint32_t D){
	return (B & C) | ((~B) & D);
}
uint32_t SHA1f20(uint32_t B,uint32_t C,uint32_t D){
	return B ^ C ^ D;
}
uint32_t SHA1f40(uint32_t B,uint32_t C,uint32_t D){
	return (B & C) | (B & D) | (C & D);
}
uint32_t SHA1f60(uint32_t B,uint32_t C,uint32_t D){
	return B ^ C ^ D;
}

int SHA1padding(char input[]){
	int i;
	int pad_bytes;
	int length;
	union long_char4 bit_length;
	
	length = strlen(input);
	bit_length.ln = length*8;
	//printf("length:%d\n",length);
	//追加するパディングビット数を決定．lengthが64n+56以上：拡張．未満：length+pad_bytesが56になるように設定
	if(length%64 >= 56)
		pad_bytes = 56 + (64-length%64);
	else
		pad_bytes = 56 - length%64;
	//printf("pad_bytes:%d\n",pad_bytes);
	//add 0x80
	input[length] = 0x80;
	//fill 0
	for(i=0;i<pad_bytes-1;i++){
		input[length+i+1] = 0x00;
	}
	//printf("fill0\n");
	
	//add length
	for(i=0;i<4;i++){
		input[length+pad_bytes+i] = 0x00;
	}
	for(i=0;i<4;i++){
		input[length+pad_bytes + 4 + i]	= bit_length.byte[3-i];
	}
	return (length + pad_bytes)/64 +1;
}

int SHA1digest(char *data,uint32_t *H){
	int i;
	int block;
	uint32_t A, B, C, D, E;
	
	uint32_t W[80];
	uint32_t TEMP;
	const uint32_t K[4] = {
    0x5A827999,    /*  0～19回目の演算で使用 */
    0x6ED9EBA1,    /* 20～39回目の演算で使用 */
    0x8F1BBCDC,    /* 40～59回目の演算で使用 */
    0xCA62C1D6     /* 60～79回目の演算で使用 */
	};
	//printf("stepA\n");
	//stepA
	for (i = 0; i < 16; i++) {
		W[i]  = data[i*4  ] << 24;
		W[i] |= data[i*4+1] << 16;
		W[i] |= data[i*4+2] <<  8;
		W[i] |= data[i*4+3];
	}
	//printf("stepB\n");
	//stepB
	for (i = 16; i < 80; i++) {
		W[i] = SHA1CircularShift(1, W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16]);
	}
	//printf("stepC\n");
	//stepC
	A = H[0];
	B = H[1];
	C = H[2];
	D = H[3];
	E = H[4];
	//printf("stepD\n");
	//stepD
		/* 0～19回目の演算 */
	for (i = 0; i < 20; i++) {
		TEMP = SHA1CircularShift(5, A) + SHA1f00(B, C, D) + E + W[i] + K[0];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = TEMP;
	}
		/* 20～39回目の演算 */
	for (i = 20; i < 40; i++) {
		TEMP = SHA1CircularShift(5, A) + SHA1f20(B, C, D) + E + W[i] + K[1];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = TEMP;
	}
		/* 40～59回目の演算 */
	for (i = 40; i < 60; i++) {
		TEMP = SHA1CircularShift(5, A) + SHA1f40(B, C, D) + E + W[i] + K[2];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = TEMP;
	}
		/* 60～79回目の演算 */
	for (i = 60; i < 80; i++) {
		TEMP = SHA1CircularShift(5, A) + SHA1f60(B, C, D) + E + W[i] + K[3];
		E = D;
		D = C;
		C = SHA1CircularShift(30, B);
		B = A;
		A = TEMP;
	}
	//printf("stepE\n");
	H[0]+=A;
	H[1]+=B;
	H[2]+=C;
	H[3]+=D;
	H[4]+=E;
	
	return 0;
}

int SHA1convert(char *data,char out[]){
	int i;
	int block;
	uint32_t H[5];
	block = SHA1padding(data);
	//printf("block:%d\n",block);
	H[0] = 0x67452301;
	H[1] = 0xEFCDAB89;
	H[2] = 0x98BADCFE;
	H[3] = 0x10325476;
	H[4] = 0xC3D2E1F0;
	for(i=0;i<block;i++)
		SHA1digest((char *)(data + i*64),H);
	for(i=0;i<5;i++)
		//printf("%08x ",H[i]);
	//printf("\n");
	
	for(i=0;i<20;i++){
		out[i] = H[i/4] >> (24-((i%4)*8));
	}
	
	return 0;
}

char BASE64BittoChar(char sixbit){
	if(0x00<=sixbit && sixbit<0x1A)
		return 'A'+sixbit;
	if(0x1A<=sixbit && sixbit<0x34)
		return 'a' + sixbit-0x1A;
	if(0x34<=sixbit && sixbit<0x3E)
		return '0' + sixbit-0x34;
	if(0x3E == sixbit)
		return '+';
	if(0x3F == sixbit)
		return '/';
}


int BASE64convert(char input[],int length,char outtext[]){
	int i;
	char nowbyte;
	char nextbyte;
	char sixbit = 0x00;
	
	for(i=0; i<(length*4)/3; i++){
		switch(i%4){
			case 0:sixbit = (input[(i*3)/4]>>2)&0x3f;		break;
			case 1:sixbit = (input[(i*3)/4]<<4)&0x30 | (input[(i*3)/4+1]>>4)&0x0f; break;
			case 2:sixbit = (input[(i*3)/4]<<2)&0x3c | (input[(i*3)/4+1]>>6)&0x03; break;
			case 3:sixbit = (input[(i*3)/4])   &0x3f;	break;
		}
		//printf("%02x,",sixbit);
		outtext[i] = BASE64BittoChar(sixbit);	
	}
	switch(i%4){
		case 0:sixbit = (input[(i*3)/4]>>2)&0x3f; break;
		case 1:sixbit = (input[(i*3)/4]<<4)&0x30; break;
		case 2:sixbit = (input[(i*3)/4]<<2)&0x3c; break;
		case 3:sixbit = (input[(i*3)/4])   &0x3f; break;
	}
	//printf("%02x,",sixbit);
	outtext[i] = BASE64BittoChar(sixbit);
	i+=1;
	while(i%4){
		outtext[i++] = '=';
	}
	outtext[i] = '\0';
	return i;
}

//*******************//
//websocket functions//
//*******************//

//int sec_websocket_key(char key[],char accept_key[])
//make accept_key from sec_websocket_key.
//└─>sec_websocket_key will be received from http heada.
//*key        : sec-websocket-key from client
//*accept_key : generated accept key
int sec_websocket_key(char key[],char accept_key[]){
	
	char buf[128];
	char temp[20];
	sprintf(buf,"%s%s",key,"258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	SHA1convert(buf,temp);
	BASE64convert(temp,20,accept_key);
	return 0;
}

//int mask_conv(char *encode,char *mask,unsigned int length)
//make masked payload data. and do data masking.
//└─>websocket payload data is always masked. for securety. non masked data will be blocked by client.
//*encode : masked data pointer. decoded data will be overwritten.
//*mask   : mask key (4byte).
//length  : data length of masked data.
//return  : decoded length.
int mask_conv(char *encode,char *mask,unsigned int length){
	unsigned int i;
	for(i=0u;i<length;i++)
		encode[i]=encode[i]^mask[i%4];
	return i;
}

//int wsfrmheada(char *encode,char *msk,unsigned int length)
//make wabsocket frame header. 
//├─>if you will send payload, frames and payloads must be sent back-to-back. ex:write(sock,frmhead,headlength);write(sock,payload,datalength);
//└─>websocket client -> server payload is need mask. server -> client payload is not need mask.
//*encode : maked heada pointer.
//*msk    : mask key (4byte). if msk is NULL, this function will not set mask header.
//opcd	  : opcode. 
//length  : payload data length.
//return  : heada length.
int wsfrmheada(char *encode,char *msk,char opcd,unsigned int length){
	struct frame_header_s *head = (struct frame_header_s *)encode;
	int i;
	
	head->fin =1;
	head->resrved=0;
	head->opcode = opcd;
	
	if(msk ==NULL)
		head->mask=0;
	else 
		head->mask=1;
	
	if(length == 0 )
		length = strlen(encode);
	
	if(length < 126){
		head->payload_len = length;	//set length
		if(msk == NULL)			// non mask
			return 2;
		for(i=0;i<4;i++)		//set mask
			head->mskey_exlen[i] = msk[i];
		return 6;
	}else if(length < 65535){
		head->payload_len = 126;
		for(i=0;i<2;i++)		//set length
			head->mskey_exlen[i] = (length>>(8*(1-i))) & 0x00ff;
		if(msk == NULL)			//non mask;
			return 4;
		for(;i<2+4;i++)			//set mask
			head->mskey_exlen[i] = msk[i];
		return 8;
	}else{
		head->payload_len = 127;
		for(i=0;i<8;i++)		//set length
			head->mskey_exlen[i] = (length>>(8*(7-i))) & 0x000000ff;
		if(msk == NULL)			//non mask
			return 6;
		for(;i<8+4;i++)			//set mask
			head->mskey_exlen[i] = msk[i];
		return 10;
	}
}

//int wsdecode(char *frame,int *headlength)
//frame data convert to decoded payload data.
//*frame      : received frame data pointer. decoded data pointer will be overwritten.
//*headlength : frame length returen varue
//*messelength: message length retrun varue
//return      : opcode.
int wsdecode(char *frame,int *headlength,int *messelength, char msk[]){

	struct frame_header_s *head = (struct frame_header_s *) frame;
	unsigned int datalength = 0;
	int i;
	
	memset(msk,0,4);
	
	if(head->payload_len <126){
		datalength = head->payload_len;
		if(head->mask)
			for(i=0;i<4;i++)
				msk[i] = head->mskey_exlen[i];
		*headlength = 6;
	}else if(head->payload_len <= 65535){
		for(i=0;i<2;i++)
			datalength += head->mskey_exlen[i]<<(8*(1-i));
		if(head->mask)
			for(i=0;i<4;i++)
				msk[i] = head->mskey_exlen[i+2];
		*headlength = 10;
	}else{
		for(i=0;i<8;i++)
			datalength += head->mskey_exlen[i]<<(8*(7-i));
		if(head->mask)
			for(i=0;i<4;i++)
				msk[i] = head->mskey_exlen[i+8];
		*headlength = 14;
	}
	
	//mask_conv(frame + *headlength,msk,datalength);

	*messelength = datalength;
	return head->opcode;
}

#define WS_BUF_MAX 1024

struct ws_sock_t{
	int wsock;
	int readlen;
	int datalen;
	int headlen;
	char *bufpt;
	char buffer[WS_BUF_MAX];
};

struct http_header{
	char item[128];
	char param[128];
};

int ws_write(struct ws_sock_t *ws_sock, char *buf,int size, int opcd){
	int sock = ws_sock->wsock;
	char head[16];
	int len=0;
	int head_len;
	
	head_len = wsfrmheada(head,NULL,opcd,size);
	len=write(sock,head,head_len);
	len=write(sock,buf,size);
	return len;
}

int ws_read(struct ws_sock_t *st, char *buf, int size,int *opcd){

	int n;
	int ret_sel;
	fd_set fds;
	struct timeval tv;
	char msk[4];
	int templen;
	int sock = st->wsock;
	int tempopcd;
	
	if(opcd ==NULL)
		opcd = &tempopcd;
	
	FD_ZERO(&fds);		//select statement set up
	FD_SET(sock,&fds);	//select statement set up
	
	tv.tv_sec = TIMEOUT_SEC;	//select statement set up
	tv.tv_usec=0;				//select statement set up
	
	if(st->readlen > st->datalen+st->headlen){	//前回の呼び出し時に複数回分のデータを読んだ場合（バッファにデータが残っている）
		st->bufpt += st->datalen+st->headlen;
		st->readlen -= st->datalen+st->headlen;
		*opcd = wsdecode(st->bufpt, &st->headlen, &st->datalen, msk);
	}
	else{										//バッファにデータが残っていない場合
		st->bufpt = st->buffer;
		st->readlen = 0;
		st->headlen = 1;
		st->datalen = 1;
	}
	
	while(st->readlen < st->headlen+st->datalen){
		ret_sel = select(sock+1,&fds,0,0,&tv);
		if(ret_sel <=0){
			printf("timeout\n");
			return READ_ERR_TIMEOUT;
		}
		st->readlen += read(sock,st->bufpt,WS_BUF_MAX- (st->buffer-st->bufpt));
		*opcd = wsdecode(st->bufpt, &st->headlen, &st->datalen, msk);
	}

	switch(*opcd){
		case OPCD_CONTNE:
			memcpy(buf+templen,st->bufpt+st->headlen,st->datalen);
			mask_conv(buf+templen, msk, st->datalen);
			return templen + st->datalen;
			break;
		case OPCD_TEXT :
			memcpy(buf, st->bufpt+st->headlen, st->datalen);
			mask_conv(buf, msk, st->datalen);
			buf[st->datalen] = '\0';
			return st->datalen;
			break;
		case OPCD_BIN  :
			memcpy(buf, st->bufpt+st->headlen, st->datalen);
			mask_conv(buf, msk, st->datalen);
			return st->datalen;
			break;
		case OPCD_CLOSE:
			memset(buf,0,st->datalen);
			ws_write(st, buf, st->datalen, OPCD_CLOSE);
			return READ_ERR_CLOSE;
			break;
		case OPCD_PING :
			memcpy(buf, st->bufpt+st->headlen, st->datalen);
			mask_conv(buf, msk, st->datalen);
			ws_write(st, buf, st->datalen, OPCD_PONG);
			return READ_ERR_PING;
			break;
		case OPCD_PONG :
			memcpy(buf, st->bufpt+st->headlen, st->datalen);
			mask_conv(buf, msk, st->datalen);
			buf[st->datalen] = '\0';
			return READ_ERR_PONG;
			break;
		default:
			break;
	}
	return READ_ERR_OPCDMISS;
}

struct ws_sock_t *ws_init(int sock){

	char buf[1024];
	char response_key[128];
	char *ptr;
	int n;
	struct http_header head;
	struct ws_sock_t *st;
	
	st = (struct ws_sock_t*)malloc(sizeof(struct ws_sock_t));
	st->readlen = 0;
	st->headlen = 0;
	st->datalen = 0;
	st->bufpt=st->buffer;
	st->wsock = sock;
	
	memset(buf, 0, sizeof(buf));		//バッフア初期化
	n = read(sock, buf, sizeof(buf));	//リクエストヘッダ受信
	//printf("read:%d\n*****\n %s***\n", n, buf);	//表示
	ptr = strtok(buf,"\n");
	while((ptr = strtok(ptr+strlen(ptr)+1,"\n")) != NULL){
		sscanf(ptr,"%[^:]: %s",head.item,head.param);
		//printf("item:%s  param:%s\n",head.item,head.param);
		if(strcmp(head.item,"Sec-WebSocket-Key")==0){
			sec_websocket_key(head.param,response_key);
			break;
		}
	}
	
	printf("request key:%s\n",head.param);
	printf("response key:%s\n",response_key);
	//*******HTTP header send*******//
	memset(buf, 0, sizeof(buf));		//バッフア初期化
	write(sock,"HTTP/1.1 101 Switching Protocols\n",33);	//送信
	write(sock,"Upgrade: websocket\n",19);	//送信
	write(sock,"Connection: Upgrade\n",20);	//送信
	write(sock,"Sec-WebSocket-Accept: ",22);	//送信
	write(sock,response_key,strlen(response_key));
	write(sock,"\n\n",2);
	
	return st;
}

int ws_close(struct ws_sock_t *ws_sock){
	close(ws_sock->wsock);
	free(ws_sock);
	ws_sock = NULL;
	return 0;
}