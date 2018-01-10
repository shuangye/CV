#include <osa/osa.h>
#include <comm/comm.h>


int COMM_init(COMM_Options *pOptions, COMM_Handle *pHandle)
{
    return OSA_STATUS_OK;
}

int COMM_deinit(COMM_Handle handle)
{
    return OSA_STATUS_OK;
}

int COMM_bind(COMM_Handle handle, Int32 *pBoundSd)
{
    return OSA_STATUS_OK;
}

int COMM_listen(COMM_Handle handle)
{
    return OSA_STATUS_OK;
}

int COMM_accept(COMM_Handle handle, Int32 *pAcceptedSd)
{
    return OSA_STATUS_OK;
}

int COMM_connect(COMM_Handle handle, const COMM_Addr *pRemoteAddr)
{
    return OSA_STATUS_OK;
}

int COMM_send(COMM_Handle handle, const Int32 commSd, const COMM_DataChunk *pDataChunk)
{
    return OSA_STATUS_OK;
}

int COMM_recv(COMM_Handle handle, const Int32 commSd, COMM_DataChunk *pDataChunk)
{
    return OSA_STATUS_OK;
}

int COMM_close(const Int32 sd)
{
    return OSA_STATUS_OK;
}

