:<<"::CMDGOTO"
@echo off
goto :CMDENTRY
rem https://stackoverflow.com/questions/17510688/single-script-to-run-in-both-windows-batch-and-linux-bash
::CMDGOTO

echo "========== rikiki build ${SHELL} ================="
DIR=$(dirname "$0")
(mkdir -p bin;)
(echo build .. $2)
(cd ${DIR}/bin;cc ../src/rikiki.c -o rkc; ./rkc ../src/spec.ri > t.c)
(cd ${DIR}/bin;./rkc ../src/spec.ri > t.c)
(cd ${DIR}/bin; cc -Wall -o a t.c; ./a)
(cd ${DIR}/bin;./rkc ../src/hello.ri > h.c)
(cd ${DIR}/bin; cc -Wall -o hello h.c; ./hello)
(cd ${DIR}/bin;./rkc ../src/vm303.ri > vm.c)
(cd ${DIR}/bin; cc -Wall -o vm vm.c; ./vm)
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

