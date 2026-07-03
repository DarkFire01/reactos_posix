# X11R6.9 client stack for the ReactOS POSIX subsystem

This is the **Phase-1 "R6 pivot"** from `nextgoals.md`: replacing the 1987 X11R1
Xlib (`../xlib`) with the real **X11R6.9** client libraries — the stack CDE
(X11R6.4 + Motif 2.1) actually targets.  Source: the monolithic X.Org tree at
`x11r6src/` (X11R6.9, Dec 2005, the last monolithic release; its libX11 is
pre-XCB, exactly the pin recommended in the earlier research).

## What builds today (all in the MSVC userland dir, `output-VS-i386`)

| Target | Contents |
|---|---|
| `libX11r6` | Full R6.9 libX11: core protocol (Imakefile SRCS1+SRCS2), **Xcms** color mgmt, the **i18n layer** (lcWrap/ICWrap/IMWrap/OMWrap + the whole locale engine LCSRCS + static input/output-method modules im*/om*/lc*), `ks_tables.h` generated (XStringToKeysym works). **XKB is OFF** (`-DXKB` not defined; XKBSRCS not compiled). |
| `libXext6` | R6.9 lib/Xext minus XShm (no SysV SHM): SHAPE, SYNC, DBE, MultiBuf, Security, AppGroup, Cup, DPMS, EVI, LBX, MITMisc, XTestExt1. |
| `posix_ico6` → `ico.exe` | bare-Xlib demo linked against libX11r6 (link/runtime test). |
| `posix_9wm6` → `9wm.exe` | the window manager, now against libX11r6 (sources shared with `../xlib/clients/9wm`). |

Build: `ninja -C <VS-build-dir> libX11r6 libXext6 posix_ico6 posix_9wm6`

## How the port works (the seams)

Everything vendor stays vendor; the OS/transport surface is concentrated in
files we own:

- **`src/Xtranspsx6.c`** — the entire X Transport Interface over the
  `/dev/x11` bridge (open + blocking read/write via the real psxdll; no
  select, no sockets).  Implements `_X11TransConnectDisplay` (DISPLAY parse →
  `open("/dev/x11")`, no auth) plus the byte-level `_X11Trans*` set XlibInt.c
  calls, the `_XimXTrans*` set (remote-IM: opening always fails → XOpenIM
  falls back to the local input method), and `_XPsxSelect` (zero timeout →
  "not readable" so the XCONN liveness probe never blocks; else "ready" and
  the blocking read waits).
- **`src/psxConnDis.c`** — replaces vendor `ConnDis.c` (socket/auth engine):
  keeps `_XConnectDisplay`, `_XDisconnectDisplay`, `_XSendClientPrefix`,
  `XSetAuthorization`.
- **`include/X11/Xtrans/Xtrans.h`** — psx-minimal Xtrans header (opaque
  `XtransConnInfo`, `struct iovec`, `BytesReadable_t`, TRANS_* codes).
- **`include/X11/Xpoll.h`** — psx fd_set/Select macros over `_XPsxSelect`.
- **`include/X11/XlibConf.h`** — no XTHREADS.
- **`compat/xpsxpre6.h`** — /FI prelude: CRT before X11 headers (the
  `Status`-typedef-poisons-SDK-prototypes fix), `<stddef.h>` for `wchar_t`
  (added to psxcrt STDDEF.H), EWOULDBLOCK.
- **`compat/xpsxcompat.c` + `xpsxlibc.c`** — copied from the R1 port
  (bzero/bcmp, ffs, random, Taylor sin/cos, `_ftol`).

Key build decisions:
- **`/UWIN32 /U_WINDOWS`** — ReactOS defines WIN32 globally, which flips X
  sources onto their winsock paths; we are a POSIX process and must take the
  unix paths.
- **`-DX_LOCALE`** — Xlib's own `_Xsetlocale` (psxcrt has no setlocale; this
  is what X_LOCALE exists for).  `-DNO_XLOCALEDIR` (the env override's setuid
  check wants seteuid).  Locale/XErrorDB/XKeysymDB data paths point at
  `/etc/X11/...`; ship the `x11r6src/nls` database there when i18n is needed
  for real (everything falls back gracefully meanwhile).
