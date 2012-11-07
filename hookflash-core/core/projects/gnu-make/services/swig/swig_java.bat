rd java /s /q
pause
mkdir java
cd java
mkdir src
cd..
pause

swig -c++ -java -o java/hfservices_wrap.cpp -oh java/hfservices_wrap.h -outdir java/src -package hookflash hfservicesBase.i
pause

