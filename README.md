# 7-Segment-Digital-Display (fork)

This is a new version of a 7 segment digital display based on a 7 segment display that uses WS2812B LED's.  It no longer functions as a clock or anything else (though this may change in the future).  For now it's only to display a number which can be changed via the web interface.<br />
It uses the 7 segment display web code from the project this one was forked from, but also merged with code from [Unexpected Maker's 7 segment project](https://github.com/UnexpectedMaker/Neo7Segment).

## License

<a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-sa/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/">Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License</a>.


## Components

| Components                    | x     |
| -------------                 | ----- |
|ESP8266 WeMos Mini D1          | 1x    |
|Micro USB Breakout board       | 1x    |
|Micro USB cable                | 1x    |
|5V 2.5A power supply           | 1x    |
|WS2812B LED Strip 60 LED's per meter     | 2x    |

## 3D Printing Design Files

It is recommended that the 3D printed files use 100% infill an 0.2mm layer height. (These files are a work in progress)

The pixel svg is ideally laser cut with 6447 light diffusing acrylic.  If not, 3D print it using a white or translucent filament.

## Schema

Connect D6 from the ESP8266 through a 10 ohm resistor to the data pin of the LED strip. Don't power the LED strip from the ESP board but use a Micro USB breakout board en connect the 5V to the LED strip.

The pixels are as follows:
 1
6 2
 7
5 3
 4

D6 should go to the D-IN of pixel 1. The arrows should go clockwise, and the middle pixel (7) is oriented from left to right. 

## Arduino code

Upload the .ino file to your ESP board. You also need to upload the html and javascript files with the ESP8266FS tool. Check this link for more info:

http://arduino.esp8266.com/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system

Before uploading the files to your ESP board you have to gzip them with the command:

`gzip -r ./data/`

Afterwards if you want to change something to the html files, just unzip with:

`gunzip -r ./data/`









