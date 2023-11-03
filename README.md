# arwtool

## これは何？

ARW ファイル（ソニー製デジタルスチルカメラが出力する RAW 画像形式）に付加された現像設定のバージョンスタック情報をコピー・削除するツールです。
コマンドラインから使用でき、巨大なファイルでも高速に処理できるため、大量のファイルの現像設定を一括で書き換えたい場合に便利です。

## 使用方法

> [!WARNING]
> このツールは ARW ファイルを直接書き換えます。大切な撮影データの喪失を防ぐため、必ずあらかじめバックアップを取るようにしてください。不具合や誤操作によるいかなる損害も作者は責任を負いません。

```
arwtool store-versionstack RAW-FILE VERSIONSTACK-FILE
```
ARW ファイル `RAW-FILE` のバージョンスタック情報を抽出して `VERSIONSTACK-FILE` へ書き出します。

```
arwtool restore-versionstack RAW-FILE VERSIONSTACK-FILE
```
書き出されたバージョンスタック情報 `VERSIONSTACK-FILE` を ARW ファイル `RAW-FILE` に埋め込みます。すでに ARW ファイルにバージョンスタックが存在する場合は削除してから埋め込みます。

```
arwtool remove-versionstack RAW-FILE
```
ARW ファイル `RAW-FILE` から書き出したバージョンスタック情報を削除します。

```
arwtool trace RAW-FILE
```
デバッグ用です。ファイル構造が表示されますが、公式資料があるわけではないのでそれぞれのフィールドの値が何を意味しているのか、本当に正しくパースできているのかは不明です。

## ビルド方法

C++17 に対応した環境で arwtool.cpp を単体コンパイルして実行ファイルを生成してください。標準ライブラリ以外の依存はありません。

GCC の場合：
```
g++ arwtool.cpp -o arwtool.exe
```

Clang の場合：
```
clang++ arwtool.cpp -o arwtool.exe
```

Visual Studio の場合：
```
cl /std:c++17 /EHsc arwtool.cpp
```

Visual Studio 2022 の IDE でビルドしたい場合は arwtool.sln を開いてビルドしてください。

