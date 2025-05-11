#include "savevideofilethread.h"

SaveVideoFileThread::SaveVideoFileThread(QObject *parent)
    : QThread{parent},m_infor(),mAudioOneFrameSize(0),m_videoBeginFlag(false)
{
    m_PicInPic_Reader=new PicInPic_Read();
    m_Audio_Reader=new Audio_Read();
    connect(m_PicInPic_Reader,&PicInPic_Read::SIG_sendVideoFrame,this,&SaveVideoFileThread::SIG_sendVideoFrame);
    connect(m_PicInPic_Reader,&PicInPic_Read::SIG_sendPicInPic,this,&SaveVideoFileThread::SIG_sendPicInPic);
    connect(m_PicInPic_Reader,&PicInPic_Read::SIG_sendVideoFrameData,this,&SaveVideoFileThread::SLO_saveVideoFrameData);
    connect(m_Audio_Reader,&Audio_Read::SIG_sendAudioFrameData,this,&SaveVideoFileThread::slot_saveAudioFrameData);
    mAudioOneFrameSize=0;
    lastVideoNode=nullptr;
    m_videoBeginFlag=false;
    m_videoBeginTime=0;
    isStop=false;
    video_pts=0;
    audio_pts=0;
}

void SaveVideoFileThread::slot_closeVideo()
{
    if( m_infor.have_screen||m_infor.have_camera)
        m_PicInPic_Reader->slot_closeVideo();
    if( m_infor.have_audio )
        m_Audio_Reader->slot_closeAudio();
    isStop = true;
}

void SaveVideoFileThread::slot_openVideo()
{
    isStop=false;
    if( m_infor.have_screen||m_infor.have_camera){
        m_videoBeginFlag = true;
        m_PicInPic_Reader->slot_openVideo();
    }
    if( m_infor.have_audio ){
        m_Audio_Reader->slot_openAudio();
    }
}


void SaveVideoFileThread::SLO_saveVideoFrameData(uint8_t *picture_buf, int buffer_size)
{
    //记录第一次时间
    //投递队列时, 每一帧数据需要 记录对应的时间
    if( m_videoBeginFlag ) {
        m_videoBeginTime = QDateTime::currentMSecsSinceEpoch();;
        m_videoBeginFlag = false;
    }
    qint64 curTime = QDateTime::currentMSecsSinceEpoch();
    qint64 pts = curTime - m_videoBeginTime;
    videoDataQuene_Input( picture_buf , buffer_size , pts );
    /*****************************/
    // //AVCodecContext *c =video_st.enc;
    // memcpy(video_st.frameBuffer,picture_buf,buffer_size);
    // av_free(picture_buf);
    // video_st.frame->pts=video_st.next_pts++;
    // write_frame(oc, video_st.enc, video_st.st, video_st.frame);
}

void SaveVideoFileThread::slot_saveAudioFrameData(uint8_t *picture_buf, int buffer_size)
{
    //视频如果没有 , 丢弃帧 拉齐视音频时间
    if( (m_infor.have_camera || m_infor.have_screen)&& m_videoBeginFlag )
    {
        free( picture_buf );
        return;
    }
    audioDataQuene_Input( picture_buf , buffer_size );
    // AVCodecContext *c;
    // c = audio_st.enc;
    // memcpy( audio_st.frameBuffer , picture_buf , buffer_size );
    // free( picture_buf );
    // audio_st.frame->pts = audio_st.next_pts;
    // audio_st.next_pts += audio_st.frame->nb_samples;
    // AVRational rational;
    // rational.num = 1;
    // rational.den = c->sample_rate;
    // audio_st.frame->pts = av_rescale_q(audio_st.samples_count, rational, c->time_base);
    // audio_st.samples_count += audio_st.frame->nb_samples;
    // write_frame(oc, audio_st.enc, audio_st.st, audio_st.frame);
}

void SaveVideoFileThread::SLO_setInfor(av_infor &infor)
{
    m_infor=infor;
    //////////////////打开文件并对文件初始化文件上下文////////////////////
    char filename[260]="";
    sprintf(filename,"%s",infor.filename.toStdString().c_str());
    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, "flv", filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        return;
    fmt = oc->oformat;
    ////////////////加入视频流至文件上下文中，并使用video_st保存视频流对应的信息///////////////////////////
    if (m_infor.have_screen) {
        add_stream(&video_st, oc, &video_codec, AV_CODEC_ID_H264);
        have_video = 1;
        encode_video = 1;
    }
    if (m_infor.have_audio) {
        add_stream(&audio_st, oc, &audio_codec, AV_CODEC_ID_AAC);
        have_audio = 1;
        encode_audio = 1;
    }
    /////////////////如果视频有效则打开视频编码器//////////////////////
    if (have_video)
        open_video(oc, video_codec, &video_st);

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st);

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            return;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return;
    }
    this->start();
}

av_infor SaveVideoFileThread::infor() const
{
    return m_infor;
}

