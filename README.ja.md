# UnMetal Scanlines & Blur Toggle

UnMetalの走査線と画面ぼかしを個別にオン・オフできるようにし、レトロフィルターを消せるようにします。ウインドウ／フルスクリーンの設定は1行にまとめ、左右入力で選べるようにしました。これらの設定はゲーム中でも利用できます。また、元のゲームにある、より大きなウインドウ解像度へ切り替えると画面の一部が欠ける不具合も修正します。

[SteamでUnMetalを入手](https://store.steampowered.com/app/1203710/UnMetal/)

[English](README.md)

## 導入

1. UnMetalを終了します。
2. 配布ZIPを `unmetal.exe` のあるフォルダへ展開するか、このブランチのファイルをコピーします。
3. `install.cmd` をダブルクリックし、Steamを起動した状態でゲームを起動します。

Mod本体の `UnMetalMod.dll` は同梱済みで、ビルドは不要です。元のSDL2 DLLは導入時に自動で退避します。Steam DLLはゲーム本来のものを使います。以前のSteam DLL方式から更新した場合は、導入時に元のSteam DLLへ自動で戻します。

## 削除・復元

ゲームを終了し、`uninstall.cmd` をダブルクリックすると元のSDL2 DLLに戻ります。Modの設定ファイル `unmetal_scanlines.ini` は再利用できるよう残ります。設定も初期化したい場合は、このファイルも削除してください。

## 操作

タイトル画面またはゲーム中のメニューから映像設定を開き、走査線と画面ぼかしをそれぞれ切り替えます。表示モードは左右入力で候補を選び、決定で適用します。チェック欄は適用済みのモードを示します。フルスクリーンとウインドウの切り替えには、ゲームの再起動が必要な場合があります。

ウインドウを広げた際に画面が欠ける問題も修正しています。効果設定は `unmetal_scanlines.ini` に保存します。

## 対応環境

Steam版UnMetal 1.0.13（build 12471095）に対応します。導入時にゲームのバージョンを確認します。

## ソースからビルド

ソースとビルドスクリプトは `src/` にあります。WindowsでPython、`pefile`、Visual Studio C++ x86ビルドツールを用意して実行します。

```powershell
python -m pip install pefile
python src/build.py --game-dir "C:\Games\UnMetal"
```

対応する未改変のゲームを指定してください。出力は `build/SDL2.dll` です。`--vcvars` で `vcvarsall.bat` を指定するとコンパイラを選べます。

## 開発について

このModはOpenAI Codexを使用して作成しました。

## ライセンス

このMod独自のコードとドキュメントは [MITライセンス](LICENSE) で公開しています。このライセンスはUnMetalやその他の第三者の著作物に対する権利を許諾するものではありません。ゲーム本体のファイルとSDL2は同梱せず、インストール済みゲームのSDL2 DLLを利用します。
