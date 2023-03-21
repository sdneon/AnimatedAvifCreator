#include "SaveImage.h"
#include <string.h>

CAvifEncoder::CAvifEncoder():
	m_pEnv(0),
	m_outStream(0),
	m_metdWrite(0),
	m_pEncoder(0),
	m_pAvifOuput(0),
	m_nWidth(0), m_nHeight(0),
	m_nTimeScaleHz(1), m_nFrameIntervalMs(1000),
	m_nQuality(60),
	m_bLossless(false),
	m_bSuccess(false)
{
}

CAvifEncoder::~CAvifEncoder()
{
	if (m_pEncoder)
	{
		avifEncoderDestroy(m_pEncoder);
		m_pEncoder = 0;
	}
}

/*
* Initialize AVIF image encoder by creating file and setting various image properties
*/
bool CAvifEncoder::Init(JNIEnv* pEnv, jobject outStream, jmethodID metdWrite, int nWidth, int nHeight, bool bUseLossless, int nQuality, uint64_t nFrameIntervalMs)
{
	if (m_pEncoder || m_pAvifOuput)
	{
		return false; //cannot init twice! abort
	}
	m_pEnv = pEnv;
	m_outStream = outStream;
	m_metdWrite = metdWrite;

	memset(&m_rgb, 0, sizeof(m_rgb));
	m_nWidth = nWidth;
	m_nHeight = nHeight;

	m_pEncoder = avifEncoderCreate();
	// Configure your encoder here (see avif/avif.h):
	// * maxThreads
	// * quality
	// * qualityAlpha
	// * tileRowsLog2
	// * tileColsLog2
	// * speed
	// * keyframeInterval
	// * timescale
	m_pEncoder->quality = m_nQuality = nQuality;
	m_bLossless = bUseLossless;
	m_pEncoder->qualityAlpha = bUseLossless ? AVIF_QUALITY_LOSSLESS : AVIF_QUALITY_DEFAULT;
	m_nFrameIntervalMs = nFrameIntervalMs;
	m_nTimeScaleHz = (uint64_t)(1.0 / (0.001 * nFrameIntervalMs));
	m_pEncoder->timescale = m_nTimeScaleHz;

	return true;
}

/*
* Write single image of non-animated AVIF
*/
bool CAvifEncoder::WriteSingleImage(void* pData, bool bUseLossless, int nQuality)
{
	return WriteImage(pData, bUseLossless, nQuality);
}

/*
* Append an image frame for an animated AVIF
*/
bool CAvifEncoder::AppendImage(void* pData, bool bUseLossless, int nQuality,
	uint64_t nFrameIntervalMs, bool bKeyFrame)
{
	return WriteImage(pData, bUseLossless, nQuality, nFrameIntervalMs,
		bKeyFrame? AVIF_ADD_IMAGE_FLAG_FORCE_KEYFRAME: AVIF_ADD_IMAGE_FLAG_NONE);
}

