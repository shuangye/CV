#pragma once
#ifndef __MEDIAD_COMMAND_PRIVATE_H__
#define __MEDIAD_COMMAND_PRIVATE_H__

/*
* Created by Liu Papillon, on Dec 26, 2017.
*/


#ifdef __cplusplus
extern "C" {
#endif    

    int MEDIAD_cmdInit();

    int MEDIAD_cmdDeinit();

    int MEDIAD_cmdWait();

#ifdef __cplusplus
}
#endif

#endif  /* __MEDIAD_COMMAND_PRIVATE_H__ */
