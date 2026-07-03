# Real libX11 (X11R6.9, the final monolithic X.Org release) port -> static library

get_directory_property(_xlib6_opts COMPILE_OPTIONS)
list(REMOVE_ITEM _xlib6_opts /RTC1)
set_directory_properties(PROPERTIES COMPILE_OPTIONS "${_xlib6_opts}")

set(POSIX_CRT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../psxcrt)

set(XLIB6_INCLUDES
    ${REACTOS_SOURCE_DIR}/sdk/include/psx        # POSIX SDK headers (open/read/...)
    ${CMAKE_CURRENT_SOURCE_DIR}/include          # <X11/Xlib.h>, psx Xpoll.h/Xtrans.h
    ${CMAKE_CURRENT_SOURCE_DIR}/compat           # <strings.h> BSD shim
    ${CMAKE_CURRENT_SOURCE_DIR}/src              # "Xlibint.h" etc.
    ${POSIX_CRT_INCLUDE})

# X_LOCALE: use Xlib's own locale implementation (_Xsetlocale) -- psxcrt has no
# setlocale(); this is exactly what X_LOCALE was invented for.  Runtime data files
# live under /etc/X11 (shipped from this repo's etc/X11 to the CD).
set(XLIB6_DEFS WIN_NT STDC_HEADERS _POSIX_SOURCE _POSIX_ __STDC__ HAS_SNPRINTF
    X_LOCALE
    NO_XLOCALEDIR   # no $XLOCALEDIR env override (its setuid check needs seteuid)
    "XLOCALEDIR=\"/etc/X11/locale\""
    "XCMSDB=\"/etc/X11/Xcms.txt\""
    "ERRORDB=\"/etc/X11/XErrorDB\""
    "KEYSYMDB=\"/etc/X11/XKeysymDB\"")

# /WX- : vendor sources are warning-noisy.  /FI : CRT-before-X11 prelude (Status
# typedef clash) -- see compat/xpsxpre6.h.  /UWIN32 : the ReactOS build defines
# WIN32 globally, which flips X sources onto their winsock paths -- we are a
# POSIX process, so force the unix paths.
set(XLIB6_OPTS /WX- /FIxpsxpre6.h /UWIN32 /U_WINDOWS)

# ---- core Xlib: Imakefile SRCS1 + SRCS2 (ConnDis.c -> psxConnDis.c/Xtranspsx6.c;
#      XDMAUTH/K5/thread-stub sources omitted) --------------------------------------
set(XLIB6_SRCS1
    Context.c Depths.c ParseCmd.c Quarks.c AllCells.c AllPlanes.c AllowEv.c
    AutoRep.c Backgnd.c BdrWidth.c Bell.c Border.c ChAccCon.c ChClMode.c
    ChActPGb.c ChCmap.c ChGC.c ChKeyCon.c ChPntCon.c ChProp.c ChSaveSet.c
    ChWAttrs.c ChWindow.c ChkIfEv.c ChkMaskEv.c ChkWinEv.c ChkTypEv.c
    ChkTypWEv.c CirWin.c CirWinDn.c CirWinUp.c ClDisplay.c Clear.c ClearArea.c
    ConfWind.c ConvSel.c CopyArea.c CopyCmap.c CopyGC.c CopyPlane.c CrBFData.c
    CrCmap.c CrCursor.c CrGC.c CrGlCur.c CrPFBData.c CrPixmap.c CrWindow.c
    Cursor.c DefCursor.c DelProp.c DestSubs.c DestWind.c DisName.c DrArc.c
    DrArcs.c DrLine.c DrLines.c DrPoint.c DrPoints.c DrRect.c DrRects.c
    DrSegs.c ErrDes.c ErrHndlr.c EvToWire.c FetchName.c FillArc.c FillArcs.c
    FillPoly.c FillRct.c FillRcts.c Flush.c Font.c FontNames.c FontInfo.c
    FreeCmap.c FreeCols.c FreeCurs.c FreeEData.c FreeGC.c FreePix.c FSSaver.c
    GCMisc.c Geom.c GetAtomNm.c GetColor.c GetDflt.c GetFPath.c GetFProp.c
    GetGeom.c GetHColor.c GetHints.c GetIFocus.c GetImage.c GetKCnt.c
    GetMoEv.c GetPCnt.c GetPntMap.c GetProp.c GetSOwner.c GetSSaver.c
    GetStCmap.c GetWAttrs.c GrButton.c GrKey.c GrKeybd.c GrPointer.c
    GrServer.c Host.c IfEvent.c ImUtil.c ImText.c ImText16.c InitExt.c
    InsCmap.c IntAtom.c KeyBind.c KeysymStr.c KillCl.c LiHosts.c LiICmaps.c
    LiProps.c ListExt.c LoadFont.c LookupCol.c LowerWin.c)