/*
* [Internal use] Write an image frame of AVIF
* nFrameIntervalMs: duration of this frame in ms; applies to animated AVIF only
* nFrameType: AVIF_ADD_IMAGE_FLAG_SINGLE = single image of a non-animated AVIF
*			  AVIF_ADD_IMAGE_FLAG_FORCE_KEYFRAME = keyframe of an animated AVIF
*			  AVIF_ADD_IMAGE_FLAG_NONE = subsequent frame of an animated AVIF
*/
bool CAvifEncoder::WriteImage(void* pData, bool bUseLossless, int nQuality,
	uint64_t nFrameIntervalMs, avifAddImageFlag nFrameType)
{
	if (!m_pEncoder || !m_pEnv || !m_outStream
		|| m_pAvifOuput) //already finished output, so no more writes allowed
	{
		return false;
	}
	bool bOk = false;
	avifImage* image = 0;
	try {
		image = avifImageCreate(m_nWidth, m_nHeight, 8, AVIF_PIXEL_FORMAT_YUV444); // these values dictate what goes into the final AVIF

		avifRGBImageSetDefaults(&m_rgb, image);
		// Override RGB(A)->YUV(A) defaults here:
		//   depth, format, chromaDownsampling, avoidLibYUV, ignoreAlpha, alphaPremultiplied, etc.
		m_rgb.format = AVIF_RGB_FORMAT_BGRA; //CJPEGImage provides BGRA
		m_rgb.rowBytes = 4 * m_nWidth;
		m_rgb.pixels = (uint8_t*)pData;

		avifResult result = avifImageRGBToYUV(image, &m_rgb);
		if (result == AVIF_RESULT_OK)
		{
			uint64_t nDurationInNumOfTimescales = 1;
			if (nFrameType != AVIF_ADD_IMAGE_FLAG_SINGLE)
			{
				if (nFrameIntervalMs != 0)
				{
					nDurationInNumOfTimescales = (uint64_t)(((double)nFrameIntervalMs) / m_nFrameIntervalMs);
					if (nDurationInNumOfTimescales < 1)
						nDurationInNumOfTimescales = 1;
				}
			}
			// Add single image in your sequence
			result = avifEncoderAddImage(m_pEncoder, image, nDurationInNumOfTimescales, nFrameType);
			if (result == AVIF_RESULT_OK)
			{
				bOk = true;
			}
			//else fprintf(stderr, "Failed to add image to encoder: %s\n", avifResultToString(addImageResult));
		}
		/*
		else
		{
			TCHAR buffer[100];
			CString s(avifResultToString(convertResult));
			_stprintf_s(buffer, 100, _T("Failed to convert to YUV(A): %s"), s.GetString());
			::MessageBox(NULL, CString(_T("Save As AVIF: ")) + buffer, _T("Error"), MB_OK);
		}
		*/
	}
	catch (...) {
	}
	m_rgb.pixels = 0;
	if (image) {
		avifImageDestroy(image);
	}
	return bOk;
}

/*
* Cleanup. Delete file if not complete success.
* Retval: true if encoding succeeded; false o.w.
*/
bool CAvifEncoder::Finish()
{
	if (!m_pEncoder)
	{
		return false;
	}
	try {
		m_pAvifOuput = new avifRWData;
		*m_pAvifOuput = AVIF_DATA_EMPTY;

		avifResult result = avifEncoderFinish(m_pEncoder, m_pAvifOuput);
		if (result == AVIF_RESULT_OK)
		{
			jbyteArray arrOut = m_pEnv->NewByteArray(m_pAvifOuput->size);
			if (arrOut)
			{
				void* bufOut = m_pEnv->GetPrimitiveArrayCritical(arrOut, NULL);
				memcpy_s(bufOut, m_pAvifOuput->size, m_pAvifOuput->data, m_pAvifOuput->size);
				m_pEnv->ReleasePrimitiveArrayCritical(arrOut, bufOut, 0);
				m_pEnv->CallVoidMethod(m_outStream, m_metdWrite, arrOut);
				m_bSuccess = true;
			}
			m_pEnv->DeleteLocalRef(arrOut);
		}
		//else fprintf(stderr, "Failed to finish encode: %s\n", avifResultToString(finishResult));
	}
	catch (...) {
	}
	if (m_pAvifOuput)
	{
		avifRWDataFree(m_pAvifOuput);
		delete m_pAvifOuput;
		m_pAvifOuput = 0;
	}
	return m_bSuccess;
}

// Convenience method to write a non-animated AVIF image
bool CAvifEncoder::SaveAVIF(JNIEnv* pEnv, jobject outStream, jmethodID metdWrite, void* pData, int nWidth, int nHeight, bool bUseLossless, int nQuality)
{
	if (Init(pEnv, outStream, metdWrite, nWidth, nHeight, bUseLossless, nQuality))
	{
		if (WriteSingleImage(pData, bUseLossless, nQuality))
		{
			return Finish();
		}
	}
	return false;
}

// Convenience method to write a non-animated AVIF image
bool CAvifEncoder::StaticSaveAVIF(JNIEnv* pEnv, jobject outStream, jmethodID metdWrite, void* pData, int nWidth, int nHeight, bool bUseLossless, int nQuality)
{
	CAvifEncoder enc;
	return enc.SaveAVIF(pEnv, outStream, metdWrite, pData, nWidth, nHeight, bUseLossless, nQuality);
}
