Zip header rebel
================

This program is to change file headers in zip archive by p7zip command. If you `make zhrebel` on a root directory of source package, `bin/7za-zhrebel` is created. Its executable could set and change character encodings or timestamps to the local file and central directory header by `uh` command with new switches.

## Install

Execute `build_install.sh` in the project root. And then `7za-rebel` is installed to `/usr/local/bin` by default.

```
  ./build_install.sh <target_system> [ <install_directory> ]
```

Otherwise, you can manually build it by overwriting `makefile.machine` to `makefile.XXXXX` (target system), and copy to any directory.


## Command

### uh (Update Header) command <a name="uh"></a>

Updates file headers in archive.

**Syntax**

```
  7za-zhrebel uh <archive_name> [ <filename> ... ]
```

By `uh` command, file data are not compressed but copied, and only header informations are updated. Headers which filename is specified at the tail of an archive name on command line are replaced in the same archive. However, `'*'` (in quotes) or no filename is put on the last, all directories or files are targeted.

Never change the modification time of encrypted data by Info-ZIP or remove its data descriptor. If done, original file can not be extracted from an archive. This command asks to change such header, so that you must select to continue or not.

**Notes**

This command can't be used with `-u` switch.

## Switches

### -uh (Update header for only directory/file) switch

Updates header information for only directories or files.

**Syntax**

```
  -uh{d|f}
```

|{id}| Item Type |
| :- | :-------- |
| d  | Directory |
| f  | File      |

**Commands that can be used with this switch**

[uh (Update Header)](#uh)

### -h (Set Header information) switch

Specifies the header information.

**Syntax**

```
  -h<header_parameters>
```

**Notes:** "Default value" in switches descriptions means the value that will be used if switch is not specified.<br>
It's allowed to use reduced forms for boolean switches: **sw+** or **sw** instead **sw=on**, and **sw-** instead of **sw=off**.

| Parameter                                          |  Default  | Description                                              |
| :------------------------------------------------- | :-------: | :------------------------------------------------------- |
| init=[off \| on]                                   |  off[^1]  | Initializes file information for setting by p7zip.       |
|[lc={Locale}](#lc)                                  |   UTF-8   | Encodes and decodes filename or comment for Locale.      |
|[en={Locale}](#lc)                                  |   UTF-8   | Encodes filename or comment for locale.                  |
|[de={Locale}](#lc)                                  |   UTF-8   | Decodes filename or comment for locale.                  |
| dc={Locale}                                        |   UTF-8   | Decodes only Comment for locale.                         |
|[t=[WIN \| UNIX \| DOS \| 0]](#t)                   | (WIN[^1]) | Stores Time field of a format type to headers.           |
|[tm={Timestamp}](#tm)                               |    [^2]   | Sets Modification time to timestamp.                     |
|[ta={Timestamp}](#tm)                               |    [^2]   | Sets Last accessed time to timestamp.                    |
|[tc={Timestamp}](#tm)                               |    [^2]   | Sets Creation time to timestamp.                         |
| tfm=[off \| on]                                    |    off    | Sets all times from Modification time.                   |
| tz=[[+\|-]hh:mm]                                   |   00:00   | Adjusts all times with a Time Zone.                      |
|[f=[WIN \| UNIX \| BOTH]](#f)                       | (BOTH[^1])| Sets host system on which file information is compatible.|
| iz=[off \| on]                                     |    off    | Sets file information with ownership field like Info-Zip.|
|[attrib=[[+\|-][R \| A \| S \| H \| I]]...](#attrib)|    [^2]   | Sets or clears file Attribute of Windows.                |
| mod={OctalMode}                                    |    [^2]   | Sets file permissions to octal number of Unix Mode bits. |
| uid={NumUID}                                       |    [^2]   | Sets file ownership to number of Unix UID.               |
| gid={NumGID}                                       |    [^2]   | Sets file ownership to number of Unix GID.               |
| own=[off \| on]                                    |    off    | Stores file Ownership field to headers.                  |
| dd=[off \| on]                                     |    off    | Stores data descriptor which follows compressed data.    |
| ex=[off \| on]                                     |    off    | Leaves all Extra fields in headers.                      |
|[exa={HeaderID(s)}](#exa)                           |    [^3]   | Leaves Extra field of header ID(s) in headers.           |
|[exd={HeaderID(s)}](#exa)                           |    [^3]   | Removes Extra field of header ID(s) from headers.        |

[^1]: #note1
[^2]: #note2
[^3]: #note3

<a name="note1">^1</a>: Only when added, Windows attribute and file permissions is set as Unix compatibility and NTFS time is stored.<br>
<a name="note2">^2</a>: Informations are read from files on disk if exists or headers in archive.<br>
<a name="note3">^3</a>: Values of `exa` or `exd` parameter is effective in the state of `ex=off` or `ex=on`.

**lc={Locale}** <a name="lc"></a><br>
**en={Locale}** <br>
**de={Locale}**

> Sets the locale for filename and comment read from and/or written to file headers. The codeset must be specified though no language code is completed by system locale.

**t=[WIN | UNIX | DOS | 0]** <a name="t"></a>

> Sets the time format of file headers. NTFS or Unix time is stored in an extra field if `WIN` or `UNIX`. Otherwise, only DOS time format. Especially, that's cleared if `0` is specified.

**tm={Timestamp}** <a name="tm"></a><br>
**ta={Timestamp}** <br>
**tc={Timestamp}** <br>

> Sets the modification, last accessed, or creation time of file headers to a timestamp. This value is `YYYYMMDDhhmm[.ss]` or `YYYY-MM-DDThh:mm:ss[.nnnnnnn]` but its seconds or nanoseconds is counted up or ignored for the time format. Moreover some time can't be stored, see below. 

| Format    | Resolution | Present time fields                 |
| :-------- | :--------: | :---------------------------------- |
| NTFS Time | 100 nsec   | all times of a file                 |
| Unix Time | 1 second   | modification and last accessed time |
| DOS Time  | 2 seconds  | only modification time              |

**f=[WIN | UNIX | BOTH]** <a name="f"></a>

> Sets the host system on which file information is compatible. Windows attribute or file permissions is set only as FAT or Unix file if `WIN` or `UNIX`. Otherwise, both information is set as Unix compatibility.

**attrib=[[+\|-][R \| A \| S \| H \| I]]...** <a name="attrib"></a>

> Sets or clears file attribute for `R` (ReadOnly), `A` (Archive), `S` (System), `H` (Hidden), and `I` (Not Content Indexed) preceded by `+` or `-`.

**exa={HeaderID(s)}** <a name="exa"></a><br>
**exd={HeaderID(s)}**

> Sets the list of extra fields which are left or removed by `uh` command in the state of `ex=off` or `ex=on`. These values are hexadecimal preceded by `0x` or decimal number of header ID (which is printed by `l` command with `-slt` switch) and following ones are separated by a comma. This parameter only changes headers so that can't control ZIP64 and encryption fields. Besides the above parameters set NTFS or Unix time fields.

**Commands that can be used with this switch**

a (Add), rn (Rename), u (Update), [uh (Update Header)](#uh)

and, only `lc`, `de`, and `dc` parameters:

e (Extract), l (List), T (Test), x (Extract with full paths)

## License

This program is published under LGPL v2.1.

See `p7zip_16.02/DOC/License.txt` about the original p7zip license.

