// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
#include <jni.h>
#include <stdio.h>
#include "sd_gui_libAvifHeicJNI.h"
#include "HEIFWrapper.h"
#include "AVIFWrapper.h"
#include "SaveImage.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

JNIEXPORT jintArray JNICALL Java_sd_gui_libAvifHeicJNI_AvifHeicDecode
(JNIEnv* pEnv, jclass, jbyteArray pSrc, jlong nSize, jint nFrameIndex, jintArray pnOutWidth, jintArray pnOutHeight, jintArray pnOutNumFrames, jlongArray pnOutFrameIntervalMs)
{
    if (!pSrc) return 0;

    int width = 0, height = 0,
        bpp = 0, frame_count = 0,
        nFrameSize = 0;
    __int64 nFrameIntervalMs = 100;
    bool outOfMemory;
    /*
    //this may make copy of array
    jbyte *bufferIn = (jbyte *)pEnv->GetByteArrayElements(pSrc, NULL);
    // read bytes in *b here
    ...
    // release it
    pEnv->ReleaseByteArrayElements(pSrc, bufferIn, 0);
    */
    jsize length = pEnv->GetArrayLength(pSrc);
    void *bufferIn = pEnv->GetPrimitiveArrayCritical(pSrc, NULL);
    /* We need to check in case the VM tried to make a copy. */
    if (!bufferIn || (length < nSize)) {
        /* out of memory exception thrown */
        printf("out of memory!!!\n");
        pEnv->ReleasePrimitiveArrayCritical(pSrc, bufferIn, JNI_ABORT);
        return NULL;
    }
    jintArray pOutFrame = NULL;

    try {
        void* pchOutBuf = 0;
        unsigned char* header = (unsigned char*)bufferIn;
        if (header[0] == 0x00 && header[1] == 0x00 && header[2] == 0x00
            &&  (memcmp(header + 4, "ftyp", 4) == 0)
            && (memcmp(header + 8, "avis", 4) == 0))
        {
            pchOutBuf = AVIFWrapper::ReadImage(
                width,   // width of the image loaded.
                height,  // height of the image loaded.
                bpp,     // BYTES (not bits) PER PIXEL.
                frame_count, // number of top-level images
                outOfMemory, // set to true when no memory to read image
                nFrameIndex, // index of requested frame
                bufferIn, // memory address containing heic compressed data.
                (int)nSize, // size of heic compressed data.
                nFrameSize,
                nFrameIntervalMs);
        }
        else
        {
            pchOutBuf = HeifReader::ReadImage(
                width,   // width of the image loaded.
                height,  // height of the image loaded.
                bpp,     // BYTES (not bits) PER PIXEL.
                frame_count, // number of top-level images
                outOfMemory, // set to true when no memory to read image
                nFrameIndex, // index of requested frame
                bufferIn, // memory address containing heic compressed data.
                (int)nSize, // size of heic compressed data.
                nFrameSize);
        }

        if (!pchOutBuf || (frame_count <= 0) || (nFrameSize <= 0) || (width <= 0) || (height <= 0) || outOfMemory)
        {
            pEnv->ReleasePrimitiveArrayCritical(pSrc, bufferIn, JNI_ABORT);
            return NULL;
        }
        nFrameSize = (nFrameSize + 3) / 4;
        pOutFrame = pEnv->NewIntArray(nFrameSize);
        pEnv->SetIntArrayRegion(pnOutWidth, 0, 1, (jint*)(&width));
        pEnv->SetIntArrayRegion(pnOutHeight, 0, 1, (jint*)(&height));
        pEnv->SetIntArrayRegion(pnOutNumFrames, 0, 1, (jint*)(&frame_count));
        pEnv->SetLongArrayRegion(pnOutFrameIntervalMs, 0, 1, (jlong*)(&nFrameIntervalMs));
        if (pOutFrame)
        {
            pEnv->SetIntArrayRegion(pOutFrame, 0, nFrameSize, (jint*)pchOutBuf);
        }
        delete[] pchOutBuf;
    }
    //catch (...) {
    catch (const std::exception& exc) {
        const char* pErr = exc.what();
        pErr = pErr;
        printf("Something failed! %s\n", exc.what());
    }
    catch (...) {
        printf("Something failed!\n");
    }
    pEnv->ReleasePrimitiveArrayCritical(pSrc, bufferIn, 0);
    return pOutFrame;
}

JNIEXPORT jboolean JNICALL Java_sd_gui_libAvifHeicJNI_AvifHeicWrite
(JNIEnv* pEnv, jclass, jobject outStream, jobjectArray frames, jint nWidth, jint nHeight, jint nNumFrames, jint nQuality, jlongArray frameIntervalsMs)
{
    bool bOk = false,
        bUseLossless = nQuality > 100;

    jclass clsOutputStream = pEnv->FindClass("java/io/OutputStream");
    jmethodID metdWrite = pEnv->GetMethodID(clsOutputStream, "write", "([B)V");
    jsize nNumFramesAvail = pEnv->GetArrayLength(frames),
        nNumIntervals = pEnv->GetArrayLength(frameIntervalsMs);
    if (nNumFramesAvail < nNumFrames)
        nNumFrames = nNumFramesAvail;
    if (nNumFrames == 1)
    {
        jbyteArray arrOut = (jbyteArray)(pEnv->GetObjectArrayElement(frames, 0));
        void* bufOut = pEnv->GetPrimitiveArrayCritical(arrOut, NULL);
        if (bufOut)
        {
            bOk = CAvifEncoder::StaticSaveAVIF(pEnv, outStream, metdWrite, bufOut, nWidth, nHeight, bUseLossless, nQuality);
            pEnv->ReleasePrimitiveArrayCritical(arrOut, bufOut, 0);
        }
    }
    else
    {
        bool bSingleInterval = nNumIntervals < nNumFrames;
        jboolean bIsCopy;
        __int64* pnFrameIntervalMs = pEnv->GetLongArrayElements(frameIntervalsMs, &bIsCopy);

        CAvifEncoder encoder;
        encoder.Init(pEnv, outStream, metdWrite, nWidth, nHeight, nQuality > 100, nQuality, *pnFrameIntervalMs);
        for (int i = 0; i < nNumFrames; ++i)
        {
            jintArray arrFrame = (jintArray)(pEnv->GetObjectArrayElement(frames, i));
            if (arrFrame)
            {
                void* bufFrame = pEnv->GetPrimitiveArrayCritical(arrFrame, NULL);
                if (bufFrame)
                {
                    bOk = encoder.AppendImage(bufFrame, bUseLossless, nQuality, bSingleInterval ? *pnFrameIntervalMs : pnFrameIntervalMs[i], i == 0);
                    pEnv->ReleasePrimitiveArrayCritical(arrFrame, bufFrame, JNI_ABORT); //no need to copy back changes
                }
                pEnv->DeleteLocalRef(arrFrame);
            }
        }
        encoder.Finish();
        pEnv->ReleaseLongArrayElements(frameIntervalsMs, pnFrameIntervalMs, 0);
    }
    return bOk;
}
