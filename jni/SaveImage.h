#pragma once

#include "libavif/avif/avif.h"
#include <jni.h>

class CJPEGImage;

class CAvifEncoder
{
public:
	CAvifEncoder();
	~CAvifEncoder();

	bool Init(JNIEnv* pEnv, jobject outStream, jmethodID metdWrite, int nWidth, int nHeight, bool bUseLossless = false, int nQuality = 60, uint64_t nFrameIntervalMs = 1000);
	bool WriteSingleImage(void* pData, bool bUseLossless = false, int nQuality = 60);
	bool AppendImage(void* pData, bool bUseLossless = false, int nQuality = 60,
		uint64_t nFrameIntervalMs = 0, bool bKeyFrame = false);
	bool Finish();
	bool SaveAVIF(JNIEnv* pEnv, jobject outStream, jmethodID metdWrite, void* pData, int nWidth, int nHeight, bool bUseLossless = false, int nQuality = 60);

	static bool StaticSaveAVIF(JNIEnv* pEnv, jobject outStream, jmethodID metdWrite, void* pData, int nWidth, int nHeight, bool bUseLossless, int nQuality);

private:
	bool WriteImage(void* pData, bool bUseLossless = false, int nQuality = 60,
		uint64_t nFrameIntervalMs = 0, avifAddImageFlag nFrameType = AVIF_ADD_IMAGE_FLAG_SINGLE);
	JNIEnv* m_pEnv;
	jobject m_outStream;
	jmethodID m_metdWrite;
	avifEncoder *m_pEncoder;
	avifRWData *m_pAvifOuput;
	avifRGBImage m_rgb;
	int m_nWidth, m_nHeight;
	uint64_t m_nTimeScaleHz, m_nFrameIntervalMs;
	int m_nQuality;
	bool m_bLossless, m_bSuccess;
};