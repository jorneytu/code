////////////////////////////////////////////////////////////////////////////////
// 版权所有，2009-2012，北京汉邦高科数字技术有限公司
// 本文件是未公开的，包含了汉邦高科的机密和专利内容
////////////////////////////////////////////////////////////////////////////////
// 文件名： TI_audio_out.c
// 作者：   封欣明
// 版本：   1.0
// 日期：   2012-11-23
// 描述：   音频输入模块实现
// 历史记录：
///////////////////////////////////////////////////////////////////////////////
#include "G711.h"
#include "audio_out_private.h"

#define CHANNEL_NUM        (1)
#define PERIOD_SIZE        (320)

////////////////////////////////////////////////////////////////////////////////
// 函数名：device_out_open
// 描述：音频输入设备打开。
// 参数：
//  ［IN］paudio - 模块句柄。
//   [IN] pdev_name - 设备名称的地址指针
// 返回值：
//  错误代码。
// 说明：
//
///////////////////////////////////////////////////////////////////////////////
HB_S32 device_out_open(AUDIO_OUT_SYS_CFG_HANDLE p_audio, HB_CHAR *pdev_name)
{
#ifdef DEBUG
    assert(NULL != p_audio);
    assert(NULL != pdev_name);
#endif
    if (NULL == p_audio || NULL == pdev_name)
    {
        AUDIO_ERROR("device_out_open failed");
        return AUDIO_FAIL;
    }

    HB_S32 ret = 0;
    snd_pcm_t *handle;

    p_audio->p_dec = g722Codec_init();
    ret = snd_pcm_open(&handle, (HB_CHAR *)pdev_name, SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
    {
        AUDIO_ERROR("Unable to open audio device %s !", (HB_CHAR *)pdev_name);
        return AUDIO_FAIL;
    }
    p_audio->alsa.handle = handle;
    AUDIO_DEBUG("open audio device %s success !", (HB_CHAR *)pdev_name);
    return AUDIO_OK;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：device_out_set_params
// 描述：音频输出设备设置参数。
// 参数：
//  ［IN］paudio - 模块句柄。
// 返回值：
//  错误代码。
// 说明：
//
///////////////////////////////////////////////////////////////////////////////
HB_S32 device_out_set_params(AUDIO_OUT_SYS_CFG_HANDLE p_audio)
{
#ifdef DEBUG
    assert(NULL != p_audio);
#endif
    if (NULL == p_audio)
    {
        AUDIO_ERROR("device_out_set_params failed");
        return AUDIO_FAIL;
    }
    HB_S32 ret = -1;
    HB_S32 dir = 0;
    HB_S32 bit_per_sample = 0;
    snd_pcm_t *pcm_handle = p_audio->alsa.handle;
    snd_pcm_uframes_t frames = 0;
    snd_pcm_uframes_t buffer_frames = 0;
    HB_U32 val = 0;
    HB_U32 buffer_time = 0;
    snd_pcm_hw_params_t *sound_params = NULL;
    snd_pcm_hw_params_t **p_sound_params = &sound_params;

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(p_sound_params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(pcm_handle, sound_params);

    /* Set the desired hardware parameters. */

    /* Sampling rate*/
    val = p_audio->config.sample_rate;
    snd_pcm_hw_params_set_rate_near(pcm_handle, sound_params, &val, &dir);
    if (val != p_audio->config.sample_rate)
    {
        AUDIO_DEBUG("Rate doesn't match (requested %iHz, get %iHz)", p_audio->config.sample_rate, val);
        return AUDIO_FAIL;
    }

    /* 使用G711 A_LAW 编码 */
    snd_pcm_hw_params_set_format(pcm_handle, sound_params, SND_PCM_FORMAT_S16_LE);
    /* Interleaved mode */
    snd_pcm_hw_params_set_access(pcm_handle, sound_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    /* Num channels */
    snd_pcm_hw_params_set_channels(pcm_handle, sound_params, CHANNEL_NUM);

    /* Set period size of frames. */
    frames = 320;
    snd_pcm_hw_params_set_period_size_near(pcm_handle, sound_params, &frames, &dir);

    /* set the buffer time */
    if(buffer_time)
    {
        ret = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, sound_params, &buffer_time, &dir);
        if (ret < 0) 
        {
            AUDIO_DEBUG("Unable to set buffer time %i for playback: %s", buffer_time, snd_strerror(ret));
            return AUDIO_FAIL;
        }
    }
    else
    {
        buffer_frames =  frames * 4;
        snd_pcm_hw_params_set_buffer_size_near(pcm_handle, sound_params, &buffer_frames);
    }

    /* Write the parameters to the driver */
    ret = snd_pcm_hw_params(pcm_handle, sound_params);
    if (ret < 0)
    {
        AUDIO_DEBUG("unable to set hw parameters: %s", snd_strerror(ret));
        return AUDIO_FAIL;
    }

    /* Use a buffer large enough to hold one period */
    //获取周期长度
    snd_pcm_hw_params_get_period_size(sound_params, &frames, &dir);
    snd_pcm_hw_params_get_period_time(sound_params, &val, &dir);
    //获取样本长度
    bit_per_sample = snd_pcm_format_physical_width(SND_PCM_FORMAT_S16_LE);
    //计算周期长度（字节数(bytes) = 每周期的桢数 * 样本长度(bit) * 通道数 / 8 ）
    p_audio->alsa.chunk_size = frames * bit_per_sample * CHANNEL_NUM / 8;
    p_audio->alsa.period_size = frames;

    AUDIO_DEBUG("chunk_size %d ", p_audio->alsa.chunk_size);
    AUDIO_DEBUG("period size = %d frames, dir = %d", (int)frames, dir);

    return AUDIO_OK;
}

////////////////////////////////////////////////////////////////////////////////
// 函数名：device_out_close
// 描述：音频输入设备打开。
// 参数：
//  ［IN］paudio - 模块句柄。
// 返回值：
//  错误代码。
// 说明：
//
///////////////////////////////////////////////////////////////////////////////
HB_S32 device_out_close(AUDIO_OUT_SYS_CFG_HANDLE p_audio)
{
#ifdef DEBUG
    assert(NULL != p_audio);
#endif
    if (NULL == p_audio)
    {
        AUDIO_ERROR("device_out_close failed");
        return AUDIO_FAIL;
    }

    HB_S32 ret = 0;

    ret = snd_pcm_close(p_audio->alsa.handle);
    if (ret < 0)
    {
        AUDIO_ERROR("Fail to close audio device !");
        return AUDIO_FAIL;
    }
    AUDIO_DEBUG("close audio device success !");

    return AUDIO_OK;
}

HB_S32 g722toalaw(HB_CHAR *pbuff, HB_U32 length)
{
    /*
    HB_U32 len = 320, i;
    uint16 usTemp;
    g722Codec_dec(pDec, (HB_S16 *)decode_stream, len);

    */
    return AUDIO_OK;
}


////////////////////////////////////////////////////////////////////////////////
// 函数名：device_out_playback
// 描述：音频输出设备播放数据。
// 参数：
//  ［IN］paudio - 模块句柄。
//   [OUT] pbuff - 数据缓冲区
//   [OUT] psize - 数据的长度
// 返回值：
//  错误代码。
// 说明：
//
///////////////////////////////////////////////////////////////////////////////
HB_S32 device_out_playback(AUDIO_OUT_SYS_CFG_HANDLE p_audio, HB_U32 *pbuff, HB_S32 size)
{
#ifdef DEBUG
    assert(NULL != p_audio);
    assert(NULL != pbuff);
#endif
    if (NULL == p_audio || NULL == pbuff)
    {
        AUDIO_ERROR("Playback buffer can't be NULL !");
        return AUDIO_FAIL;
    }

    HB_S32 ret = 0;
    //HB_S16 pcm_unit;
    //HB_U8  alaw_unit;
    //HB_CHAR  pcm_buff[2 * 1024];
    HB_CHAR  pcm_buff[2 * PERIOD_SIZE];
    //HB_CHAR  *p_buff;
    //HB_CHAR  *p_pcm;

    g722Codec_dec(p_audio->p_dec, (HB_S16 *)pcm_buff, (HB_U8*)pbuff, PERIOD_SIZE);

    //size为G711编码的8位数据大小，chunk_size为传入声卡需要的16位PCM数据大小 chunk_size为size的2倍
    #if 0
    if (size * 2 != p_audio->alsa.chunk_size)
    {
        AUDIO_ERROR("error size for playback(need %d not %d) !", p_audio->alsa.chunk_size, size);
        return AUDIO_FAIL;
    }

    p_buff = (HB_CHAR *)buff;
    p_pcm = pcm_buff;

    //G711解码为16位PCM
    while(size)
    {
        memcpy(&alaw_unit, p_buff, 1);
        pcm_unit = alaw2linear(alaw_unit);
        memcpy(p_pcm, &pcm_unit, 2);
        size -= 1;
        p_pcm += 2;
        p_buff++;
    }
    #endif

    ret = snd_pcm_writei(p_audio->alsa.handle, pcm_buff, p_audio->alsa.period_size);
    if (ret == -EPIPE)
    {
        /* EPIPE means overrun */
        AUDIO_ERROR("audio overrun occurred");
        if (snd_pcm_prepare(p_audio->alsa.handle) < 0)
        {
            AUDIO_ERROR("Failed to recover from underrun !");
        }
    }
    else if(ret < 0)
    {
        AUDIO_ERROR("audio error from write : %s", (char *)snd_strerror(ret));
        if (snd_pcm_prepare(p_audio->alsa.handle) < 0)
        {
            AUDIO_ERROR("Failed to recover from snd_strerror !");
        }
    }

    return AUDIO_OK;
}

