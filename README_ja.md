Zip header rebel
================

ZIPアーカイブのヘッダをいじる程度のプログラムですが、追加した`uh`コマンドは新しくアーカイブが作れません。"AS IS"で提供されていることを念頭にお使い下さい。プログラムの詳細は英語のREADME\.mdにあります。大して難しい単語は使っておらず、さらっと目を通せたはずです。あなたがこれを使う必要があるどうか、今からZIPファイルを取り巻く現状について整理しましょう。

ここではWindows 10 64ビット版上でオープンソースの[7-Zip](https://www.7-zip.org/)と[Info-ZIP](http://infozip.sourceforge.net/)を使い、Windows 10/Vistaが標準で備える圧縮フォルダーと比較します。コマンドを実行するのでDebian（Windows Subsystem for Linux）とCygwin 64-bitを用意しました。

**7-Zip**

> Igor Pavlov氏によるフリーソフトウェア。ライセンスはLGPLv2.1 + unRAR restrictionです。名前の通り、7zファイルやZIPファイルに（他にもいろいろ）対応し、アーカイブ内のファイルをAESで暗号化が可能。UNIX版はp7zipとして配布されています。

**Info-ZIP**

> Info-ZIPプロジェクトで開発されたフリーソフトウェア。修正BSDライセンスに似たライセンスで公開されています。ZIPファイルの操作のみ。zip/unzipコマンドはほとんどのUNIXで動くものの、Windows GUI版のWiZは動作が不安定です。

## ZIPアーカイブの構造

ZIPアーカイブ内でファイルはヘッダと圧縮データとして（ディレクトリはヘッダのみが）先頭から並べられます。ヘッダにはファイル名、日時、サイズといった情報を格納。それらを配置した後に`Central directory`が置かれ、そこで全てのファイルのヘッダだけを再び集めます。最後に、`Central directory`のエントリ数、合計サイズ、最初のオフセットなどの情報を付けてお終いです。ファイル毎に圧縮や暗号化が行われるため、ファイルの追加や削除がしやすい構造となっています。

ZIPファイルフォーマットはPKZIPの開発元であるPKWareが公開しています。細かい仕様は以下のリンク先を参考にして下さい。

- **[.ZIP Application Note](https://support.pkware.com/home/pkzip/developer-tools/appnote)**

#### ファイルヘッダ

ファイルヘッダには`Local file header`と`Central directory header`があります。両者は多くのフィールドが重複していますが、後者は拡張フィールドを除いて前者を含んだ内容になっています。`relative offset of local header`によってファイルにアクセスできることもあり、7-ZipやInfo-ZIPはこちらのヘッダの情報を参照するようです。

|   Size   | Local file header         | Central directory header        |                         |
| :------: | :------------------------ | :------------------------------ | :---------------------- |
| 4 bytes  | signature (0x04034b50)    | signature  (0x02014b50)         | "PK"から始まる識別子    |
| 2 bytes  |  -                        | version made by                 | ZIPバージョンと作成OS   |
| 2 bytes  | version needed to extract | version needed to extract       | 7-Zipでは上記と同じ     |
| 2 bytes  | general purpose bit flag  | general purpose bit flag        |                         |
| 2 bytes  | compression method        | compression method              |                         |
| 2 bytes  | last mod file time        | last mod file time              | DOS時刻形式（２秒精度） |
| 2 bytes  | last mod file date        | last mod file date              | DOS日時形式             |
| 4 bytes  | crc-32                    | crc-32                          | ファイルのCRC32値       |
| 4 bytes  | compressed size           | compressed size                 | 圧縮データのサイズ      |
| 4 bytes  | uncompressed size         | uncompressed size               | ファイルのサイズ        |
| 2 bytes  | file name length          | file name length                | ファイル名の長さ        |
| 2 bytes  | extra field length        | extra field length              | 拡張フィールドの長さ    |
| 2 bytes  |  -                        | file comment length             | コメントの長さ          |
| 2 bytes  |  -                        | disk number start               |                         |
| 2 bytes  |  -                        | internal file attributes        |                         |
| 4 bytes  |  -                        | external file attributes        |                         |
| 4 bytes  |  -                        | relative offset of local header | Local file headerの位置 |
| variable | file name                 | file name                       | ファイル名（パス）      |
| variable | extra field               | extra field                     | 拡張フィールド          |
| variable |  -                        | file comment                    | コメント                |
| variable | file data                 |  -                              | 圧縮データ              |

拡張フィールドにはヘッダIDとデータ長からなるヘッダとデータ（拡張ブロック）が並べられます。ヘッダIDは拡張ブロックにあるデータの種類を表し、0から31まではPKWareに予約され、それ以降は開発者用です。拡張フィールドをサポートしない場合は安全にファイルを飛ばします。ただし、サポートするとしてもあらゆる拡張ブロックに対応する事はありません。7-ZipではZIPアーカイブ内のファイルを更新する時にファイルヘッダからAES暗号化以外のデータは全て削除しています。

```
  header1+data1 + header2+data2 ...
```

|   Size   | Extra block |                |
| :------: | :---------- | :------------- |
| 2 bytes  | Header ID   | ヘッダID       |
| 2 bytes  | Data Size   | データサイズ   |
| variable | Data Block  | データブロック |

## ZIPアーカイブにおける文字エンコーディング

ZIPアーカイブは最後尾にファイル自体のコメントを格納する場所があります。しかし、[APPNOTE.TXT](https://www.pkware.com/documents/casestudies/APPNOTE.TXT)に何も書かれていないので実装次第です。一方、前述の`Local file header`と`Central directory header`ではファイル名やコメントの文字エンコーディングは`general purpose bit flag`に設定されます。11ビット目にフラグが立っていればUTF-8、そうでなければCP437（ASCII＋α）になります。とはいえ、この仕様はZIPバージョン6.3.0からであり、それまでの実装にもよります。

#### ファイル追加時のエンコード

ZIPアーカイブへファイルを追加する時にファイル名をバイトデータに変換してヘッダに格納します。Windowsソフトと圧縮フォルダ―はCP932、DebianやCygwinのコマンドはUTF-8です。WindowsはもうしばらくCP932を使い続けるみたいですが、7-Zipではユーザーに委ねることにしています。GUI版の7zFMはパラーメーターで`cu+`を指定すればUTF-8に変換でき、7zaコマンドはCygwinのja_JP.SJISロケールで`-mcl+`スイッチを付ければCP932も可能です。

|       |  7za 16.02   | 7zFM 19.00 |  zip 3.0   |  WiZ 5.03  | Win10 圧縮 | Vista 圧縮 |
| :---: | :----------: | :--------: | :--------: | :--------: | :--------: | :--------: |
| UTF-8 |      〇      |     〇     |     〇     |     ×     |     ×     |     ×     |
| CP932 | CygwinでSJIS |     〇     |     ×     |     〇     |     〇     |     〇     |

#### ファイル参照・展開時のデコード

ZIPアーカイブからファイルを参照・展開する時に格納されたバイトデータをファイル名に変換します。開発を停止したWiZやWindows Vistaの圧縮フォルダーはUTF-8フラグを無視し、ファイル名をCP932として扱います。unzipコマンドはその逆です。また、7-Zip（7zaはCygwinのja_JP.SJISロケール）とWindows 10の圧縮フォルダ―はフラグが設定されない場合をCP932と判断しています。

|                    |  7za 16.02   | 7zFM 19.00 | unzip 6.00 |  WiZ 5.03  | Win10 圧縮 | Vista 圧縮 |
| :----------------- | :----------: | :--------: | :--------: | :--------: | :--------: | :--------: |
| UTF-8（フラグ正常）|      〇      |     〇     |     〇     |     ×     |     〇     |     ×     |
| UTF-8（フラグ異常）|      〇      |     〇     |     〇     |     ×     |     ×     |     ×     |
| CP932（フラグ正常）| CygwinでSJIS |     〇     |     ×     |     〇     |     〇     |     〇     |
| CP932（フラグ異常）|      ×      |     ×     |     ×     |     〇     |     ×     |     〇     |

**Info-ZIP Unicode Path/Comment Extra Field**

Info-ZIPにはWin32バイナリのコマンド（zip.exe/unzip.exe）が存在します。zip.exeはZIPアーカイブへファイルを追加する時のエンコーディングがGUI版のWizと同じくCP932です。しかし、拡張フィールドの`Info-ZIP Unicode Path Extra Field`や`Info-ZIP Unicode Comment Extra Field`にUTF-8のバイトデータを格納しています。このデータはunzipだけでなく7zaコマンドからも参照されます。

## ZIPアーカイブにおけるファイル時刻

ZIPアーカイブのファイルヘッダには`last mod file time`と`last mod file date`という２バイトずつのフィールドがあり、ファイルの更新日時を表します。MS-DOSのFATに用いられた時刻形式（２秒精度）です。Windowsの圧縮フォルダ―で作成したアーカイブでは奇数秒が＋１されます。それを避けるためには拡張フィールドに別の時刻形式を格納することになります。

```
  last mod file time (2 bytes)

  xxxxx00000000000 : 0-31 > 24時 （O.K.）
  00000yyyyyy00000 : 0-63 > 60分 （O.K.）
  00000000000zzzzz : 0-31 < 60秒 （足りない！）
```

**NTFS Extra Field**

7-ZipではZIPアーカイブへファイルを追加する時に`Central directory header`の拡張フィールドへ`NTFS Extra Field`を付け加えます。この拡張ブロックはタイムスタンプの更新日時、アクセス日時、作成日時を100ナノ秒単位で格納可能です。Windows GUI版の7zFMはファイル時刻がそのまま使われますが、（UNIXのコードが元になっている）7zaコマンドでは秒単位になり、作成日時ではなく状態変更時刻が格納されます。おまけに、その時刻はファイルを展開する時に復元されません。

| 7zFM/7za |  Size   |                               |
| :------: | :-----: | :---------------------------- |
| 0x000a   | 2 bytes | ヘッダID                      |
| 0x0020   | 2 bytes | データサイズ                  |
| Reserved | 4 bytes |                               |
| 0x0001   | 2 bytes |                               |
| 0x0018   | 2 bytes | 時刻フィールドの長さ          |
| Mtime    | 8 bytes | 更新日時                      |
| Atime    | 8 bytes | アクセス日時                  |
| Ctime    | 8 bytes | 作成日時（7zaは状態変更時刻） |

**Extended Timestamp**

Info-ZIPではZIPアーカイブへファイルを追加する時にファイルヘッダの拡張フィールドへ`Extended Timestamp`を付け加えます。この拡張ブロックは`Local file header`でUNIX時刻（1970年1月1日からの経過秒）の更新日時、アクセス日時、作成日時を格納でき、`Central directory header`より情報を多く含んでいます（下表参照）。DebianやCygwinのzipコマンドは更新日時とアクセス日時、Win32バイナリのzip.exeはさらに作成日時まで対応します。また、7-Zipからは更新日時のみが参照されます。

Local file header:

| Debian/Cygwin | Win32バイナリ |  Size   |                                |
| :-----------: | :-----------: | :-----: | :----------------------------- |
|    0x5455     |    0x5455     | 2 bytes | ヘッダID                       |
|    0x0009     |    0x000d     | 2 bytes | データサイズ                   |
|    0x0003     |    0x0007     | 1 byte  | 日時データが存在するかのフラグ |
|    ModTime    |    ModTime    | 4 bytes | 更新日時                       |
|    AcTime     |    AcTime     | 4 bytes | アクセス日時                   |
|       -       |    CrTime     | 4 bytes | 作成日時                       |

Central directory header:

| Debian/Cygwin | Win32バイナリ |  Size   |                                |
| :-----------: | :-----------: | :-----: | :----------------------------- |
|    0x5455     |    0x5455     | 2 bytes | ヘッダID                       |
|    0x0005     |    0x0005     | 2 bytes | データサイズ                   |
|    0x0003     |    0x0007     | 1 byte  | 日時データが存在するかのフラグ |
|    ModTime    |    ModTime    | 4 bytes | 更新日時                       |
