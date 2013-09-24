////////////////////////////////////////////////////////////////////////////////
// 版权所有，2009-2012，北京汉邦高科数字技术有限公司
// 本文件是未公开的，包含了汉邦高科的机密和专利内容
////////////////////////////////////////////////////////////////////////////////
// 文件名： audio_in.c
// 作者：   封欣明
// 版本：   1.0
// 日期：   2012-11-23
// 描述：   音频输入模块实现
// 历史记录：
///////////////////////////////////////////////////////////////////////////////
#include "audio_in_private.h"

static AUDIO_IN_SYS_CFG_OBJ    audio_in_cfg;
HB_S32 start_flag;

////////////////////////////////////////////////////////////////////////////////
// 函数名：get_audio_thdfxn
// 描述：读取音频数据线程。
// 参数：
//  ［IN］parg - 线程参数。
// 返回值：无
// 说明：
//
////////////////////////////////////////////////////////////////////////////////
HB_VOID *get_audio_thdfxn(HB_VOID *parg)
{ 
#ifdef DEBUG
    assert(NULL != parg);
#endif
    if (NULL == parg)
    {
        return (HB_VOID *)0;
    }

    HB_S32 buff[4 * 1024];
    HB_S32 size = 0;
    HB_S32 i = 0;

    AUDIO_IN_SYS_CFG_HANDLE paudio = (AUDIO_IN_SYS_CFG_HANDLE)parg;
    AUDIO_IN_DATA_OBJ data;
    AUDIO_IN_CALLBACK *data_process = paudio->callback;

	while(!start_flag)
		usleep(10);
    while(THREAD_RUNNING == paudio->thread_status)
    {
        if (AUDIO_OK != device_in_capture(paudio, buff, &size))
        {
            //AUDIO_DEBUG("capture data error !");
            continue;
        }
        data.pdata_addr = buff;
        data.data_size = size;

        if (NULL == data_process)
        {
            continue;
        }
        for(i = 0;i < paudio->counter;i ++)
        {
            //printf("i = %d paudio->counter = %d data.data_size = %d\n\n\n\n",i,paudio->counter,data.data_size);
            data_process[i](NULL, &data, paudio->pcontext[i]);
        }
    }
    return (HB_VOID *)0;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：audio_in_open
// 描述：打开音频输入设备。
// 参数：
//  ［OUT］handle - 模块句柄。
//  ［IN］arg_in - 音频输入参数。
//  ［IN］pcontext - 不需要关心。
// 返回值：
//  错误代码。
// 说明：
//
////////////////////////////////////////////////////////////////////////////////
HB_S32 audio_in_open(HB_HANDLE * handle, AUDIO_IN_ARG_OBJ arg_in, HB_VOID *pcontext)
{
#ifdef DEBUG
    assert(NULL != handle);
    //assert(NULL != pcontext);
#endif
    //if (NULL == handle || NULL == pcontext)
    if (NULL == handle)
    {
        AUDIO_ERROR("audio_in_open failed !");
        return AUDIO_IN_ERRNO(COM_ERR_INVALID_PARAMETER);
    }

	printf("Jorney audio_in1 \n");
    AUDIO_IN_SYS_CFG_HANDLE pfd = &audio_in_cfg;
    HB_CHAR audio_device[] = "default";

    if (NULL == pfd)
    {
        return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
    }

	printf("Jorney audio_in2 \n");
	AUDIO_DEBUG("mic_num:     [%d]", arg_in.reserved.mic);
	AUDIO_DEBUG("line_in_num: [%d]", arg_in.reserved.line_in);
	/*

    if (0 == arg_in.reserved.mic && 0 == arg_in.reserved.line_in)
    {
        pfd->is_support = DISABLE;
        *handle = pfd;
        AUDIO_DEBUG("audio in isn't support");
        return AUDIO_IN_ERRNO(COM_ERR_NOT_SUPPORTED);
    */
    //该模块只在第一次打开时初始化音频输入参数
    if(0 == pfd->counter)
    {
        AUDIO_DEBUG("audio_in_open first !");
		memset(pfd, 0, sizeof(AUDIO_IN_SYS_CFG_OBJ));
		if (AUDIO_OK != device_in_open(pfd, audio_device))
		{
			AUDIO_ERROR("Audio device open failed !");
			return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
		}
		memcpy(&pfd->config, &arg_in.config, sizeof(AUDIO_IN_ENCODE_CFG_OBJ));	  //音频初始化参数赋值

        pfd->alsa.period_size = 320;
#if 0
        if (device_in_set_params(pfd) != AUDIO_OK)
        {
            AUDIO_ERROR("set audio init params failed !");
            return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
        }
#endif
    }
    pfd->is_support = ENABLE;
    /*
    else
    {
        if(0 != memcmp(&pfd->config, &arg_in.config, sizeof(AUDIO_IN_ENCODE_CFG_OBJ)))
        {
            AUDIO_ERROR("Audio device config changed !");
            return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
        }
    }
    */
    pfd->callback[pfd->counter] = arg_in.callback;
    pfd->pcontext[pfd->counter] = pcontext;
    if(MODULE_CLOSE == pfd->running_flag)
    {
        pfd->running_flag = MODULE_OPEN;
    }
    pfd->flag = MODULE_AUDIO_IN;
    pfd->healthy = MODULE_HEALTHY;
    pfd->counter += 1;
    *handle = pfd;
	start_flag = 0;

    AUDIO_DEBUG("audio_in_open success !");
    return COMMON_OK;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：audio_in_start
// 描述：开始音频输入设备。
// 参数：
//  ［IN］handle - 模块句柄。
//   [IN] priority - 优先级
// 返回值：
//  错误代码。
// 说明：
//
////////////////////////////////////////////////////////////////////////////////
HB_S32 audio_in_start(HB_HANDLE handle, HB_S32 priority)
{
#ifdef DEBUG
    assert(NULL != handle);
#endif
    if (NULL == handle)
    {
        AUDIO_ERROR("audio_in_start failed !");
        return AUDIO_IN_ERRNO(COM_ERR_HANDLEINVAL);
    }

    AUDIO_IN_SYS_CFG_HANDLE pfd = (AUDIO_IN_SYS_CFG_HANDLE)handle;
    
    if (DISABLE == pfd->is_support)
    {
        AUDIO_DEBUG("audio in isn't support");
        return AUDIO_IN_ERRNO(COM_ERR_NOT_SUPPORTED);
    }

    if (0 == AUDIOIN_FLAG_VERIFY(pfd))
    {
        AUDIO_ERROR("Invalid handle for audio !");
        return AUDIO_IN_ERRNO(COM_ERR_INVALID_PARAMETER);
    }
    
    if(MODULE_START == pfd->running_flag)
    {
        AUDIO_DEBUG("audio_in_start success again !");
        return COMMON_OK;
    }

    pthread_attr_t attr;
    pthread_t *thr_id = &pfd->thr_id;


    if (MODULE_OPEN != pfd->running_flag && MODULE_STOP != pfd->running_flag)
    {
        return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
    }
    pfd->running_flag = MODULE_START;
    pfd->thread_status = THREAD_RUNNING;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (pthread_create(thr_id, &attr, get_audio_thdfxn, (HB_VOID *)pfd) != 0)
    {
        AUDIO_ERROR("pthread_create failed !");
        return AUDIO_IN_ERRNO(COM_ERR_THREADSTART);
    }
    pthread_attr_destroy(&attr);

    AUDIO_DEBUG("audio_in_start success !");

    return COMMON_OK;
}


////////////////////////////////////////////////////////////////////////////////
// 函数名：audio_in_stop
// 描述：停止音频输入设备。
// 参数：
//  ［IN］handle - 模块句柄。
// 返回值：
//  错误代码。
// 说明：
//
///////////////////////////////////////////////////////////////////////////////
HB_S32 audio_in_stop(HB_HANDLE handle)
{
#ifdef DEBUG
    assert(NULL != handle);
#endif
    if (NULL == handle)
    {
        AUDIO_ERROR("audio_in_stop failed !");
        return AUDIO_IN_ERRNO(COM_ERR_HANDLEINVAL);
    }

    HB_VOID * pstatus = NULL;
    AUDIO_IN_SYS_CFG_HANDLE pfd = (AUDIO_IN_SYS_CFG_HANDLE)handle;
	
    if (DISABLE == pfd->is_support)
    {
        AUDIO_DEBUG("audio in isn't support");
        return AUDIO_IN_ERRNO(COM_ERR_NOT_SUPPORTED);
    }

    if (0 == AUDIOIN_FLAG_VERIFY(pfd))
    {
        AUDIO_ERROR("Invalid handle for audio !");
        return AUDIO_IN_ERRNO(COM_ERR_INVALID_PARAMETER);
    }

    pthread_t thr_id = pfd->thr_id;

    if (MODULE_START != pfd->running_flag)
    {
        return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
    }
    pfd->running_flag = MODULE_STOP;
    pfd->thread_status = THREAD_STOP;
    if (pthread_join(thr_id, &pstatus) != 0)
    {
        perror("pthread_join failed !");
        return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
    }

    AUDIO_DEBUG("audio_in_stop success !");
    return COMMON_OK;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：audio_in_close
// 描述：关闭音频输入设备。
// 参数：
//  ［IN］handle - 模块句柄。
// 返回值：
//  错误代码。
// 说明：
//
////////////////////////////////////////////////////////////////////////////////
HB_S32 audio_in_close(HB_HANDLE handle)
{
#ifdef DEBUG
    assert(NULL != handle);
#endif
    if (NULL == handle)
    {
        AUDIO_ERROR("audio_in_close failed !");
        return AUDIO_IN_ERRNO(COM_ERR_HANDLEINVAL);
    }

    AUDIO_IN_SYS_CFG_HANDLE pfd = (AUDIO_IN_SYS_CFG_HANDLE)handle;

    if (DISABLE == pfd->is_support)
    {
        AUDIO_DEBUG("audio in isn't support");
        return AUDIO_IN_ERRNO(COM_ERR_NOT_SUPPORTED);
    }

    if (0 == AUDIOIN_FLAG_VERIFY(pfd))
    {
        AUDIO_ERROR("Invalid handle for audio !");
        return AUDIO_IN_ERRNO(COM_ERR_INVALID_PARAMETER);
    }
    
    if (MODULE_STOP != pfd->running_flag && MODULE_OPEN != pfd->running_flag)
    {
        return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
    }
    if (AUDIO_OK != device_in_close(pfd))
    {
        AUDIO_ERROR("Audio in device close error !");
        return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
    }

    pfd->running_flag = MODULE_CLOSE;
    pfd->counter -= 1;

    AUDIO_DEBUG("audio_in_close success !");

    return COMMON_OK;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：audio_in_ioctrl
// 描述：音频输入控制。
// 参数：
//  ［IN］handle - 模块句柄。
//  ［IN］cmd - 命令码。
//  ［IN］pinput – 输入数据地址。
//  ［IN］ ninlen – 输入数据长度。
//  ［OUT］poutput – 输出数据地址。
//  ［OUT］poutlen – 输出数据长度。
// 返回值：
//  错误代码。
// 说明：
//
////////////////////////////////////////////////////////////////////////////////
HB_S32 audio_in_ioctrl(HB_HANDLE handle, HB_S32 cmd, HB_VOID * pinput, HB_S32 ninlen, HB_VOID * poutput, HB_S32 *noutlen)
{
#ifdef DEBUG
    assert(NULL != handle);
#endif
    if (NULL == handle)
    {
        AUDIO_ERROR("audio_in_ioctrl failed !");
        return AUDIO_IN_ERRNO(COM_ERR_INVALID_PARAMETER);
    }

    AUDIO_IN_SYS_CFG_HANDLE pfd = (AUDIO_IN_SYS_CFG_HANDLE)handle;

    if (DISABLE == pfd->is_support)
    {
        AUDIO_DEBUG("audio in isn't support");
        return AUDIO_IN_ERRNO(COM_ERR_NOT_SUPPORTED);
    }

    if (0 == AUDIOIN_FLAG_VERIFY(pfd))
    {
        AUDIO_ERROR("Invalid handle for audio !");
        return AUDIO_IN_ERRNO(COM_ERR_INVALID_PARAMETER);
    }

    if (MODULE_START != pfd->running_flag)
    {
        return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
    }

    switch (cmd)
    {
        case SET_AUDIO_IN_CFG :
            {
                AUDIO_DEBUG("NOT SUPPORTED NOW !");
                break;
            }
        case GET_AUDIO_IN_CFG :
            {
                AUDIO_DEBUG("NOT SUPPORTED NOW !");
                break;
            }
        case SET_AUDIO_IN_VOLUME :
            {
				device_in_set_vol(*(HB_S32 *)pinput);
                break;
            }
        case GET_AUDIO_IN_VOLUME :
            {
				*(HB_S32 *)poutput = device_in_get_vol();
                break;
            }
		case 100:

			if (device_in_set_params(pfd) != AUDIO_OK)
			{
				AUDIO_ERROR("set audio init params failed !");
				return AUDIO_IN_ERRNO(COM_ERR_GENERIC);
			}
			start_flag = 1;
			break;
        default :
            {
                AUDIO_ERROR("Unknown CMD for audio_in_ioctrl !");
                break;
            }
    }

    return COMMON_OK;
}

