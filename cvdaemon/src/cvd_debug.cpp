/*
* Created by Liu Papillon, on Dec 21, 2017.
*/

#include <unistd.h>
#include <time.h>
#include <osa/osa.h>
#include "cvd_debug.hpp"

static const string gDumpDir = string("/data/rk_backup/");




void CVD_debugDumpImages(const string label, const vector<Mat> images)
{
    stringstream stream;
    string controlFilePath = gDumpDir + label;
    string dumpFilePath;
    long timestamp;

    if (0 != access(controlFilePath.c_str(), F_OK)) {
        return;
    }

    timestamp = (long)time(NULL);
    for (size_t i = 0; i < images.size(); ++i) {
        if (images[i].empty()) {
            continue;
        }
        stream.str("");
        stream.clear();        
        stream << gDumpDir << label << "_" << i << "_" << timestamp << ".jpg";
        imwrite(stream.str(), images[i]);
    }
}


