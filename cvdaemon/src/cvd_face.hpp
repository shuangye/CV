#pragma once
#ifndef __CVD_FACE_HPP__
#define __CVD_FACE_HPP__

/*
* Created by Liu Papillon, on Nov 14, 2017.
*/


int CVD_faceInit();

int CVD_faceDeinit();

void CVD_faceInvalidateRectifier();

int CVD_faceWait();

#ifdef __cplusplus
extern "C" {
#endif     


#ifdef __cplusplus
}
#endif

#endif  /* __CVD_FACE_HPP__ */
