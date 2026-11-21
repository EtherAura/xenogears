[English](Matching.md) | 日本語

# マッチング

このページでは、関数に取り組むためのツール、どの変更でも守る規約、リポジトリ内の配置をまとめています。先にビルドを済ませてください。手順は[ビルド](Building-ja.md)にあります。

## ツール

| ツール | 内容 |
|---|---|
| `tools/funcdiff.py us main` | C の関数を製品版と比べ、逆アセンブルの差分を表示 |
| `tools/objdiff_expected.py us`, `tools/objdiff_config.py us` | objdiff 用の期待オブジェクトと `objdiff.json` |
| `tools/m2c_func.py`, `tools/trymatch.py`, `tools/splice.py` | m2c による最初の下書き、マッチングの試行、関数をファイルへ組み込む作業 |
| `tools/fullasm.py` | すべての関数のアセンブリ（一致済みの関数も含む） |
| `tools/boundaries.py`, `tools/split_c.py`, `tools/add_c_file.py` | ファイル境界: その根拠、C ファイルの分割、新しいファイルの切り出し |
| `tools/datacheck.py` | C ファイルのデータセクションを製品版と比較 |
| `tools/share_symbols.py`, `tools/map_versions.py`, `tools/version_fallback.py` | 北米版と日本版の間で名前と C を共有 |

## 規約

- 関数を一致済みと数えるのは、objdiff が完全一致と判定したときだけです。
- `.word` や 16 進数で書いた関数本体は使いません。
- GTE の操作は `include/gte.h` のマクロだけで行います。スタックの切り替えと `$ra` の取得は `include/stack.h` だけで、デバッガーのポーリングは `include/pollhost.h` だけで、インラインアセンブリによるオーダリングテーブルのリンクは `include/ot.h` だけで行います。
- 構造体のフィールドには、`m1C_position` のようにオフセットを付けます。
- 北米版と日本版は同じ C からビルドします。日本版だけの違いは `#if VERSION_JP` の中に書きます。
- `#ifdef PLATFORM_PC` ブロックは [PC 移植版](https://github.com/EtherAura/xenogears-pc)のためのもので、PlayStation 向けのビルドでは何も生成しません。

## 構成

| パス | 内容 |
|---|---|
| `config/<ver>/` | バージョンごとのディスクのマニフェスト、splat の設定、シンボルの一覧 |
| `config/build.yaml` | パスごとのコンパイラーのプリセット |
| `src/`, `include/` | 逆コンパイルした C（バージョン間で共有） |
| `tools/` | セットアップ、抽出、ビルド、マッチングのスクリプト |

## 変更を送る

変更が通るべき検査と、プルリクエストの扱いについては、[CONTRIBUTING](../CONTRIBUTING.ja.md) にまとめてあります。
