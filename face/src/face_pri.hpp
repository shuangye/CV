#pragma once
#ifndef __DS_FACE_PRI_HPP__
#define __DS_FACE_PRI_HPP__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*/


#include <osa/osa.h>
#include <face/face_definitions.h>
#include "face_detect.hpp"
#include "face_living.hpp"


typedef struct FACE_Module {
    FACE_Options               options;
    Detector                  *pDetector;
    Living                    *pLiving;
} FACE_Module;


#endif // __DS_FACE_PRI_HPP__
