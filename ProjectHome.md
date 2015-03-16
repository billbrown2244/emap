EMAP (Easy MIDI Audio Production) is a software front-end (GUI) for the [fluidsynth](http://fluidsynth.org) [soundfont](http://en.wikipedia.org/wiki/SoundFont)  synthesizer.  It is written in C++ with the [gtkmm](http://www.gtkmm.org/en/) and fluidsynth libraries.

The project uses [Maven](http://maven.apache.org/) as the build platform and can be configured to build form all platforms supported by fluidsynth and gtkmm.

TODO:  deploy nar artificats to maven central.

For now there is a binanry for the amd64 linux environment available from the downloads page. You'll need to "chmod +x emapXYZ" to make the file executable first.


![https://sites.google.com/a/colorfulsoftware.com/colorfulsoftware/emap/emap.png](https://sites.google.com/a/colorfulsoftware.com/colorfulsoftware/emap/emap.png)

![https://sites.google.com/a/colorfulsoftware.com/colorfulsoftware/emap/emaplv2.png](https://sites.google.com/a/colorfulsoftware.com/colorfulsoftware/emap/emaplv2.png)

Note:

The Java version of this project has been moved to a legacy branch because of issues making the code suitable for realtime with jack on linux.  If you are still interested in working with the java / scala version of the project please clone the repository and checkout the  java-scala branch.

Questions and comments are welcome.