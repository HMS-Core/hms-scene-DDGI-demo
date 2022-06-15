# DDGI Sample
English | [中文](README_ZH.md)



Firstly, compile shaders file by the cmd:
    python3 .\data\shaders\glsl\compileshaders.py

For Windows with Visual Studio：
step 1: create the build folder, and go into it;
step 2: execute the cmd:
    cmake ..
step 3: double click the "ddgi-sample.sln", and set the "DDGIExample" as startup project;
step 4: just run and enjoy.


There're 2 way for Android:

One way with Android Studio:
step 1: use Android Studio to open the "android" folder;
step 2: click "Sync Project with Gradle Files";
step 3: run "DDGIExample" module.

Another way with cmd:
step 1: go into the "android" folder, and execute the cmd:
    .\gradlew assembleRelease for release version, or .\gradlew assembleDebug for debug version;
step 2: the apk will be packaged into the "android\examples\bin" folder.