void SaveVideoFileThread::add_stream(OutputStream *ost, AVFormatContext *oc,
                                     AVCodec **codec,
                                     enum AVCodecID codec_id)
{
    AVCodecContext *c;
    //找到我们指定的编码器
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }
    //对文件上下文oc添加流并使用传入的ost中的st保存
    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    //保存流对应的流索引
    ost->st->id = oc->nb_streams-1;
    //分配对应编码器的上下文
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ///////开始对编码器进行初始化///////////
    //保存此上下文
    ost->enc = c;
    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (int i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (int i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = { 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = m_infor.videoBiteRate;
        /* Resolution must be a multiple of two. */
        c->width    = m_infor.w;
        c->height   = m_infor.h;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->time_base       = ost->st->time_base;

        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}
void SaveVideoFileThread::open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    //设置编码器打开的选项
    av_dict_set(&opt, "preset", "superfast", 0);
    av_dict_set(&opt, "tune", "zerolatency", 0);  //实现实时编码
    c->thread_count = 4;
    //打开编码器
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: \n");
        exit(1);
    }

    //初始化转储帧空间
    ost->frame = av_frame_alloc();
    ost->frame->format = c->pix_fmt;
    ost->frame->width  = c->width;
    ost->frame->height = c->height;
    int numBytes_yuv = avpicture_get_size(AV_PIX_FMT_YUV420P, c->width,c->height);
    uint8_t * out_buffer_yuv = (uint8_t *) av_malloc(numBytes_yuv * sizeof(uint8_t));
    avpicture_fill((AVPicture *) ost->frame, out_buffer_yuv, AV_PIX_FMT_YUV420P,
                   c->width, c->height);
    ost->frameBuffer = out_buffer_yuv;
    ost->frameBufferSize = numBytes_yuv;

    //将编码器中的的编码ID，采样率，比特率帧率等信息复制到视频流的参数中
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
}
void SaveVideoFileThread::open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;

    c = ost->enc;

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec\n");
        exit(1);
    }

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame = av_frame_alloc();
    if (!ost->frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    ost->frame->format = c->sample_fmt;
    ost->frame->channel_layout = c->channel_layout;
    ost->frame->sample_rate = c->sample_rate;
    ost->frame->nb_samples = nb_samples;

    //申请空间
    mAudioOneFrameSize = av_samples_get_buffer_size(NULL, c->channels,c->frame_size, c->sample_fmt, 1);
    qDebug() << "mAudioOneFrameSize:" << mAudioOneFrameSize;
    ost->frameBuffer = (uint8_t *)av_malloc(mAudioOneFrameSize);
    ost->frameBufferSize = mAudioOneFrameSize;
    ///这句话必须要(设置这个 frame 里面的采样点个数)
    int oneChannelBufferSize = mAudioOneFrameSize / c->channels; //计算出一个声道的数据
    int nb_samplesize = oneChannelBufferSize /
                        av_get_bytes_per_sample(c->sample_fmt); //计算出采样点个数
    ost->frame->nb_samples = nb_samplesize;
    ///这 2 种方式都可以
    // avcodec_fill_audio_frame(ost->frame, c->channels, c->sample_fmt,(constuint8_t*)ost->frameBuffer, mAudioOneFrameSize, 0);
    av_samples_fill_arrays(ost->frame->data, ost->frame->linesize,
                           ost->frameBuffer, c->channels, ost->frame->nb_samples, c->sample_fmt, 0);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

}
void SaveVideoFileThread::close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_free(ost->frameBuffer);
    //av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}
