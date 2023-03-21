#include "AVIFWrapper.h"
#include <string.h>
#include <new>

void* AVIFWrapper::ReadImage(int& width,
	int& height,
	int& nchannels,
	int& frame_count,
	bool& outOfMemory,
	int frame_index,
	const void* buffer,
	int sizebytes,
	int& frameSize,
	__int64& frameIntervalMs)
{
	outOfMemory = false;
	width = height = 0;
	frame_count = 0;
	nchannels = 4;

	avifDecoder* avifDecoder = 0;
	bool bHasAnimation = false;
	unsigned char* pPixelData = NULL;
	avifRGBImage rgb;
	try
	{
		memset(&rgb, 0, sizeof(rgb));
		// Override decoder defaults here (codecChoice, requestedSource, ignoreExif, ignoreXMP, etc)

		avifDecoder = avifDecoderCreate();
		avifDecoder->imageIndex = frame_index;

		avifResult result = avifDecoderSetIOMemory(avifDecoder, (const uint8_t*)buffer, sizebytes);
		if (result != AVIF_RESULT_OK) {
			goto cleanup; //won't happen
		}

		result = avifDecoderParse(avifDecoder);
		if (result != AVIF_RESULT_OK) {
			goto cleanup; //Failed to decode image
		}
		frame_count = avifDecoder->imageCount;

		// Now available:
		// * All decoder->image information other than pixel data:
		//   * width, height, depth
		//   * transformations (pasp, clap, irot, imir)
		//   * color profile (icc, CICP)
		//   * metadata (Exif, XMP)
		// * decoder->alphaPresent
		// * number of total images in the AVIF (decoder->imageCount)
		// * overall image sequence timing (including per-frame timing with avifDecoderNthImageTiming())

		if (avifDecoderNthImage(avifDecoder, frame_index) == AVIF_RESULT_OK)
		{
			// Now available (for this frame):
			// * All decoder->image YUV pixel data (yuvFormat, yuvPlanes, yuvRange, yuvChromaSamplePosition, yuvRowBytes)
			// * decoder->image alpha data (alphaPlane, alphaRowBytes)
			// * this frame's sequence timing

			avifRGBImageSetDefaults(&rgb, avifDecoder->image); //internally chooses RGBA
			// Override YUV(A)->RGB(A) defaults here:
			//   depth, format, chromaUpsampling, avoidLibYUV, ignoreAlpha, alphaPremultiplied, etc.
			rgb.format = AVIF_RGB_FORMAT_BGRA; //CJPEGImage desires BGRA, so change it
			if (rgb.depth > 8)
			{
				//CJPEGImage only supports 8bpp? so force downgrade to 8pp if original of higher bpp
				rgb.depth = 8;
			}

			// Alternative: set rgb.pixels and rgb.rowBytes yourself, which should match your chosen rgb.format
			// Be sure to use uint16_t* instead of uint8_t* for rgb.pixels/rgb.rowBytes if (rgb.depth > 8)
			// Use new(std::nothrow) unsigned char[] as per JPEGView internals (so it can cleanup itself), instead of avifRGBImageAllocatePixels(&rgb)'s c-based malloc.
			if (rgb.pixels) {
				delete[] rgb.pixels;
				rgb.pixels = 0;
			}
			rgb.rowBytes = rgb.width * avifRGBImagePixelSize(&rgb);
			frameSize = rgb.rowBytes * rgb.height;
			rgb.pixels = new(std::nothrow) unsigned char[(size_t)frameSize];
			if (rgb.pixels)
			{
				if (avifImageYUVToRGB(avifDecoder->image, &rgb) != AVIF_RESULT_OK) {
					delete[] rgb.pixels;
					rgb.pixels = 0;
					goto cleanup; //Conversion from YUV failed
				}

				// Now available:
				// * RGB(A) pixel data (rgb.pixels, rgb.rowBytes)

				frameIntervalMs = (int)(avifDecoder->imageTiming.duration * 1000);
				width = avifDecoder->image->width;
				height = avifDecoder->image->height;
			}
			else
			{
				outOfMemory = true;
			}
		}
	}
	catch (...) {
		rgb.pixels = 0;
	}
cleanup:
	if (avifDecoder)
	{
		avifDecoderDestroy(avifDecoder);
		avifDecoder = 0;
	}
	return rgb.pixels;
}