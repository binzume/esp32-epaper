# e-Paper image loader for ESP32

指定したHTTP/HTTPSのURLからGIF/モノクロBMPの画像を読み込んで Waveshare や Good Display 製の電子ペーパに表示します。

サーバのHTTPレスポンスに refresh ヘッダを付けることで、次回の更新タイミングを指定できます。次回更新タイミングまでは deep sleep に入るので省電力です。

## Schematic

TODO: 回路図

回路図はまだありません。ESP32 と 電子ペーパの接続はコード中で使っているGPIOピンを見てください。

## ESP32

ESP32であれば動くと思いますが、以下のボードで動作確認済みです(ピン配置はプリプロセッサで切り替えてください)

- SeedStudio XIAO ESP32C3
- SeedStudio XIAO ESP32C5
- M5Stack ATOM
- NodeMCU ESP-32S

## e-Paper Module

[GxEPD](https://github.com/ZinggJM/GxEPD) がサポートする電子ペーパ。

以下のモジュール用に実装してるので、別の電子ペーパーを使う場合は、GxEPDのヘッダファイル等を調整してください。

- GDEW075Z08 800x480 White/Black/Red
- https://www.amazon.co.jp/dp/B08H8R6TQG


## Build with Arduino-CLI

```bash
cd esp32-epapeer
cd libraries
./setup_libs.sh
cd ..
arduino-cli compile --build-path ./build
arduino-cli upload --port COM9
```


## Build with Platform.IO

```bash
cd esp32-epapeer
pio run -t upload
```

# License

MIT License
