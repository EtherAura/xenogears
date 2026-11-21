[English](Tooling.md) | 日本語

# ツールと謝辞

| ツール | 内容 |
|---|---|
| `tools/funcdiff.py us main` | C の関数を製品版と比べ、逆アセンブルの差分を表示 |
| `tools/objdiff_expected.py us`, `tools/objdiff_config.py us` | objdiff 用の期待オブジェクトと `objdiff.json` |
| `tools/m2c_func.py`, `tools/trymatch.py`, `tools/splice.py` | m2c による最初の下書き、マッチングの試行、関数をファイルへ組み込む作業 |
| `tools/fullasm.py` | すべての関数のアセンブリ（一致済みの関数も含む） |
| `tools/boundaries.py`, `tools/split_c.py`, `tools/add_c_file.py` | ファイル境界: その根拠、C ファイルの分割、新しいファイルの切り出し |
| `tools/datacheck.py` | C ファイルのデータセクションを製品版と比較 |
| `tools/share_symbols.py`, `tools/map_versions.py`, `tools/version_fallback.py` | 北米版と日本版の間で名前と C を共有 |

北米版と日本版は同じ C からビルドします。日本版だけの違いは `#if VERSION_JP` の中に書きます。同じ C から、ネイティブの PC 移植版 [EtherAura/xenogears-pc](https://github.com/EtherAura/xenogears-pc) もビルドできます。`#ifdef PLATFORM_PC` ブロックはこの移植版のためのもので、PlayStation 向けのビルドでは何も生成しません。

## リポジトリの構成

| パス | 内容 |
|---|---|
| `config/<ver>/` | バージョンごとのディスクのマニフェスト、splat の設定、シンボルの一覧 |
| `config/build.yaml` | パスごとのコンパイラーのプリセット |
| `src/`, `include/` | 逆コンパイルした C（バージョン間で共有） |
| `tools/` | セットアップ、抽出、ビルド、マッチングのスクリプト |

生成物で、コミットしないもの: `disc/`（抽出した製品版のファイル）、`asm/`、`linker/`、`build/`、`expected/`、`tools/toolchain/`、`.venv/`。

## 謝辞

[splat](https://github.com/ethteck/splat)、[spimdisasm](https://github.com/Decompollaborate/spimdisasm)、[maspsx](https://github.com/mkst/maspsx)、[decompals の old-gcc](https://github.com/decompals/old-gcc) のビルド、[objdiff](https://github.com/encounter/objdiff)、[m2c](https://github.com/matt-kempster/m2c)、[psx_psyq_signatures](https://github.com/lab313ru/psx_psyq_signatures) を使って作っています。ハードウェアとゲームの挙動については、[psx-spx](https://psx-spx.consoledev.net/) と yaz0r 氏の [Noah](https://github.com/yaz0r/Noah) を参考にしました。
