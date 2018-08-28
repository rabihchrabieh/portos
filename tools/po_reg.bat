@echo off

set mygrep=%SystemRoot%\system32\find

REM reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Texas Instruments" | %mygrep% "CCS_" | po_cmd reg query /qin | %mygrep% "%1" | po_cmd reg query /qin | %mygrep% "Build Tools" | po_cmd reg query /qin |%mygrep% "CGT_"

reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Texas Instruments" /s | %mygrep% "CCS_" | %mygrep% "%1" | %mygrep% "Build Tools" | po_cmd reg query /qin |%mygrep% "CGT_"