set(XLIB6_SRCS2
    Macros.c MapRaised.c MapSubs.c MapWindow.c MaskEvent.c Misc.c ModMap.c
    MoveWin.c NextEvent.c OpenDis.c ParseCol.c ParseGeom.c PeekEvent.c
    PeekIfEv.c Pending.c PmapBgnd.c PmapBord.c PolyReg.c PolyTxt.c PolyTxt16.c
    PutBEvent.c PutImage.c QuBest.c QuColor.c QuColors.c QuCurShp.c QuExt.c
    QuKeybd.c QuPntr.c QuStipShp.c QuTextE16.c QuTextExt.c QuTileShp.c
    QuTree.c RaiseWin.c RdBitF.c RecolorC.c ReconfWin.c Region.c RepWindow.c
    RestackWs.c RotProp.c ScrResStr.c SelInput.c SendEvent.c SetBack.c
    SetCRects.c SetClMask.c SetClOrig.c SetDashes.c SetFPath.c SetFont.c
    SetFore.c SetFunc.c SetHints.c SetIFocus.c SetLStyle.c SetPMask.c
    SetPntMap.c SetSOwner.c SetSSaver.c SetState.c SetStip.c SetStCmap.c
    SetTile.c SetTSOrig.c StBytes.c StColor.c StColors.c StNColor.c StName.c
    StrKeysym.c Sync.c Synchro.c Text.c Text16.c TextExt.c TextExt16.c
    TrCoords.c UndefCurs.c UngrabBut.c UngrabKbd.c UngrabKey.c UngrabPtr.c
    UngrabSvr.c UninsCmap.c UnldFont.c UnmapSubs.c UnmapWin.c VisUtil.c
    WarpPtr.c WinEvent.c Window.c WrBitF.c XlibAsync.c XlibInt.c Xrm.c
    evtomask.c globals.c GetGCVals.c GetNrmHint.c GetRGBCMap.c GetTxtProp.c
    GetWMCMapW.c GetWMProto.c Iconify.c PixFormats.c PropAlloc.c ReconfWM.c
    SetNrmHint.c SetRGBCMap.c SetTxtProp.c SetWMCMapW.c SetWMProto.c
    StrToText.c TextToStr.c Withdraw.c WMGeom.c WMProps.c)

set(XLIB6_PSX_SRCS
    psxConnDis.c Xtranspsx6.c)

# ---- Imakefile SRCS3: Xcms + i18n wrappers + locking (XKBSRCS off for now;
#      x11trans.c/ximtrans.c replaced by Xtranspsx6.c) -------------------------------
set(XLIB6_SRCS3
    Lab.c LabGcC.c LabGcL.c LabGcLC.c LabMnL.c LabMxC.c LabMxL.c LabMxLC.c
    LabWpAj.c Luv.c LuvGcC.c LuvGcL.c LuvGcLC.c LuvMnL.c LuvMxC.c LuvMxL.c
    LuvMxLC.c LuvWpAj.c XYZ.c uvY.c xyY.c HVC.c HVCGcC.c HVCGcV.c HVCGcVC.c
    HVCMnV.c HVCMxC.c HVCMxV.c HVCMxVC.c HVCMxVs.c HVCWpAj.c UNDEFINED.c
    AddDIC.c AddSF.c cmsAllNCol.c cmsAllCol.c CCC.c cmsCmap.c cmsColNm.c
    CvColW.c CvCols.c cmsGlobls.c IdOfPr.c cmsInt.c LRGB.c cmsLkCol.c
    cmsMath.c OfCCC.c PrOfId.c cmsProp.c QBlack.c QBlue.c QGreen.c QRed.c
    QWhite.c QuCol.c QuCols.c SetCCC.c SetGetCols.c StCol.c StCols.c
    cmsTrig.c XRGB.c FSWrap.c ICWrap.c IMWrap.c OCWrap.c OMWrap.c lcWrap.c
    mbWMProps.c utf8WMProps.c mbWrap.c wcWrap.c utf8Wrap.c FilterEv.c
    RegstFlt.c locking.c LockDis.c)

