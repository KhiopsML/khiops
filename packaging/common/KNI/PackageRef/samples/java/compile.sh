#!/bin/bash 
javac -cp jna.jar KNIRecodeFile.java KNI.java
jar cf kni.jar *.class
rm *.class
