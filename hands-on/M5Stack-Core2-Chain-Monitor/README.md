# M5Stack Core2 — CHAIN Encoder + ToF Monitor

> CDLEセミナー向け M5Stack × センサー連携デモ

このプロジェクトは、**CDLEのセミナーで紹介・体験するためのIoTデモプロジェクト**です。ワークショップ資料リポジトリの`hands-on/M5Stack-Core2-Chain-Monitor`に収録しています。

M5Stack Core2 v1.3にCHAIN EncoderとCHAIN ToFを接続し、回転と距離という2種類のセンサーデータを取得します。取得した値は本体画面とシリアルモニターへリアルタイムに表示し、回転が一定時間止まると音で知らせます。

「センサー入力 → 状態判定 → 画面・音によるフィードバック」という、エッジデバイス開発の基本的な流れを小さな構成で体験できます。

## セミナーで体験できること

- M5Stack Core2とCHAINデバイスの接続
- PlatformIOを使ったビルドとファームウェア書き込み
- EncoderとToFからのリアルタイムデータ取得
- センサーデータの画面表示とシリアル出力
- 経過時間を使った状態判定
- スピーカーによるアラート通知
- 定数を変更してデバイスの挙動をカスタマイズ

## デモの動作

1. 起動時にCHAIN EncoderとCHAIN ToFを自動検出します。
2. Encoderを回すと、回転カウントがCore2の画面に表示されます。
3. ToFが測定した距離をmm単位で表示します。
4. Encoderが5秒間回転しないと、2kHzの警告音を1秒間鳴らします。
5. 回転を再開するとアラート状態を解除します。
6. Encoderのノブを押すと、回転カウントを0にリセットします。

## デモの見どころ

| 入力 | 処理 | 出力 |
|---|---|---|
| Encoderの回転 | カウント変化と停止時間を判定 | 数値・色・警告音 |
| Encoderのボタン | 押下イベントを判定 | カウントを0にリセット |
| ToFの距離 | 200ms間隔で測定 | 距離をmm単位で表示 |
| CHAIN接続状態 | 起動時にデバイスを探索 | 接続状況・エラー画面 |

単に値を表示するだけでなく、複数のセンサー情報と時間条件を組み合わせ、ユーザーへ分かりやすくフィードバックする構成になっています。

## 必要なもの

### ハードウェア

