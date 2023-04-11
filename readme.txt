チュートリアル

　・ファイル"htd_apps"を"/opt/redpitaya/www/apps/htd_apps"になるように配置.
　・パーミッションがありません的な警告が出る場合は"rw"コマンドを入力.

　・"cd /opt/redpitaya/www/apps/htd_apps"でディレクトリ移動．
　・"make mother","make oscillo","make mcamcs"などを実行（更新したいものだけ）．

　・"./mother"で待機状態．
　・ウェブブラウザで"http://rp-f00b00.local/htd_apps/"へアクセス(f00b00の部分はmacアドレス下6桁)．
　・起動したいアプリを開き，IPaddressを設定してconect．

ディレクトリ/ファイル解説
htd_apps
├mcamcs				mcamcsアプリの保存される予定のディレクトリ（未完成）
│├src				
││├mcamcs.c			アプリケーションソースコード．
││└mcamcs.h			アプリケーションソースコード(あまり重要でないもの)．
│├config.txt		設定ファイル．
│├mcamcs			各種アプリケーション実行ファイル．上記のmakeコマンドで生成．
│├mcamcs.bit		FPGAビットストリームファイル(実行ファイルから呼び出される)．
│├mcamcs.html		GUI制御画面を表示するhtml．
│└mcamcs.js			ウェブブラウザとrepitayaで通信するためのjavascript(htmlから呼び出される)．
│
├oscillo			オシロスコープアプリの保存されたディレクトリ．
│├src				
││├oscillo.c		アプリケーションソースコード．
││└oscillo.h		アプリケーションソースコード(あまり重要でないもの)．
│├config.txt		設定ファイル．
│├oscillo			各種アプリケーション実行ファイル．上記のmakeコマンドで生成．
│├oscillo.bit		FPGAビットストリームファイル(実行ファイルから呼び出される)．
│├oscillo.html		GUI制御画面を表示するhtml．
│└oscillo.js		ウェブブラウザとrepitayaで通信するためのjavascript(htmlから呼び出される)．
│
├src				各種アプリから参照されるヘッダファイル．motherプログラムを格納したディレクトリ．
│├dma.h				FPGA-Cプログラム間で通信を行うためのライブラリ．oscilloアプリなどから参照されている．
│├websocket.h		ウェブブラウザとrepitayaで通信するためのwebsocketを利用するためのライブラリ．
│├mother.c			mother
│└mother_killer.c	mother_killer
│
├GNUmakefile		makeコマンドを実行するための設定ファイル．
├index.html			各種アプリにアクセスするための入口．
├mother				解説は後述．
├mother_killer		解説は後述．
└readme.txt			このファイル．

mother,mother_killerについて
　motherプログラムを実行すると，websocket接続待機状態になります．
　ブラウザから各種アプリを開き，conectボタンを押すと，
　motherプログラムが各種アプリの実行ファイルを起動します．
　motherプログラムは実行するとデーモンとなり，ctrl+C等で実行停止することができません．
　mother_killerは実行するとmotherプログラムへ停止シグナルを送り，実行を正常終了します．