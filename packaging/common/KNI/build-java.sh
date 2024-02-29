javac -cp jna.jar java/KNIRecodeFile.java java/KNI.java
jar cf kni.jar -C java KNI.class -C java KNIRecodeFile.class