int SaveVideoFileThread::write_frame(AVFormatContext *fmt_ctx, AVCodecContext*c,
                                     AVStream *st, AVFrame *frame ,int64_t & pts ,
                                     OutputStream *ost )
{
    int ret;
    if( !frame ) return 0;
    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame to the encoder\n" );
        return 0 ;
    }
    while (ret >= 0) {
        AVPacket pkt = { 0 };
        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding a frame\n");
            return 0 ;
        }
        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
        pkt.stream_index = st->index;
        //todo 修改部分 下一帧数据时间戳 pts 单位 ms
        pts = av_rescale_q( ost->next_pts, st->time_base, {1, 1000});
        /* Write the compressed frame to the media file. */
        ret = av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet\n");
            return 0 ;
        }
    }
    return ret == AVERROR_EOF ? 1 : 0;
}
void SaveVideoFileThread::videoDataQuene_Input(uint8_t * buffer, int size, int64_t
                                                                              time)
{
    BufferDataNode * node = (BufferDataNode*)malloc(sizeof(BufferDataNode));
    node->bufferSize = size;
    node->time = time;
    node->buffer = buffer;
    m_videoMutex.lock();
    m_videoDataList.append(node);
    m_videoMutex.unlock();
}
BufferDataNode *SaveVideoFileThread::videoDataQuene_get(int64_t time)
{
    BufferDataNode * node = NULL;
    m_videoMutex.lock();
    while(m_videoDataList.size()!= 0 )
    {
        node = m_videoDataList.takeFirst(); //取出元素并在链表移除该点
        if( m_videoDataList.size() == 0) break;
        if( time > node->time ) //淘汰掉小于下一帧时间的数据
        {
            av_free(node->buffer);
            node->buffer = NULL;
            free(node);
            node = NULL;
        }else
        {
            break;
        }
    }
    m_videoMutex.unlock();
    return node;
}
void SaveVideoFileThread::audioDataQuene_Input(const uint8_t *buffer, const int &size)
{
    BufferDataNode *node = (BufferDataNode*)malloc(sizeof(BufferDataNode));
    node->bufferSize = size;
    node->buffer = (uint8_t*)buffer;
    m_audioMutex.lock();
    m_audioDataList.append(node);
    m_audioMutex.unlock();
}
BufferDataNode * SaveVideoFileThread::audioDataQuene_get()
{
    BufferDataNode *node = NULL;
    m_audioMutex.lock();
    if (m_audioDataList.size() != 0 ){
        node = m_audioDataList.takeFirst();
    }
    m_audioMutex.unlock();
    return node;
}
void SaveVideoFileThread::run()
{
    while(1)
    {
        /* select the stream to encode */
        if ( have_video &&
            ( !have_audio|| (av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                           audio_st.next_pts, audio_st.enc->time_base) <= 0)) )
        {
            if (!write_video_frame(oc, &video_st, video_pts))
                msleep(1);
        }
        else
        {
            if (!write_audio_frame(oc, &audio_st))
                msleep(1);
        }
        if(isStop)
            if( ( have_audio && ( m_audioDataList.size() == 0 ) )|| ( have_video && ( m_videoDataList.size() == 0) ))
                break;
    }
    while(1){
        if ( isStop )
        {
            if( ( have_audio && ( m_audioDataList.size() == 0 ) )
                || ( have_video && ( m_videoDataList.size() == 0) ))
            {
                if( m_audioDataList.size() != 0 )
                {
                    write_audio_frame(oc, &audio_st);
                }else if( m_videoDataList.size() != 0 )
                {
                    write_video_frame(oc, &video_st, video_pts);
                }
            }
            if( m_audioDataList.size() == 0 && m_videoDataList.size() == 0)
                break;
        }
    }
    //回收的代码从 slot_closeVideo() 函数移植过来
    /* Write the trailer, if any. The trailer must be written before you
 * close the CodecContexts open when you wrote the header; otherwise
 * av_write_trailer() may try to use memory that was freed on
 * av_codec_close(). */
    av_write_trailer(oc);
    /* Close each codec. */
    if ( m_infor.have_camera || m_infor.have_screen )
        close_stream(oc, &video_st);
    if ( m_infor.have_audio)
        close_stream(oc, &audio_st);
    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);
    /* free the stream */
    avformat_free_context(oc);

}
bool SaveVideoFileThread::write_video_frame(AVFormatContext *oc, OutputStream *ost,
                                            double time)
{
    qDebug() << __func__;
    /// 稳帧的处理, 首先根据下一帧的时间, 获取大于下一帧时间的数据
    /// 如果没有就播放上一画面
    BufferDataNode *node = videoDataQuene_get(time);
    if (node == NULL)
    {
        if( lastVideoNode )
            node = lastVideoNode ;
        else
            return false;
    }
    else
    {
        if (node != lastVideoNode)
        {
            if (lastVideoNode != NULL)
            {
                av_free(lastVideoNode->buffer);
                free(lastVideoNode);
            }
            lastVideoNode = node;
        }
    }
    if( !node ) return false;
    memcpy(ost->frameBuffer, node->buffer, node->bufferSize);
    ost->frame->pts = ost->next_pts++;
    return write_frame( oc , ost->enc , video_st.st , ost->frame , video_pts , ost);
}
bool SaveVideoFileThread::write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    qDebug() << __func__;
    AVCodecContext *c;
    c = audio_st.enc;
    //从队列里面获取
    BufferDataNode* node = NULL;
    node = audioDataQuene_get();
    if ( node )
    {
        memcpy(ost->frameBuffer, node->buffer, node->bufferSize);
        free(node->buffer);
        node->buffer = NULL;
        free(node);
        node = NULL;
    }
    else
    {
        return false;
    }
    audio_st.frame->pts = audio_st.next_pts;
    audio_st.next_pts += audio_st.frame->nb_samples;
    AVRational rational;
    rational.num = 1;
    rational.den = c->sample_rate;
    audio_st.frame->pts = av_rescale_q(audio_st.samples_count, rational, c->time_base);
    audio_st.samples_count += audio_st.frame->nb_samples;
    return write_frame(oc, audio_st.enc, audio_st.st, audio_st.frame ,audio_pts , ost);
}
