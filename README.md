# RaspberryPI_JJYRepeter
JJY Repeter using Raspberry PI

Raspberry PI で、標準電波JJYを発信する簡単なプログラムです。

ハードの回路は、日経の記事を参考にしてください。
http://itpro.nikkeibp.co.jp/atcl/column/14/093000080/

PWMを使用して40kHzの発信をしていますので、信号ピンは、
GPIO:18 としています。(Python版)

C言語版は、発信 GPIO:12、ntp同期状態確認 GPIO:13、
プログラム動作確認でGPIO:6 を使っています。
