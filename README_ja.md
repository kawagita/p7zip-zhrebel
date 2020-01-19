Zip header rebel
================

ZIPアーカイブのヘッダをいじる程度のプログラムですが、追加した`uh`コマンドは新しくアーカイブが作れません。"AS IS"で提供されていることを念頭にお使い下さい。プログラムの詳細は英語のREADME\.mdにあります。大して難しい単語は使っておらず、さらっと目を通せたはずです。あなたがこれを使う必要があるどうか、今からZIPファイルを取り巻く現状について整理しましょう。

ここではWindows 10 64ビット版上でオープンソースの[7-Zip](https://www.7-zip.org/)と[Info-ZIP](http://infozip.sourceforge.net/)を使い、Windows 10/Vistaが標準で備える圧縮フォルダーと比較します。コマンドを実行するのでDebian（Windows Subsystem for Linux）とCygwin 64-bitを用意しました。

**7-Zip**

> Igor Pavlov氏によるフリーソフトウェア。ライセンスはLGPLv2.1 + unRAR restrictionです。名前の通り、7zファイルやZIPファイルに（他にもいろいろ）対応し、アーカイブ内のファイルをAESで暗号化が可能。UNIX版はp7zipとして配布されています。その中の7zaコマンドの機能を損なわないように改変したのが7za-zhrebelコマンドです。

**Info-ZIP**

> Info-ZIPプロジェクトで開発されたフリーソフトウェア。修正BSDライセンスに似たライセンスで公開されています。ZIPファイルの操作のみ。zip/unzipコマンドはほとんどのUNIXで動くものの、Windows GUI版のWiZは動作が不安定です。

## ZIPアーカイブの構造

ZIPアーカイブでは先頭からファイルがヘッダと圧縮データとして（ディレクトリはヘッダのみが）並べられます。圧縮データ前のヘッダは`Local file header`と呼ばれ、ファイル名、日時、サイズ等を格納。それらを配置した後に`Central directory`が置かれ、そこに全てのファイルの`Central directory header`群を作って再びファイル情報を納めます。最後に`Central directory`のエントリ数、合計サイズ、最初のオフセットなどの情報を`End of central directory record`に格納してお終い。ファイル毎に圧縮や暗号化が行われるため、ファイルの追加や削除がしやすい構造となっています。

ZIPファイルフォーマットはPKZIPの開発元であるPKWareが公開しています。細かい仕様は以下のリンク先を参考にして下さい。

- **[.ZIP Application Note](https://support.pkware.com/home/pkzip/developer-tools/appnote)**

#### ファイルヘッダ

各ファイルに対して`Local file header`と`Central directory header`の二つのヘッダがあります。両者は多くのフィールドが重複していますが、後者は拡張フィールドを除いて前者の内容をすべて含んでいます。ZIPアーカイブ内には`End of central directory record`が必ず存在するので、ソフトウェアは`Central directory header`を参照してファイルにアクセスするようです。7-ZipやInfo-ZIPでもこちらのヘッダの情報が表示されます。

|   Size   | Local file header         | Central directory header        |                                 |
| :------: | :------------------------ | :------------------------------ | :------------------------------ |
| 4 bytes  | signature (0x04034b50)    | signature  (0x02014b50)         | "PK"から始まる識別子            |
| 2 bytes  |  -                        | version made by                 | ZIPバージョン／作成環境を示す値 |
| 2 bytes  | version needed to extract | version needed to extract       | 展開に必要なZIPバージョン       |
| 2 bytes  | general purpose bit flag  | general purpose bit flag        |                                 |
| 2 bytes  | compression method        | compression method              |                                 |
| 2 bytes  | last mod file time        | last mod file time              | DOS時刻形式（２秒精度）         |
| 2 bytes  | last mod file date        | last mod file date              | DOS日時形式                     |
| 4 bytes  | crc-32                    | crc-32                          | ファイルのCRC32値               |
| 4 bytes  | compressed size           | compressed size                 | 圧縮データのサイズ              |
| 4 bytes  | uncompressed size         | uncompressed size               | ファイルのサイズ                |
| 2 bytes  | file name length          | file name length                | ファイル名の長さ                |
| 2 bytes  | extra field length        | extra field length              | 拡張フィールドの長さ            |
| 2 bytes  |  -                        | file comment length             | コメントの長さ                  |
| 2 bytes  |  -                        | disk number start               |                                 |
| 2 bytes  |  -                        | internal file attributes        |                                 |
| 4 bytes  |  -                        | external file attributes        | ファイル属性とパーミッション    |
| 4 bytes  |  -                        | relative offset of local header | Local file headerの開始位置     |
| variable | file name                 | file name                       | ファイル名（パス）              |
| variable | extra field               | extra field                     | 拡張フィールド                  |
| variable |  -                        | file comment                    | コメント                        |
| variable | file data                 |  -                              | 圧縮データ                      |

#### 拡張フィールド

拡張フィールドにはヘッダIDとサイズからなるヘッダとデータ（拡張ブロック）が並べられます。ヘッダIDは拡張ブロックにあるデータの種類を表し、0から31まではPKWareに予約され、それ以降は開発者用です。拡張フィールドをサポートしない場合は安全にファイルを飛ばします。ただ、サポートするとしてもあらゆる拡張ブロックに対応する事はありません。7-Zipがアーカイブ内のファイルを更新する時はファイルヘッダからAES暗号化以外の拡張ブロックを全て削除しています。

|   Size   | Extra Field |                            |
| :------: | :---------- | :------------------------- |
| 4 bytes  | Header1     | データ１のヘッダID／サイズ |
| variable | Data1       | データ１                   |
| 4 bytes  | Header2     | データ２のヘッダID／サイズ |
| variable | Data2       | データ２                   |
|   ...    |             |                            |
| 4 bytes  | HeaderN     | データＮのヘッダID／サイズ |
| variable | DataN       | データＮ                   |

## ZIPアーカイブにおける文字エンコーディング

ZIPアーカイブは最後尾にファイル自体のコメントを格納する場所があります。しかし、[APPNOTE.TXT](https://www.pkware.com/documents/casestudies/APPNOTE.TXT)に何も書かれていないので実装次第です。一方、前述の`Local file header`と`Central directory header`ではファイル名やコメントの文字エンコーディングは`general purpose bit flag`に設定されます。11ビット目にフラグが立っているとUTF-8、そうでなければCP437（ASCII＋α）になります。とはいえ、この仕様はZIPバージョン6.3.0からであり、日本語への対応もそれまでの実装によります。

#### ファイル名 フィールド （エンコード）

ZIPアーカイブへファイルを追加する時にファイル名はバイトデータに変換してヘッダに格納されます。Windows用ソフトウェアと圧縮フォルダ―はCP932、DebianやCygwinのコマンドはUTF-8です。やはりCP932はもうしばらくWindowsの標準らしい……

7-ZipではUTF-8とCP932に対応しています。GUI版の7zFMはパラメーター欄に`cu+`を入力すればUTF-8に変換でき、7zaコマンドは`-mcl+`スイッチを指定すれば現在のロケールに従います（Debianは不可）。さらに、7za-zhrebelコマンドは指定したロケールに変換できるようにしました。

|       |  7za 16.02   | 7zFM 19.00 |  zip 3.0   |  WiZ 5.03  | Win10 圧縮 | Vista 圧縮 |
| :---: | :----------: | :--------: | :--------: | :--------: | :--------: | :--------: |
| UTF-8 |      〇      |     〇     |     〇     |     ×     |     ×     |     ×     |
| CP932 | SJISロケール |     〇     |     ×     |     〇     |     〇     |     〇     |

#### ファイル名 フィールド （デコード）

ZIPアーカイブからファイルを参照・展開する時にヘッダに格納したバイトデータがファイル名に変換されます。開発を停止したWiZやWindows Vistaの圧縮フォルダーはUTF-8フラグを無視し、バイトデータをCP932として扱います。unzipコマンドはその逆です。

その他は仕様通りにフラグを解釈します。ヘッダにフラグが立ってない場合、Windows 10の圧縮フォルダ―はCP932として変換し、7zFMも同じです。7zaコマンドは現在のロケールに従います（Debianは不可）。エンコードと同様、7za-zhrebelコマンドにはロケールを指定できるようにしました。

|                    |  7za 16.02   | 7zFM 19.00 | unzip 6.00 |  WiZ 5.03  | Win10 圧縮 | Vista 圧縮 |
| :----------------- | :----------: | :--------: | :--------: | :--------: | :--------: | :--------: |
| UTF-8（フラグ有り）|      〇      |     〇     |     〇     |     !?     |     〇     |     ×     |
| UTF-8（フラグ無し）|      〇      |     ×     |     〇     |     !?     |     ×     |     ×     |
| CP932（フラグ無し）| SJISロケール |     〇     |     ×     |     〇     |     〇     |     〇     |
| CP932（フラグ有り）|      ×      |     ×     |     ×     |     〇     |     ×     |     〇     | 

×は文字化け、!?は異常終了

#### Info-ZIP Unicodeパス／コメント拡張フィールド

Info-ZIPにはWin32バイナリのコマンド（zip.exe/unzip.exe）が存在します。zip.exeはZIPアーカイブへファイルを追加する時のエンコーディングがGUI版のWizと同じくCP932です。しかし、拡張フィールドに`Info-ZIP Unicode Path Extra Field`や`Info-ZIP Unicode Comment Extra Field`を追加してUTF-8のバイトデータを格納します。この拡張ブロックは7zaコマンドからも参照されるので、冗長な代わりにレガシーなWindows環境とUNIXの両方でZIPファイルを利用できるメリットがあります。

## ZIPアーカイブにおけるファイル時刻

ZIPアーカイブはファイルの更新日時がヘッダの`last mod file time`と`last mod file date`に格納されます。MS-DOSの頃からFATに用いられた時刻形式です。それとは別に、拡張フィールドにNTFSやUNIXのタイムスタンプに合わせた時刻形式のデータを追加されることがあります。

#### 最終ファイル更新日付／時刻

`last mod file date`と`last mod file time`は２バイトずつのフィールドです。それぞれ、ビット単位で年月日や時分秒を分けて日時を表現します。日付が比較的長い割に、時刻は２秒精度であり、奇数秒のタイムスタンプを＋１して格納されます。

Last mod file date:

| Bit allocation   | Max |            |
| :--------------- | --: | :--------- |
| 0000000000011111 |  31 | 日（O.K.） |
| 0000000111100000 |  16 | 月（O.K.） |
| 1111111000000000 | 127 | 年         |

Last mod file time:

| Bit allocation   | Max |                  |
| :--------------- | --: | :--------------- |
| 0000000000011111 |  31 | 秒（足りない!!） |
| 0000011111100000 |  63 | 分（O.K.）       |
| 1111100000000000 |  31 | 時（O.K.）       |


#### NTFS拡張フィールド

7-ZipはZIPアーカイブへファイルを追加する時に`Central directory header`の拡張フィールドへ`NTFS Extra Field`を追加します。この拡張ブロックはタイムスタンプの更新日時、アクセス日時、作成日時を100ナノ秒単位で格納可能です。

Windows GUI版の7zFMはファイル時刻がそのまま使われますが、（UNIXのコードが元になっている）7zaコマンドでは秒単位になり、作成日時でなく状態変更時刻が格納されます。おまけに、その時刻はファイルを展開する時に復元されません。7za-zhrebelコマンドはCygwinでファイルの作成日時を取得し、`./build_install.sh`の実行時に`-nsec`を指定した場合にナノ秒が使われます。

|  Size   | Value    |                               |
| :-----: | :------- | :---------------------------- |
| 2 bytes | 0x000a   | ヘッダID                      |
| 2 bytes | 0x0020   | データサイズ                  |
| 4 bytes | Reserved |                               |
| 2 bytes | 0x0001   |                               |
| 2 bytes | 0x0018   | 時刻フィールドの長さ          |
| 8 bytes | Mtime    | 更新日時                      |
| 8 bytes | Atime    | アクセス日時                  |
| 8 bytes | Ctime    | 作成日時（7zaは状態変更時刻） |

#### 拡張タイムスタンプ

Info-ZIPはZIPアーカイブへファイルを追加する時にファイルヘッダの拡張フィールドへ`Extended Timestamp`を追加します。この拡張ブロックは`Local file header`でUNIX時刻（1970年1月1日からの経過秒）の更新日時、アクセス日時、作成日時を格納できます。

DebianやCygwinのzipコマンドは更新日時とアクセス日時が、Win32バイナリのzip.exeは作成日時までが格納されます。また、7-Zipから更新日時のみが参照されるので、7za-zhrebelコマンドはすべての日時に変え、更新日時とアクセス日時を格納できるようにしました。作成日時を省いたのはUNIXのコードでファイルに設定できず、NTFS拡張フィールドのCtimeも復元されないからです。

Local file header:

|  Size   | Value             |                                |
| :-----: | :---------------- | :----------------------------- |
| 2 bytes | 0x5455            | ヘッダID                       |
| 2 bytes | 0x0009 (0x000d)   | データサイズ                   |
| 1 byte  | 0x0003 (0x0007)   | 日時データが存在するかのフラグ |
| 4 bytes | ModTime           | 更新日時                       |
| 4 bytes | AcTime            | アクセス日時                   |
| 4 bytes | -------- (CrTime) | 作成日時（zip.exeのみ格納）    |

Central directory header:

|  Size   | Value           |                                |
| :-----: | :-------------- | :----------------------------- |
| 2 bytes | 0x5455          | ヘッダID                       |
| 2 bytes | 0x0005          | データサイズ                   |
| 1 byte  | 0x0003 (0x0007) | 日時データが存在するかのフラグ |
| 4 bytes | ModTime         | 更新日時                       |

## ZIPアーカイブにおけるファイル属性

ZIPアーカイブはファイルの属性情報がヘッダの`internal file attributes`や`external file attributes`に格納されます。仕様にはあまり詳細が定められていませんが、ある程度決まっている様子です。

#### 作成バージョン（作成環境） フィールド

ファイルヘッダで`version made by`の上位バイトは作成環境を示す値が格納され、`external file attributes`に格納された属性情報の互換性を表わします。7zFM、WiZ、zip.exeはMS-DOS（FAT）で、7zaとzipコマンドはUNIXです。7za-zhrebelコマンドは表示や作成の時にZIPアーカイブ内の属性情報がどちらかを指定できます。

#### 内部ファイル属性 フィールド

`internal file attributes`はファイル内容に関するフラグが格納されます。１ビット目にフラグが立っていればデータがASCIIのみかテキスト、それ以外はバイナリです。改行コードの変換ができるInfo-ZIPのみが設定していました。

#### 外部ファイル属性 フィールド

`external file attributes`は４バイトのフィールドであり、データは`version made by`の設定に従うとなっています。作成環境がFATのソフトウェアでは下位２バイトにWindowsのファイル属性が格納されます。UNIXではそれに加えて上位２バイトにパーミッションが格納されます。

7za-zhrebelコマンドはZIPアーカイブの（指定もできる）作成環境に応じ、ファイル種別以外の下表にあるデータを変えられます。ビットが立ったファイル属性やパーミッションには`l`コマンドで角括弧内のアルファベットやハイフンの替わりに`r/w/x`が表示されます。

Lower two bytes:

| Bit allocation   |                                                     |
| :--------------- | :-------------------------------------------------- |
| 0000000000000001 | [R]eadOnly : 読み込み専用                           |
| 0000000000000010 | [H]idden : 隠しファイル                             |
| 0000000000000100 | [S]ystem : システムファイル                         |
| 0000000000001000 | [D]irectory : ディレクトリ                          |
| 0000000000010000 | [A]rchive : バックアップ済みを示す                  |
|    ......        |                                                     |
| 0000000001000000 | [N]ormal : 属性がないファイル（Info-ZIPは付けない） |
|    ......        |                                                     |
| 0010000000000000 | Not [I]ndexed : インデックス付きでない事を示す      |
|    ......        |                                                     |
| 1000000000000000 | UNIX拡張ビット（7zaが付けるフラグ）                 |

Upper two bytes:

| Bit allocation   |                                                        |
| :--------------- | :----------------------------------------------------- |
| 0000000000000111 | その他のファイルアクセス許可（chmodコマンドの8進値）   |
| 0000000000111000 | グループのファイルアクセス許可（chmodコマンドの8進値） |
| 0000000111000000 | 所有者のファイルアクセス許可（chmodコマンドの8進値）   |
|    ......        |                                                        |
| 1111000000000000 | ファイル種別（ディレクトリは4、ファイルは8）           |
