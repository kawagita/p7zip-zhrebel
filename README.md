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

### uh (Update Header) command <a name="command_uh"></a>

Updates file headers in archive.

**Syntax**

```
  7za-rebel uh <archive_name> [ <filename> ... ]
```

By `uh` command, file data are not compressed but copied, and only header informations are updated. Headers which filename is specified at the tail of an archive name on command line are replaced in the same archive. However, `'*'` (in quotes) or no filename is put on the last, all directories or files are targeted.

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

[uh (Update Header)](#command_uh)

### -h (Set Header information) switch

Specifies the header information.

**Syntax**

```
  -h<header_parameters>
```

| Parameter                              | Description                                     |
| :------------------------------------- | :---------------------------------------------- |
|[en={Locale}](#param_en)                | ENcodes filename or comment as a locale.        |
|[de={Locale}](#param_de)                | DEcodes filename or comment as a locale.        |
| dc={Locale}                            | Decodes only Comment as a locale.               |
|[t=[WIN \| UNIX \| DOS \| 0]](#param_t) | Stores Time field of a type to headers.         |
|[tm={Timestamp}](#param_tm)             | Sets a Timestamp to last Modified time.         |
|[ta={Timestamp}](#param_tm)             | Sets a Timestamp to last Accessed time.         |
|[tc={Timestamp}](#param_tm)             | Sets a Timestamp to Creation time.              |
| ts={Timestamp}                         | Sets a TimeStamp to all times.                  |
| tz={[+\|-]hh:mm}                       | Adjusts Time Zone to all times.                 |
| ex=[off \| on]                         | Leaves all EXtra fields for files.              |
|[exa={HeaderID(s)}](#param_exa)         | Leaves EXtra field of header ID(s) for files.   |
|[exd={HeaderID(s)}](#param_exa)         | Removes EXtra field of header ID(s) from files. |

**en={Locale}** <a name="param_en"></a>

> Sets the locale for filename and comment written to file headers. The codeset must be specified though no language code is completed by system locale.

**de={Locale}** <a name="param_de"></a>

> Sets the locale for filename and comment read from file headers. The codeset must be specified though no language code is completed by system locale.

**t=[WIN | UNIX | DOS | 0]** <a name="param_t"></a>

> Sets the time format of file headers. NTFS or Unix time is added to extra field if `WIN` or `UNIX`. Otherwise, only DOS time format. Especially, that's cleared if `0` is specified.

**t{m|a|c}={Timestamp}** <a name="param_tm"></a>

> Sets a timestamp to the last modified, last accessed, or creation time of file headers. This value is `YYYYMMDDhhmm[.ss]` or `YYYY-MM-DDThh:mm:ss[.nnnnnnn]` but its seconds or nanoseconds is counted up or ignored for the time format. Moreover some time can't be stored, see below. 

Timestamp setting:

| Format    | Resolution | Present time fields             | 
| :-------- | :--------: | :------------------------------ |
| NTFS Time | 100 ns     | all times                       |
| Unix Time | 1 second   | last modified and accessed time |
| DOS Time  | 2 seconds  | only last modified time         |

**ex{a|d}={HeaderID(s)}** <a name="param_exa"></a>

> Sets the list of extra fields which are left or removed by `uh` command in the state of `ex=off` or `ex=on`. This value is hexadecimal preceded by `0x` or decimal number of header ID (which is printed by `l` command with `-slt` switch) and following ones are separated by a comma. This parameter only changes headers so that can't control ZIP64 and encryption fields. Besides the above parameters set NTFS or Unix time fields.

**Commands that can be used with this switch**

a (Add), rn (Rename), u (Update), [uh (Update Header)](#command_uh)

only `de` and `dc` parameters:

e (Extract), l (List), T (Test), x (Extract with full paths)


## License

This program is published under LGPL v2.1.

See `p7zip_16.02/DOC/License.txt` about the original p7zip license.

