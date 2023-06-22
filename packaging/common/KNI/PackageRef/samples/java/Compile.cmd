@echo off
REM Home of java SDK; update according to local config
set JAVA_HOME=C:\Program Files\Java\jdk1.7.0_51\bin

REM compile Java files
"%JAVA_HOME%\javac.exe" -cp jna.jar KNIRecodeFile.java KNI.java

REM make jar file
"%JAVA_HOME%\jar.exe" cf kni.jar *.class

REM clean class files
del *.class
