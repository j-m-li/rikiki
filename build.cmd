:<<"::CMDGOTO"
@echo off
goto :CMDENTRY
rem https://stackoverflow.com/questions/17510688/single-script-to-run-in-both-windows-batch-and-linux-bash
::CMDGOTO

echo "========== rikiki build ${SHELL} ================="
DIR=$(dirname "$0")
(mkdir -p bin;)
(echo build .. $2)
(cd ${DIR}/bin; node ../src/spec.mjs)
(cd ${DIR}/bin; node ../src/hello.mjs)
exit
(cd ${DIR}/bin;./rkc ../src/quine.ri > q.c)
(cd ${DIR}/bin; cc -Wall -o quine q.c; ./quine)
(cd ${DIR}/bin;./rkc ../src/asm303.ri > as.c)
(cd ${DIR}/bin; cc -Wall -o as3 as.c; ./as3 -o test.raw ../src/test.asm)
(cd ${DIR}/bin;./rkc ../src/vm303.ri > vm.c)
(cd ${DIR}/bin; cc -Wall -o vm vm.c; ./vm test.raw)
exit $?
:CMDENTRY

echo ============= rikiki build %COMSPEC% ============
set OLDDIR=%CD%
mkdir bin  >nul 2>&1
cd bin
echo build %1 %2
cl %~dp0\src\rikiki.c /D_CRT_SECURE_NO_WARNINGS=1 /Fe:rkc.exe >>build.log 2>&1
clang -D_CRT_SECURE_NO_WARNINGS=1 %~dp0\src\rikiki.c -o rkc.exe >>build.log 2>&1
chdir /d %OLDDIR%
exit 0

