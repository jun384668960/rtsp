@echo off
del /s/q bin  
del /s/q CodeDB
del /s/q debug
del /s/q gen
del /s/q obj
del /s/q VisualGDBCache
del /s/q build.xml
del /s/q proguard-project.txt
del /s/q streamer.sdf
del /s/q streamer.v*.suo
del /s/q streamer.vcxproj.user
del /s/q make*.bat
del /s/q .d
del /s/q libs\armeabi\gdb*
del /s/q libs\armeabi-v7a\gdb*

rd  /s/q bin
rd  /s/q CodeDB
rd  /s/q debug
rd  /s/q gen
rd  /s/q obj
rd  /s/q VisualGDBCache