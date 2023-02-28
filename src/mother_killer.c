#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define PORT_DEBUG	 10000

int main()
{
	struct sockaddr_in server;
	int sock;
	char buf[128];		//バッファ(1kB)
	int n;

	/*ソケットの作成*/
	sock = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;	//よくわからん
	server.sin_port = htons(PORT_DEBUG);	//接続先ポート番号
	server.sin_addr.s_addr = inet_addr("127.0.0.1");	//接続先IPアドレス

 
	if(0 != connect(sock, (struct sockaddr *)&server, sizeof(server))){
		printf("error\n");
		return -1;
	}
	
	printf("conected\n");

	memset(buf, 0, sizeof(buf));		//バッフア初期化
	n = read(sock, buf, sizeof(buf));	//受信
	
	printf("read:%d\n***\n %s***\n", n, buf);	//表示

	/*ソケットの終了*/
	close(sock);

	return 0;
}