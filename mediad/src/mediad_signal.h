#pragma once
#ifndef __MEDIAD_SIGNAL_H__
#define __MEDIAD_SIGNAL_H__

/*
* Created by Liu Papillon, on Dec 26, 2017.
*/


#include <signal.h>
#include <setjmp.h>



#ifdef __cplusplus
extern "C" {
#endif    
    
    extern sigjmp_buf MEDIAD_gStackEnv;


    int MEDIAD_sigInit();

    int MEDIAD_sigDeinit();

    int MEDIAD_getSig();

    int MEDIAD_sigCheck(const int signum);

    void MEDIAD_sigClear(const int signum);

#ifdef __cplusplus
}
#endif


#endif  /* __MEDIAD_SIGNAL_H__ */