- [M5Stack Core2 v1.3](https://www.switch-science.com/collections/m5stack/products/11050)
- [M5Stack CHAIN Encoder](https://www.switch-science.com/products/10919?_pos=12&_sid=3317d8f22&_ss=r)
- [M5Stack CHAIN ToF](https://www.switch-science.com/products/11001?_pos=8&_sid=f649cb7d9&_ss=r)
- CHAINデバイス間およびCore2との接続ケーブル
- [M5Stack用GROVE互換ケーブル 5 cm](https://www.switch-science.com/products/8664?pr_prod_strat=jac&pr_rec_id=c9a1cb7ee&pr_rec_pid=7721035989190&pr_ref_pid=8797683417286&pr_seq=uniform)
- データ通信対応USBケーブル

### ソフトウェア

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE](https://platformio.org/install/ide?install=vscode)
- Git（リポジトリをクローンする場合）

依存ライブラリは`platformio.ini`からPlatformIOが自動取得します。

- `M5Unified`
- `M5Chain` 1.0.8

## 接続

Core2のHY2.0-4P端子とCHAINデバイスを接続します。端子は通常PORT.A（I2C）として案内されていますが、このプロジェクトではGPIO32/33をUARTへ割り当てます。同じ端子へI2C機器を同時接続しないでください。

| Core2 | GPIO | 信号方向 | CHAIN |
|---|---:|---|---|
| HY2.0 白 | GPIO33 | Core2 RX ← | TX |
| HY2.0 黄 | GPIO32 | Core2 TX → | RX |

CHAIN EncoderとCHAIN ToFはデイジーチェーンで接続します。デバイスを認識しない場合は、CHAINの接続方向とケーブルを確認してからCore2を再起動してください。

## セットアップ

リポジトリをクローンし、プロジェクトをVisual Studio Codeで開きます。

```bash
git clone https://github.com/Tori41257/CDLE-Generative-AI-IoT-Workshop.git
cd CDLE-Generative-AI-IoT-Workshop/hands-on/M5Stack-Core2-Chain-Monitor
code .
```

PlatformIO CLIを使う場合は、次のコマンドでビルド・書き込み・シリアル確認を行えます。

```bash
pio run
pio run --target upload
pio device monitor
```

シリアルモニターの通信速度は`115200 bps`です。

Visual Studio Codeでは、PlatformIOの **Build**、**Upload**、**Serial Monitor** を順に実行してください。

## セミナーで試せるカスタマイズ

設定は`src/main.cpp`冒頭の定数で変更できます。

| 設定 | 初期値 | 試せること |
|---|---:|---|
| `STOP_TIMEOUT_MS` | 5000 ms | 停止と判定する時間を短く・長くする |
| `TOF_INTERVAL_MS` | 200 ms | 距離表示の更新頻度を変える |
| `ALERT_FREQUENCY_HZ` | 2000 Hz | 警告音の高さを変える |
| `ALERT_DURATION_MS` | 1000 ms | 警告音の長さを変える |
| `SPEAKER_VOLUME` | 50 / 255 | 音量を調整する |
| `CHAIN_BAUDRATE` | 115200 | CHAINとの通信速度を確認する |
| `MAX_CHAIN_DEVICES` | 8 | 探索するCHAINデバイス数の上限を変える |

例えば、ToFの距離が一定値より近い場合に画面色や音を変えると、「接近検知」デモへ発展させられます。Encoderの停止判定は、設備の回転監視や簡易的な異常検知を考える題材にもなります。

## 学習ポイント

- 組み込み機器での`setup()`と`loop()`の役割
- UARTによるデバイス間通信
- 複数センサーの初期化とデバイス探索
- ポーリング間隔を`millis()`で管理する方法
- 状態変化があったときだけ表示を更新する考え方
- センサー値を「見える情報」と「気づける通知」に変換する方法
- 実機デモから異常検知・設備監視へ発展させる考え方

## プロジェクト構成

```text
.
├── include/        # プロジェクト用ヘッダー
├── lib/            # プロジェクト固有ライブラリ
├── src/
│   └── main.cpp    # デモアプリケーション本体
├── test/           # PlatformIOテスト
└── platformio.ini  # ボード、依存関係、書き込み設定
```

## トラブルシューティング

### `No Chain device found`と表示される

- PORT.Aの配線とCHAINの接続方向を確認してください。
- 各コネクタが奥まで差し込まれているか確認してください。
- 接続後にCore2を再起動してください。

### `Encoder not found`または`Chain ToF not found`と表示される

- EncoderとToFの両方がデイジーチェーンに含まれているか確認してください。
- CHAINデバイスの接続順と方向を確認してください。

### 書き込みに失敗する

- USBケーブルがデータ通信に対応しているか確認してください。
- PlatformIOで正しいシリアルポートが選択されているか確認してください。
- 必要に応じて`platformio.ini`の`upload_speed`を下げてください。

### シリアルモニターに文字が表示されない

- 通信速度が`115200 bps`になっているか確認してください。
- 書き込みに使用したポートと同じポートを選択してください。

## 今後の発展例

- ToFの距離しきい値による接近アラート
- センサーデータのグラフ化
- Wi-Fi経由でのデータ送信
- MQTTやクラウドサービスとの連携
- 複数条件を組み合わせた異常判定
- 収集データを用いた機械学習・異常検知

## ライセンス

このプロジェクトは[MIT License](LICENSE)で公開しています。
