const ST_IDLE_NCOM	= 0;	//default, disconect status
const ST_IDLE_COM	= 1;	//conected, idling status
const ST_DATATAKE	= 2;	//conected, datataking status

let connectingflg =false;
let ws;
let statchkInt;
let dtgetInt;
let xrange=0;
let xres=1;	//ch 512:8, 1024:4, 2048:2, 4096:1
let xmag=2;	//
let ylange=0;
let yres=1;
let ymag =1;

let stat = ST_IDLE_NCOM;


function time(){
	document.getElementById("Srate").textContent = "rate"+2**document.getElementById("CLKdiv1").value;
}

function statuscheck(){
	//1分毎にパケットを送って疎通チェック.応答ない場合はdisconect()
	ws.binaryType = "blob";
	ws.send('STACK');
	ws.onmessage = function(e){
		document.getElementById("console").value = 'messe:'+e.data;
	}
}

function dataget(){
	//1.5s毎にDTREQ要求を送信．
	ws.binaryType = "arraybuffer";
	ws.send('DTREQ');
	ws.onmessage = function(e){
		let hist = new Int32Array(e.data);
		console.log(hist);
		document.getElementById("console").value = hist[0];
		
		//バイナリデータから描画データ作成
		
		let point_array = new Array(1024);
		

		for(let i=0; i<1024;i++){
			for(let j=0;j<xres;j++)
				point_array[i] += hist[i];
				//point_array[i] += hist[xrange + parseInt(i*xmag*xres +j,10)];
		}
		console.log(point_array);

		let xoffset =0;
		let yoffset =768/2;
		let pathdata;
	
		pathdata = 'M '+xoffset;
		pathdata += ' '+yoffset +' L';
	
		for(let i=0;i<1024 ;i++){
			let ypoint =0;
			for(let j=0;j<xres;j++)
				ypoint += hist[xrange + parseInt(i*xmag*xres +j,10)];
			pathdata += ' '+(xoffset+(i+1)*1);
			pathdata += ' '+(yoffset -ypoint/ymag);
		}
		let hist_path=document.getElementById('path001')
		hist_path.setAttribute('d',pathdata);
	}
	
}

function connect(){
	if(stat == ST_IDLE_NCOM){	//connect
		document.getElementById("connect").value="disconnect";
		stat = ST_IDLE_COM;
		//websocket　http upgrade要求送信
		IPaddress = document.getElementById("IPaddress").value;
		port = 		document.getElementById("port").value;
		ws = new WebSocket("ws://" + IPaddress + ":"+ port+"/");
		ws.binaryType = "blob";
		ws.onmessage = function(e){
			let getdata = e.data;
			document.getElementById("console").value = getdata;
		}
		//status checkを開始（1min毎に疎通チェック）
		statchkInt = setInterval(statuscheck,60000);
	}
	else{						//disconnect
		document.getElementById("connect").value="connect";
		stat = ST_IDLE_NCOM;
		clearInterval(dtgetInt);
		clearInterval(statchkInt);
		//websocket完全接続断
		ws.binaryType = "blob";
		ws.send('ASTOP');
		ws.onmessage = function(e){
			document.getElementById("console").value = 'messe:'+e.data;
		}
		ws.close();
	}
}

function start(){
	//START要求を送信
	switch(stat){
		case ST_IDLE_COM:
			stat = ST_DATATAKE;
			ws.binaryType = "blob";
			ws.send('START');
			ws.onmessage = function(e){
				document.getElementById("console").value = 'messe:'+e.data;
			}
			//statuscheckを停止，
			clearInterval(statchkInt);
			let trg1h = document.getElementById("TRG1H").value;
			let trg1l = document.getElementById("TRG1L").value;
			let trg2h = document.getElementById("TRG2H").value;
			let trg2l = document.getElementById("TRG2L").value;
			let clkdiv1 = document.getElementById("CLKdiv1").value;
			let clkdiv2 = document.getElementById("CLKdiv2").value;
			let tau = document.getElementById("tau").value;
			let div = document.getElementById("div").value;
			let sel = document.getElementById("sel").value;
			let fname= document.getElementById("filename").value;
	
			let setting = trg1h+','+trg1l+','+trg2h+','+trg2l+','+clkdiv1+','+clkdiv2+','+tau+','+div+','+sel+','+fname;
			console.log(setting);
			ws.send(setting);
			rate = document.getElementById("rate").value*  1000;
			dtgetInt = setInterval(dataget,rate);
			break;
		case ST_DATATAKE:
			document.getElementById("console").value = "already data taking";
			break;
		case ST_IDLE_NCOM:
			document.getElementById("console").value = "not connected";
			break;
	}
}

function stop(){
	if(stat == ST_DATATAKE){
		stat = ST_IDLE_COM;
		ws.binaryType = "blob";
		ws.send('STOP_');
		ws.onmessage = function(e){
			document.getElementById("console").value = 'messe:'+e.data;
		}
		//datagetを停止
		clearInterval(dtgetInt);
		//status checkを再開（1min毎に疎通チェック）
		statchkInt = setInterval(statuscheck,50000);
	}
	else
		document.getElementById("console").value = "not started";
}

function save(){
	ws.binaryType = "blob";
	ws.send('DTGET');
	ws.onmessage = function(e){
		document.getElementById("console").value = 'messe:'+e.data;
	}
}

function clearrr(){
	ws.binaryType = "blob";
	ws.send('CLEAR');
	ws.onmessage = function(e){
		document.getElementById("console").value = 'messe:'+e.data;
	}
}
function xzoom(){
	xmag = xmag *2;
	if(xmag>4)
		xmag = 0.5;
	
	if(xrange + xmag*xres*1024 > 4096)
			xrange =0;
	
	document.getElementById("xscale0").textContent = xrange;
	document.getElementById("xscale1").textContent = xrange + xmag*xres*512;
	document.getElementById("xscale2").textContent = xrange + xmag*xres*1024;
}
function yzoom(){
	ymag =ymag *2;
	if(ymag >64)
		ymag =0.5;
	document.getElementById("yscale0").textContent = ymag*512;
	document.getElementById("yscale1").textContent = 0;
	document.getElementById("yscale2").textContent = ymag*512;
}
function xscroll(dir){
	if(dir>0){
		xrange =xrange +512;
		if(xrange + xmag*xres*1024 > 4096)
			xrange =0;
	}
	if(dir<0){
		xrange -= 512;
		if(xrange<0)
			xrange=512;
	}
	document.getElementById("xscale0").textContent = xrange;
	document.getElementById("xscale1").textContent = xrange + xmag*xres*512;
	document.getElementById("xscale2").textContent = xrange + xmag*xres*1024;
}
function yscroll(dir){
	if(dir>0){
		yrange =yrange +512;
		if(yrange + ymag*yres*1024 > 4096)
			yrange =0;
	}
	if(dir<0){
		yrange -= 512;
		if(yrange<0)
			yrange=512;
	}
	document.getElementById("yscale0").textContent = 0;
	document.getElementById("yscale1").textContent = yrange + ymag*yres*512;
	document.getElementById("yscale2").textContent = yrange + ymag*yres*1024;
}

/*
comand set
START
STOP_
ASTOP
DTREQ
DTGET
STACK
*/