# ---- Imakefile LCSRCS + STATICI18NSTATICSRCS: the locale engine and the
#      statically-linked input/output-method modules ---------------------------------
set(XLIB6_LCSRCS
    SetLocale.c imConv.c imKStoUCS.c lcCT.c lcCharSet.c lcConv.c lcDB.c
    lcDefConv.c lcFile.c lcGeneric.c lcInit.c lcPrTxt.c lcPubWrap.c
    lcPublic.c lcRM.c lcStd.c lcTxtPr.c lcUtil.c lcUTF8.c
    imCallbk.c imDefFlt.c imDefIc.c imDefIm.c imDefLkup.c imDispch.c
    imEvToWire.c imExten.c imImSw.c imInsClbk.c imInt.c imLcFlt.c imLcGIc.c
    imLcIc.c imLcIm.c imLcLkup.c imLcPrs.c imLcSIc.c imRm.c imRmAttr.c
    imThaiFlt.c imThaiIc.c imThaiIm.c imTrX.c imTransR.c imTrans.c
    lcEuc.c lcGenConv.c lcJis.c lcSjis.c lcUTF8Load.c
    omDefault.c omGeneric.c omImText.c omText.c omTextEsc.c omTextExt.c
    omTextPer.c omXChar.c)

list(TRANSFORM XLIB6_SRCS1 PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)
list(TRANSFORM XLIB6_SRCS2 PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)
list(TRANSFORM XLIB6_SRCS3 PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)
list(TRANSFORM XLIB6_LCSRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)
list(TRANSFORM XLIB6_PSX_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_library(libX11r6 ${XLIB6_SRCS1} ${XLIB6_SRCS2} ${XLIB6_SRCS3}
    ${XLIB6_LCSRCS} ${XLIB6_PSX_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/xpsxcompat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/xpsxlibc.c)
target_compile_definitions(libX11r6 PRIVATE ${XLIB6_DEFS})
target_compile_options(libX11r6 PRIVATE ${XLIB6_OPTS})
target_include_directories(libX11r6 BEFORE PRIVATE ${XLIB6_INCLUDES})

# ---- libXext (X11R6.9 lib/Xext, minus XShm -- no SysV SHM on the subsystem) --------
# Extension client stubs: SHAPE, SYNC, DBE, MultiBuf, Security, AppGroup, DPMS, ...
# Pure protocol code over Xlib; the server may honestly report "extension absent".
set(XEXT6_SRCS
    globals.c extutil.c XMultibuf.c XShape.c MITMisc.c XTestExt1.c XSync.c
    Xdbe.c XLbx.c XSecurity.c XAppgroup.c Xcup.c DPMS.c XEVI.c)
list(TRANSFORM XEXT6_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/xext/)

add_library(libXext6 ${XEXT6_SRCS})
target_compile_definitions(libXext6 PRIVATE ${XLIB6_DEFS})
target_compile_options(libXext6 PRIVATE ${XLIB6_OPTS})
target_include_directories(libXext6 BEFORE PRIVATE ${XLIB6_INCLUDES})

# ---- libICE (X11R6.9 lib/ICE) ------------------------------------------------------
# Inter-Client Exchange.  No SM server exists on the subsystem, so the psx ICE
# transport (ice/Xtranspsxice.c) fails to open/listen gracefully -- clients run
# session-less, same as unix with SESSION_MANAGER unset.  libXt links this.
set(ICE6_SRCS
    accept.c authutil.c connect.c error.c getauth.c iceauth.c listen.c
    listenwk.c locking.c misc.c ping.c process.c protosetup.c register.c
    replywait.c setauth.c shutdown.c watch.c Xtranspsxice.c)
list(TRANSFORM ICE6_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/ice/)

add_library(libICE6 ${ICE6_SRCS})
target_compile_definitions(libICE6 PRIVATE ${XLIB6_DEFS})
target_compile_options(libICE6 PRIVATE ${XLIB6_OPTS})
target_include_directories(libICE6 BEFORE PRIVATE ${XLIB6_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ice)

# ---- libSM (X11R6.9 lib/SM) --------------------------------------------------------
# Session Management over ICE; SmcOpenConnection fails cleanly (no SM server).
set(SM6_SRCS
    sm_auth.c sm_client.c sm_error.c sm_genid.c sm_manager.c sm_misc.c
    sm_process.c)
list(TRANSFORM SM6_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/sm/)

add_library(libSM6 ${SM6_SRCS})
target_compile_definitions(libSM6 PRIVATE ${XLIB6_DEFS})
target_compile_options(libSM6 PRIVATE ${XLIB6_OPTS})
target_include_directories(libSM6 BEFORE PRIVATE ${XLIB6_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/sm)

# ---- libXt (X11R6.9 lib/Xt -- the Intrinsics, the make-or-break layer) -------------
# StringDefs.c/StringDefs.h/Shell.h are generated from util/string.list with
# makestrs (host clang; recipe in README.md) and checked in under xt/ + include/X11.
# sharedlib.c omitted (static build).
set(XT6_SRCS
    ActionHook.c Alloc.c ArgList.c Callback.c ClickTime.c Composite.c
    Constraint.c Convert.c Converters.c Core.c Create.c Destroy.c Display.c
    Error.c Event.c EventUtil.c Functions.c GCManager.c Geometry.c
    GetActKey.c GetResList.c GetValues.c HookObj.c Hooks.c Initialize.c
    Intrinsic.c Keyboard.c Manage.c NextEvent.c Object.c PassivGrab.c
    Pointer.c Popup.c PopupCB.c RectObj.c ResConfig.c Resources.c
    Selection.c SetSens.c SetValues.c SetWMCW.c Shell.c StringDefs.c
    Threads.c TMaction.c TMgrab.c TMkey.c TMparse.c TMprint.c TMstate.c
    Varargs.c VarCreate.c VarGet.c Vendor.c)
list(TRANSFORM XT6_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/xt/)

add_library(libXt6 ${XT6_SRCS})
target_compile_definitions(libXt6 PRIVATE ${XLIB6_DEFS})
target_compile_options(libXt6 PRIVATE ${XLIB6_OPTS})
target_include_directories(libXt6 BEFORE PRIVATE ${XLIB6_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/xt)

# ---- libXmu (mini): just what the first Xt clients need ----------------------------
# Grown file-by-file as clients demand (full lib/Xmu is on disk in x11r6src).
set(XMU6_SRCS StrToBS.c Lower.c)
list(TRANSFORM XMU6_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/xmu/)

add_library(libXmu6 ${XMU6_SRCS})
target_compile_definitions(libXmu6 PRIVATE ${XLIB6_DEFS})
target_compile_options(libXmu6 PRIVATE ${XLIB6_OPTS})
target_include_directories(libXmu6 BEFORE PRIVATE ${XLIB6_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/xmu)

# ---- libXm (OpenMotif 2.2.2 / 2.1.x ABI -- lib/Xm, the CDE widget set) --------------
# Phase 5 of nextgoals.md.  Staged under motif/Xm/ (all vendor lib/Xm .c + .h, flat).
# Include convention: <Xm/Foo.h> (public) resolves via the motif/ parent dir; "Foo.h"
# and <Xm.h>/<local_ctype.h> via motif/Xm + the bundled wchar extras.  XmStrDefs.h/.c/
# I.h are generated on the host with Motif's own makestrs (config/util/makestrs.c;
# recipe in README.md) from xmstring.list + the .ct/.ht templates -- default ABI.
# NB: the real Motif dirs MUST precede XLIB6_INCLUDES -- xlib6/include/Xm/ holds a
# dthello-era *placeholder* Xm.h (MwmUtil only); the real <Xm/Xm.h> has to win here.
set(XM2_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/motif                 # <Xm/Foo.h>
    ${CMAKE_CURRENT_SOURCE_DIR}/motif/Xm              # "Foo.h", <Xm.h>
    ${CMAKE_CURRENT_SOURCE_DIR}/../motif/extras/wchar/h  # <local_ctype.h> (vendor Motif tree)
    ${CMAKE_CURRENT_SOURCE_DIR}/regex                 # <regex.h> (Xmos.c FileSB)
    ${XLIB6_INCLUDES})

file(GLOB XM2_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/motif/Xm/*.c)
# PrintS.c (XmPrintShell) calls the Xp client library -- CDE has no printing.
list(FILTER XM2_SRCS EXCLUDE REGEX "/PrintS\\.c$")

# Henry Spencer POSIX regex (X.Org extras/regex) -- Xmos.c uses regcomp/regexec
# for FileSelectionBox pattern filtering (engine.c is #included by regexec.c).
set(REGEX_SRCS regcomp.c regerror.c regexec.c regfree.c)
list(TRANSFORM REGEX_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/regex/)

add_library(libXm2 ${XM2_SRCS} ${REGEX_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/xpsxmotif.c)
target_compile_definitions(libXm2 PRIVATE ${XLIB6_DEFS})
target_compile_options(libXm2 PRIVATE ${XLIB6_OPTS})
target_include_directories(libXm2 BEFORE PRIVATE ${XM2_INCLUDES})

# ---- hellomotif: the FIRST MOTIF CLIENT (BulletinBoard + Label + PushButton) -------
# Hand-coded (no UIL/Mrm): XtVaAppInitialize + XmBulletinBoard + XmLabel +
# XmPushButton + activate callback -> exercises Manager/Composite/Constraint,
# Primitive/Gadget, XmString, resource conversion and fallback resources.
add_executable(posix_hellomotif ${CMAKE_CURRENT_SOURCE_DIR}/motif/clients/hellomotif/hellomotif.c)
set_target_properties(posix_hellomotif PROPERTIES OUTPUT_NAME hellomotif)
target_compile_definitions(posix_hellomotif PRIVATE ${XLIB6_DEFS})
target_compile_options(posix_hellomotif PRIVATE ${XLIB6_OPTS})
target_include_directories(posix_hellomotif BEFORE PRIVATE ${XM2_INCLUDES})
set_module_type(posix_hellomotif module)
set_subsystem(posix_hellomotif posix)
set_entrypoint(posix_hellomotif __PosixProcessStartup)
target_link_libraries(posix_hellomotif libXm2 libXt6 libSM6 libICE6 libXext6 libX11r6 psxcrt chkstk)
add_importlibs(posix_hellomotif psxdll)
add_cd_file(TARGET posix_hellomotif DESTINATION reactos/bin FOR all)

# ---- xeyes: the first Xt CLIENT (Intrinsics end-to-end; SHAPE off) -----------------
set(XEYES_SRCS xeyes.c Eyes.c transform.c)
list(TRANSFORM XEYES_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/demos/xeyes/)

add_executable(posix_xeyes ${XEYES_SRCS})
set_target_properties(posix_xeyes PROPERTIES OUTPUT_NAME xeyes)
target_compile_definitions(posix_xeyes PRIVATE ${XLIB6_DEFS})
target_compile_options(posix_xeyes PRIVATE ${XLIB6_OPTS})
target_include_directories(posix_xeyes BEFORE PRIVATE ${XLIB6_INCLUDES})
set_module_type(posix_xeyes module)
set_subsystem(posix_xeyes posix)
set_entrypoint(posix_xeyes __PosixProcessStartup)
target_link_libraries(posix_xeyes libXmu6 libXt6 libSM6 libICE6 libXext6 libX11r6 psxcrt chkstk)
add_importlibs(posix_xeyes psxdll)
add_cd_file(TARGET posix_xeyes DESTINATION reactos/bin FOR all)

# ---- dthello: the FIRST CDE PROGRAM (login splash; Xt-only, no Motif linked) ------
# One vendor file + three DtSvc objects (per the CDE Imakefile: EnvControl,
# DtNlUtils, GetDispRes).  <Xm/*.h> satisfied by psx-minimal headers in include/Xm
# (only the _MOTIF_WM_HINTS wire protocol is used).
set(DTHELLO_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/cde/dthello/dthello.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cde/dtsvc/EnvControl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cde/dtsvc/DtNlUtils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cde/dtsvc/GetDispRes.c)

add_executable(posix_dthello ${DTHELLO_SRCS})
set_target_properties(posix_dthello PROPERTIES OUTPUT_NAME dthello)
target_compile_definitions(posix_dthello PRIVATE ${XLIB6_DEFS}
    "CDE_INSTALLATION_TOP=\"/usr/dt\""      # imake-provided CDE roots
    "CDE_CONFIGURATION_TOP=\"/etc/dt\"")
target_compile_options(posix_dthello PRIVATE ${XLIB6_OPTS})
target_include_directories(posix_dthello BEFORE PRIVATE ${XLIB6_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/cde/dtsvc)
set_module_type(posix_dthello module)
set_subsystem(posix_dthello posix)
set_entrypoint(posix_dthello __PosixProcessStartup)
target_link_libraries(posix_dthello libXt6 libSM6 libICE6 libXext6 libX11r6 psxcrt chkstk)
add_importlibs(posix_dthello psxdll)
add_cd_file(TARGET posix_dthello DESTINATION reactos/bin FOR all)

# ---- ico demo (bare Xlib spinning polyhedron) -- the R6 link test ------------------
add_executable(posix_ico6 ${CMAKE_CURRENT_SOURCE_DIR}/demos/ico/ico.c)
set_target_properties(posix_ico6 PROPERTIES OUTPUT_NAME ico)
target_compile_definitions(posix_ico6 PRIVATE ${XLIB6_DEFS})
target_compile_options(posix_ico6 PRIVATE ${XLIB6_OPTS})
target_include_directories(posix_ico6 BEFORE PRIVATE ${XLIB6_INCLUDES})
set_module_type(posix_ico6 module)
set_subsystem(posix_ico6 posix)
set_entrypoint(posix_ico6 __PosixProcessStartup)
target_link_libraries(posix_ico6 libX11r6 psxcrt chkstk)
add_importlibs(posix_ico6 psxdll)
add_cd_file(TARGET posix_ico6 DESTINATION reactos/bin FOR all)

