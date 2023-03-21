# Animated AVIF Creator (Java)

This is an **_animated AVIF_** creator with a Java GUI and C++ AVIF encoder.
* Java GUI is modified from a prior animated GIF creator app, using sample GifSequenceWriter from memorynotfound's ['How to Generate GIF Image in Java with delay and infinite loop' example](https://memorynotfound.com/generate-gif-image-java-delay-infinite-loop-example/).
* The animated AVIF encoder is a JNI component, `libAvifHeic.dll`, built using the decoder/encoder from [JPEGView](https://github.com/sdneon/jpegview).
  * Only the encoding part is used for now.

## Usage
* Run the executable JAR, with all necessary DLLs in the same folder or easily available in system path.
  * If doesn't load, obtain the extra Visual Studio C++ redistributable DLLs from [JPEGView.zip/JPEGView/Extras folder](https://github.com/sdneon/jpegview/releases/download/v1.1.42.2/JPEGView.zip) - unzip to same folder.
* ALT+I or click "Choose images" to select a few images for your animation.
* ALT+O or click "Choose output filename:" to select a filename to save to.
* ALT+C or click "Create AVIF" to start generating the animated AVIF. It will take a LONG time, as the encoding process is very slow, in order to get a fantastically small output file.

**Options**
* Frame interval/delay: Once images are selected, you may change the individual frame delays below each image thumbnail. Or reset all delays usinkg the "Reset delays" button to the default value at the top.
* Loop: loop animation forever.
* Add reverse frames: enabling/ticking this will automatically add all images in reverse to the animation generation. These will not be shown in the thumbnails.

### AVIF JNI
This is a quick attempt to reuse the encoders/decoders from JPEGView. It is not optimized for performance.

* The DLLs are in `gui/lib`.
  * `libAvifHeic.dll` is the JNI bridge.
  *  `avif.dll` is the main one used to write animated AVIF.
  *  The rest are for reading AVIF.
* The Java declaration is `gui/sc/main/java/sd/gui/libAvifHeicJNI.java`.

#### Decode
```java
int[] AvifHeicDecode(byte[] src, long size, int frameIndex, int[] width, int[] height, int[] numFrames, long[] frameIntervalMs)
```
  * src: raw image file input
  * size: size of above in bytes
  * frameIndex: which frame to decode; base 0
    * make repeated calls to AvifHeicDecode with each frame index up to the total available in numFrames[0] to retrieve all animation frames
  * width: array with 1 slot must be provided for return value of image width (pixels)
  * height: array with 1 slot must be provided for return value of image height (pixels)
  * numFrames: array with 1 slot must be provided for return value of number of frames in image. Will be 1 if non-animated.
  * frameIntervalMs: array with 1 slot must be provided for return value of frame interval (ms) for this image frame
  * return value: decoded image output in BGRA int's

##### Example
```java
import java.nio.file.Files;
...

byte imgBytes[] = Files.readAllBytes(path);

int width[] = new int[1];
int height[] = new int[1];
int numFrames[] = new int[1];
long frameIntervalMs[] = new long[1];
int[] frame = libAvifHeicJNI.AvifHeicDecode(imgBytes, mem.size(), 0, width, height, numFrames, frameIntervalMs);
if (frame != null)
{
    int w = width[0], h = height[0],
        n = numFrames[0];
    BufferedImage bufImg = new BufferedImage(width[0], height[0], BufferedImage.TYPE_INT_ARGB);
    bufImg.setRGB(0, 0, w, h, frame, 0, w);
    if (n == 1)
    {
        return bufImg;
    }
    else
    {
        BufferedImage imgs[] = new BufferedImage[n];
        int intervalMs = (int)frameIntervalMs[0];
        int durations[] =  new int[n];
        imgs[0] = bufImg;
        durations[0] = intervalMs;
        for (int m = 1; m < n; ++m)
        {
            frame = libAvifHeicJNI.AvifHeicDecode(imgBytes, mem.size(), m, width, height, numFrames, frameIntervalMs);
            bufImg = new BufferedImage(width[0], height[0], BufferedImage.TYPE_INT_ARGB);
            bufImg.setRGB(0, 0, w, h, frame, 0, w);
            imgs[m] = bufImg;
            durations[m] = intervalMs;
        }
        bufImg = new AnimatedAvifImage(w, h, imgs, durations); //pass to your animated image class or equivalent
        return bufImg;
    }
}
```
#### Encode
```java
boolean AvifHeicWrite(java.io.OutputStream out, int[][] src, int width, int height, int numFrames, int quality, long[] frameIntervalMs)
```
  * out: java.io.OutputStream for writing encoded output to
  * src: raw BGRA image frames. For non-animated, just provide a single frame.
  * size: size of a single frame, i.e. the size of a frame's int[]
  * width: image width (pixels)
  * height: image height (pixels)
  * numFrames: number of frames in image. 1 if non-animated.
  * quality: image quality (up to 100).
  * frameIntervalMs: array of frame interval (ms) for all image frames.
    * May provide just 1 value for all frames.
    * Ignored for non-animated image, i.e. numFrames = 1.
  * return value: success or not

Look in `sd/gui/animatedavifcreator/AnimatedAvifCreator.java`'s' createAnimAvif() method for example usage.

## How to Build

### GUI
* Import `gui` folder in Netbeans IDE.
* Edit `pom.xml` if having problems with maven build, by setting `<maven.compiler.source>` & `<maven.compiler.target>` to your JDK version.
* Build

### JNI
* Build project in 'jni' using Visual Studio 2019.

## Minimum Requirements
* Win 7 64-bit
* Java JRE 8

## Wishlist
* Use the AVIF decoder to allow use of AVIF images as well in creating animated AVIF.

# Thanks
Thanks to the various parties for the codes and example, like memorynotfound, Alliance for Open Media, etc.