# NextLib

NextLib is an Android ARM64 listener for My Singing Monsters SFS2X responses. It hooks `libmonsters.so`, logs useful SmartFox traffic, and saves decoded payloads as JSON files for debugging server compatibility.
Made by Nextstars
## Files

- `NextLib.c` - cleaned source code.
- `libNextLib.so` - prebuilt ARM64 shared library.

## How It Must Be Loaded

`libNextLib.so` must load after `libmonsters.so`.

Recommended order:

```java
System.loadLibrary("monsters");
System.loadLibrary("NextLib");
```

If the loader cannot guarantee order, load `libNextLib.so` later and call:

```c
nextlib_logger_install();
```

The library also retries installation automatically from its constructor, but loading it after `libmonsters.so` is still the safest setup.

## Output

Logcat tag:

```text
NextLib_LOGGER
```

Main log files:

```text
/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files/nextlib_client.log
/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files/nextlib_summary.log
/sdcard/Download/nextlib_client.log
/sdcard/Download/nextlib_summary.log
```

Decoded SFS JSON payloads:

```text
/sdcard/Android/data/com.bigbluebubble.singingmonsters.full/files/msm_json/
```

## Logcat

```bash
adb logcat -c
adb logcat -s NextLib_LOGGER
```

PowerShell filter:

```powershell
adb logcat | findstr NextLib_LOGGER
```

## Build

Use Android NDK Clang for ARM64:

```bash
aarch64-linux-android21-clang -shared -fPIC -O2 -Wl,-soname,libNextLib.so -llog -o libNextLib.so NextLib.c
```

The included prebuilt `libNextLib.so` is already ARM64. Its internal SONAME may still show the old name, but the file name and exported install symbol are `libNextLib.so` and `nextlib_logger_install`.

## Notes

- This is a debugging listener, not a gameplay mod.
- Offsets are tied to the current supported `libmonsters.so` build.
- If the game updates, offsets may need to be refreshed.
