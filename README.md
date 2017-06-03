# mozc-pseudo-reconv

mozc.el での再変換もどき

decompjp.el でローマ字に戻したものを boild-mozc でローマ字漢字変換します。

## 使用方法
* [decompjp.el](https://gist.github.com/kiwanami/1601147) とこれが使用する [mecab](http://taku910.github.io/mecab/) をインストールしてください。mecab 辞書の文字コードは UTF-8 でインストール。他の文字コードでの動作は未確認です。また、decompjp.el は chasen でも使えるようですがこちらも未確認です。

  Windows では [自己解凍インストーラ(mecab-0.996.exe)](https://drive.google.com/uc?export=download&id=0B4y35FiV1wh7WElGUGt6ejlpVXc)が用意されています(32bit版)。
  インストール後 mecab へ環境変数 PATH を通しておいてください。

  M-x dcj:reverse-test でローマ字への変換ができることを確認してください。

* [boild-mozc](https://github.com/tadanagao/boiled-mozc) をインストールし、boild-mozc でローマ字からの漢字変換ができることを確認してください。

* mozc-pseudo-reconv.el を load-path の通った場所に置き、init.el に

    (require 'mozc-pseudo-reconv)

  を追加、適当なキーにmozc-pseudo-reconv を割り当てて下さい。

  私の場合は

  (global-set-key [convert] 'mozc-pseudo-reconv)

  で(Windows では emacs 本体の IME を無効にした上で)変換キーに割り当てています。

  割り当てたキーを打つとカーソル位置より前方の適当な位置(行頭や、記号/句読点などの直後)までを再変換します。リージョンが指定されていればその範囲を再変換します。

## reverse-translate-driver-mecab-module
* decompjp.el の mecab による漢字から読みへの変換を emacs-25 以降の dynamic module にしたものです。

* Linux でパッケージから mecab をインストールしている場合は libmecab-dev パッケージをインストールしておいてください。

* emacs ソースの modules/ 下に適当なディレクトリを掘ってそこに Makefile と reverse-translate-driver-mecab-module.c をコピーしてください。またはこれらのあるディレクトリに emacs ソースの src/emacs-module.h をコピーしてください。

    Windows (cygwin, mingw)の場合はここにさらに mecab.h と libmecab.dll をコピーして下さい。32bit版はインストーラで指定したインストール場所の sdk/mecab.h と bin/libmecab.dll にあります。64bit版は配布されていないので mecab ソースからコンパイルしてください。コンパイル方法はググれば出てくると思いますが、私の場合は以下のようにしました。

  - VisualStudio 2017インストール

  - [mecab-0.996.tar.gz](https://drive.google.com/uc?export=download&id=0B4y35FiV1wh7cENtOXlicTFaRUE) を展開し mecab-0.996/src/ 下で Makefile.msvc.in を Makefile にコピーし、LDFLAGS の /MACHINE:X86 を /MACHINE:X64 に、DEFS の @DIC\_VERSION@ を 102 に、@VERSION@ を 0.996 に、-DMECAB\_DEFAULT\_RC= の値を自分のインストール場所に合わせて修正 (mecab-0.996.src.Makefile.diff 参照)

  - feature\_index.cpp、writer.cpp 修正 (mecab-0.996.src.feature_index.cpp.diff、mecab-0.996.src.writer.cpp.diff 参照)

  - スタートメニューから VisualStudio 2017 の x64 Native Tools コマンドプロンプト を起動し、mecab-0.996/src/ 下で nmake を実行。できた libmecab.dll と mecab.h を上記の場所にコピーしてください。

  - mecab-0.996/mecabrc.in を mecab-0.996/src/Makefile の -DMECAB\_DEFAULT\_RC で指定したファイルにコピーして dicdir = を辞書のあるディレクトリに修正してください。

  - 辞書もソースからインストールする場合は [mecab-ipadic-2.7.0-20070801.tar.gz](https://drive.google.com/uc?export=download&id=0B4y35FiV1wh7MWVlSDBCSXZMTXM) を展開し、上記 mecab-0.996 のコンパイル後に mecab-0.996/ にできている mecab-dict-index.exe (と libmecab.dll) を PATH の通った場所に置いて

    mecab-dict-index -d . -o . -f EUC-JP -t utf-8

    を実行。dicrc char.bin left-id.def matrix.bin pos-id.def rewrite.def right-id.def sys.dic unk.dic を mecabrc の dicdir = で指定したディレクトリにコピーしてください。

* Windows では 32bit版は MSYS2 MinGW 32-bit、64bit版は MSYS2 MinGW 64-bit で

    $ make SO=dll

    その他の場合

    $ make [MECAB=(mecabインストール場所)]

    として reverse-translate-driver-mecab-module.cを コンパイルしてください。make コマンドは GNU Make です。なお Windows で mingw でコンパイルしたものは cygwin 版 emacs でも使用できます。

    MECAB=... は mecab をソースからインストールした場合に configure で --prefix= を指定したなどで /usr/ または /usr/local/ にインストールされていないときに指定してください。/usr/ または /usr/local/ 下に include/mecab.h、lib/libmecab.a があるなら指定不要です。

* できた reverse-translate-driver-mecab-module.dll(Windows) または reverse-translate-driver-mecab-module.so(その他) を load-path の通った場所に置いてください。

    Windowsの場合はさらに libmecab.dll を PATH の通った場所か emacs.exe のある場所に置いてください。

    Windows の場合 libmecab.dll デフォルトの mecabrc ファイルでうまく動かない場合があるようで(私の場合VisualStudio 2015でコンパイルした場合に発生しました)、その場合 init.el で mecab-rcfile を設定してください。
mecabrc ファイルがデフォルトの場所以外にある場合も mecab-rcfile を設定してください。

以上