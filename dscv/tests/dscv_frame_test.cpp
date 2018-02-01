#include <stdio.h>
#include <fstream>>
#include <osa/osa.h>
#include <dscv/dscv.hpp>
#include <mio/mio.hpp>
#include "../src/dscv_config.h"
#include "dscv_tests.hpp"

using namespace std;
using namespace mio;


static size_t DSCV_getFileSize(const Char *pPath)
{
    FILE *fp;
    size_t size;

    fp = fopen(pPath, "r");
    if (NULL == fp) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    return size;
}


static int DSCV_readFile(const Char *pPath, void **ppContent, size_t *pLen)
{
    FILE *fp = NULL;
    size_t fileLen = 0;
    size_t totalReadLen = 0;
    size_t readLen = 0;
    size_t itemsCount;
    int error;
    void *pContent = NULL;
    void *pCursor = NULL;

    
    fp = fopen(pPath, "rb");
    if (NULL == fp) {
        return errno;
    }
    
    fileLen = DSCV_getFileSize(pPath);
    totalReadLen = *pLen > 0 ? *pLen : fileLen;    

    pContent = malloc(totalReadLen);
    if (NULL == pContent) {
        return OSA_STATUS_ENOMEM;
    }

    for (pCursor = pContent, readLen = 0; readLen < totalReadLen && !feof(fp); pCursor = (void *)((size_t)pContent + readLen)) {
        error = 0;
        itemsCount = fread(pCursor, 1, 1, fp);
        if (error) {
            OSA_error("Encountered error %d while reading.\n", error);
        }
        readLen += itemsCount * 1;
    }

    fclose(fp);
    *ppContent = pContent;
    *pLen = totalReadLen;
    return OSA_STATUS_OK;
}



static void DSCV_releaseFile(void *pContent)
{
    free(pContent);
}



static int DSCV_encodeMat_Test(const Mat &mat)
{
    int ret;
    vector<Uchar> encodedImage;
    FILE *fp;

    
    // imwrite("encodedImage.jpg", mat);   

    ret = DSCV_encodeMat(mat, DSCV_FRAME_TYPE_JPG, encodedImage);
    if (OSA_isFailed(ret)) {
        return ret;
    }

#if 1
    fp = fopen("encodedImage.jpg", "wb");
    if (NULL == fp) {
        return OSA_STATUS_EGENERAL;
    }

    fwrite(encodedImage.data(), 1, encodedImage.size(), fp);
    fclose(fp);
#else
    std::string str_encode(encodedImage.begin(), encodedImage.end());  
    ofstream ofs("encodedImage.jpg", ofstream::binary);  
    assert(ofs.is_open());  
    ofs << str_encode;  
    ofs.flush();  
    ofs.close();  
#endif

    return OSA_STATUS_OK;
}


static int DSCV_jpgToMat_Test()
{
    int ret;
    DSCV_Frame frame;
    Mat mat;
    const char *path = "D:/CV/Supporting/Pic.jpg";


    OSA_clear(&frame);
    frame.type = DSCV_FRAME_TYPE_JPG;
    frame.dataLen = 0;
    ret = DSCV_readFile(path, &frame.pData, &frame.dataLen);
    assert(OSA_isSucceeded(ret));
    
    ret = DSCV_matFromFrame(&frame, mat);
    assert(OSA_isSucceeded(ret));

    imshow("JPG to Mat", mat);
    waitKey(0);

    DSCV_releaseFile(frame.pData);
    return ret;
}



static int DSCV_matFromFrame_Test()
{
    int ret;
    DSCV_Frame frame;
    Mat mat;
    const char *path = "D:/CV/Supporting/YUYV_640x480.yuv";
    
    frame.type = DSCV_FRAME_TYPE_YUYV;
    frame.size.w = 640;
    frame.size.h = 480;
    frame.dataLen = frame.size.w * frame.size.h * 2;

#if 1
    ret = DSCV_readFile(path, &frame.pData, &frame.dataLen);
    assert(OSA_isSucceeded(ret));
#else
    FILE *fp = fopen(path, "r");
    assert(NULL != fp);
    frame.pData = malloc(frame.dataLen);
    fread(frame.pData, 1, frame.dataLen, fp);
    fclose(fp);
#endif

    ret = DSCV_matFromFrame(&frame, mat);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to convert YUV to mat: %d.\n", ret);
        return ret;
    }

    imshow("YUV to Mat", mat);
    waitKey(0);

    DSCV_encodeMat_Test(mat);

    free(frame.pData);
    return ret;
}


static int DSCV_matFromFile_Test2()
{
    int ret;
    Mat mat;
    DSCV_Frame frame;

    frame.type = DSCV_FRAME_TYPE_YUYV;
    frame.size.w = 640;
    frame.size.h = 480;
    ret = DSCV_matFromFile("D:/Temp/DumpFrame_0_1358802375", &frame, mat);
    if (OSA_isFailed(ret)) {
        return ret;
    }

    imshow("Mat From File", mat);
    waitKey(0);
    return ret;
}



int DSCV_frame_Tests()
{    
    DSCV_matFromFile_Test2();
    // DSCV_matFromFrame_Test();
    // DSCV_jpgToMat_Test();
    return 0;
}
