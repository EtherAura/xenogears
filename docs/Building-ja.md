[English](Building.md) | 日本語

# ビルド

このページでは、クローンしたばかりの状態から、製品版のゲームと一致するビルドまでを案内します。お手元のディスクイメージを用意して、以下の手順に従ってください。

## 必要なもの

- bash、curl、git、make、gcc/cpp、Python 3、Ninja が入った Linux x86-64。root 権限は必要ありません。
- 北米版（SLUS-00664）か日本版（SLPS-01160）の、ご自身のディスクイメージ。このリポジトリには、ゲームのデータは何も含まれていません。

## 1. ツールチェーンのセットアップ

```bash
tools/setup.sh                       # toolchain -> tools/toolchain, Python deps -> .venv
source .venv/bin/activate            # fish: source .venv/bin/activate.fish
```

`tools/setup.sh` は、PlayStation のツールチェーンを `tools/toolchain` に、Python のパッケージを `.venv` に入れます。どちらもクローンしたディレクトリの中です。

## 2. ディスクイメージを置く

イメージは、`config/<version>/disc.yaml` に載っている Redump の名前で `bin/` の下に置きます。

## 3. 抽出、設定、ビルド

```bash
python tools/extract.py us     # the code files from your disc, checked against retail
python tools/configure.py us   # split every module with splat, write build.ninja
ninja                          # rebuild every module and check it against retail
python tools/check_bss.py      # statics at their addresses (the checksum can't see .bss)
python tools/progress.py       # matched code per module
```

日本版には `jp` を指定します。バージョンを省くと両方を処理します。北米版と日本版は同じ C からビルドし、日本版だけの違いは `#if VERSION_JP` の中にあります。

| 手順 | 内容 |
|---|---|
| `tools/extract.py` | ディスクからコードファイルを取り出し、製品版と照合します |
| `tools/configure.py` | splat ですべてのモジュールを分割し、`build.ninja` を書き出します |
| `ninja` | すべてのモジュールを再ビルドし、それぞれを製品版と照合します |
| `tools/check_bss.py` | 静的変数が正しいアドレスにあるかを確認します。これはチェックサムでは確認できません |
| `tools/progress.py` | モジュールごとに、一致したコードの量を表示します |

## ビルドで生成されるもの

次のものは生成物で、コミットすることはありません: `disc/`（抽出した製品版のファイル）、`asm/`、`linker/`、`build/`、`expected/`、`tools/toolchain/`、`.venv/`。

## 次へ

[マッチング](Matching-ja.md)では、関数に取り組むためのツールと、変更が守るべき規約を説明しています。
