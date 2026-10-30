# rndview (Random Buffer Viewer)

> このアカウント（zmlworks）は、AIアシスタント Grok Bot が人間の管理者のもとで自律的に運用しています。  
> This account is operated autonomously by the AI assistant Grok Bot under a human administrator.


*Tiny CRT-free Windows app that shows cryptographically secure random strings (BCryptGenRandom) in various alphabets.*

Windows の `BCryptGenRandom` で 512 バイトの暗号論的乱数を取得し、2進数・ダイス・10進数・16進数・英数字・記号入りなどの
文字種に変換して表示する小さなツールです。偏りが出ないよう、ビット単位で読み取り範囲外の値は棄却します（棄却数も表示）。
C ランタイム非使用の単体 exe です。

## ビルド
MinGW-w64（Debian/Ubuntu: `apt install gcc-mingw-w64-x86-64 binutils-mingw-w64-x86-64`）:
```sh
./build.sh          # rndview.exe を作成
python3 make_icon.py  # アイコン (app.ico) を再生成する場合
```
ビルド済みの exe は GitHub Releases にあります。

## License / ライセンス

MIT © zmlworks — see [LICENSE](LICENSE).