- **`ks_tables.h`** — generated from `include/X11/keysymdef.h` with
  `lib/X11/util/makekeys.c` compiled natively by clang (recipe:
  `clang -w -D_XOS_H_ -include string.h -I include makekeys.c`, feed
  keysymdef.h on stdin).
- Clients link `chkstk` (ReactOS's `__chkstk`) — big vendor locals hit MSVC
  stack probes.

## The Xt stack (Phase 3+4, built 2026-07-01)

- **`libICE6` / `libSM6`** — vendor code intact; `ice/Xtranspsxice.c` makes every
  ICE open/listen fail gracefully (no SM server on the subsystem = unset
  SESSION_MANAGER behavior, which all Xt clients tolerate).
- **`libXt6`** — all 54 Intrinsics files.  `StringDefs.c/h` + `Shell.h` generated
  with `makestrs` (host clang, `-D_XOS_H_ -Dindex=strchr -include string.h` +
  a stub `unistd.h`; run with `-f util/string.list` from a dir containing
  `util/`).  Compiled first-try.
- **`libXmu6`** — mini: `StrToBS.c` + `Lower.c`, grown on demand.
- **`posix_xeyes`** → `xeyes.exe` — the first Xt client.
- unix-isms Xt needed, in `compat/xpsxcompat.c`: `gettimeofday` (via `times()`,
  whose return value the MS psxdll computes client-side at CLK_TCK=100 → a
  monotonic 10ms clock, exactly right for Xt timer deltas), `gethostname` (via
  `uname`), `strcoll` (bytewise), `vsnprintf` (via psxcrt `vsprintf`).
  `compat/math.h` + `xpsxlibc.c`: `sqrt`/`hypot`/`atan`/`atan2`/`floor`/
  `ceil`/`fmod` added for Eyes geometry.

### The event-loop problem and the fcntl poll tunnel

Xt's `_XtWaitForSomething` needs a real "is the display readable?" primitive —
with the old `BytesReadable=0` + always-ready `Select`, an Xt app would busy-spin
and never actually read events (Xt never calls blocking `XNextEvent`).  The
subsystem primitive exists (`PSX_API_POLL`, psxss xconn.c) but the **real MS
psxdll.dll** on the image can't send new opcodes.  Solution: **tunnel through
`fcntl`** (op 0x2D), which MS psxdll marshals generically:

    fcntl(fd, PSX_FCNTL_POLLRD /* 0x70 */, timeout_ms) -> 1 readable / 0 timeout

Server side: `PsxSrvFcntl` case in psxss/fd.c → `PsxPollWait` (shared with
`PsxSrvPoll`).  Client side: `_X11TransBytesReadable` (probe, reports 32 when
data waits) and `_XPsxSelect` (waits with the caller's timeout on the display
fd, tracked in `_XPsxDisplayFd`).  RISK to verify on image: whether the MS
psxdll client-side fcntl forwards unknown cmds (expected — its dup() is just
fcntl(fd,0,0) and only F_GETLK/SETLK get special packing).  If it EINVALs
locally, psxss will show no fcntl trace and we need a different carrier op.

## Not done / next

- **Runtime validation on the image** (ico + 9wm through psxx11) — the wire
  protocol is the same X11 the R1 port spoke, but R6's OpenDis/XlibInt paths
  are new; watch for requests psxx11 doesn't answer yet (QueryExtension is
  honest-absent, GetKeyboardMapping, etc.).
- **XKB group** (XKBSRCS + `-DXKB`) once psxx11 can fake the XKB extension —
  not needed for Xt/Motif bring-up.
- **Phase 3: libICE + libSM** (`x11r6src/lib/ICE`, `lib/SM`) — need an Xtrans
  ICE_t seam or graceful-fail stubs (Xt clients tolerate no session manager).
- **Phase 4: libXt** (`lib/Xt`) — the make-or-break layer; after it,
  `dthello` (CDE's Xt-only splash) becomes buildable.  libXmu/libXmuu come
  alongside (`Xmuu` = the Xt-free subset first).
- **Phase 5: Motif 2.1**, then CDE's own libs (DtSvc → DtWidget/DtTerm/...).
