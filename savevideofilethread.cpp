#include "savevideofilethread.h"

SaveVideoFileThread::SaveVideoFileThread(QObject *parent)
    : QThread{parent},m_infor()
{
    m_PicInPic_Reader=new PicInPic_Read();
    connect(this,&SaveVideoFileThread::SIG_open, m_PicInPic_Reader,&PicInPic_Read::slot_openVideo);
    connect(this,&SaveVideoFileThread::SIG_close,m_PicInPic_Reader,&PicInPic_Read::slot_closeVideo);
    connect(m_PicInPic_Reader,&PicInPic_Read::SIG_sendVideoFrame,this,&SaveVideoFileThread::SIG_sendVideoFrame);
    connect(m_PicInPic_Reader,&PicInPic_Read::SIG_sendPicInPic,this,&SaveVideoFileThread::SIG_sendPicInPic);
    connect(m_PicInPic_Reader,&PicInPic_Read::SIG_sendVideoFrameData,this,&SaveVideoFileThread::SLO_saveVideoFrameData);
}

void SaveVideoFileThread::slot_openVideo()
{
    emit SIG_open();
}

void SaveVideoFileThread::slot_closeVideo()
{
    //启动读取画面与音频
    emit SIG_close();

    av_write_trailer(oc);

    /* Close each codec. */
    if (m_infor.have_screen|m_infor.have_camera)
        close_stream(oc, &video_st);
    if (m_infor.have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);
}

void SaveVideoFileThread::SLO_saveVideoFrameData(uint8_t *picture_buf, int buffer_size)
{
    //AVCodecContext *c =video_st.enc;
    memcpy(video_st.frameBuffer,picture_buf,buffer_size);
    av_free(picture_buf);
    video_st.frame->pts=video_st.next_pts++;
    write_frame(oc, video_st.enc, video_st.st, video_st.frame);
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
    // if (m_infor.have_audio) {
    //     add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
    //     have_audio = 1;
    //     encode_audio = 1;
    // }
    /////////////////如果视频有效则打开视频编码器//////////////////////
    if (have_video)
        open_video(oc, video_codec, &video_st);

    // if (have_audio)
    //     open_audio(oc, audio_codec, &audio_st);

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
    //保存此编码器id
    c->codec_id = codec_id;
    c->bit_rate = m_infor.videoBiteRate;
    /* Resolution must be a multiple of two. */
    c->width    = m_infor.w;
    c->height   = m_infor.h;

    /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
    //设置此流的timebase时间单位
    ost->st->time_base = { 1, m_infor.frame_rate };
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
void SaveVideoFileThread::close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_free(ost->frameBuffer);
    //av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}
int SaveVideoFileThread::write_frame(AVFormatContext *fmt_ctx,AVCodecContext *c, AVStream *st,AVFrame* frame )
{
    int ret;
    if(!frame)return 0;
    ret = avcodec_send_frame(c,frame);
    if(ret<0){
        fprintf(stderr,"Error sending a frame to the enoder: \n");
        return 0;
    }
    while(ret>=0){
        AVPacket pkt={0};
        ret=avcodec_receive_packet(c,&pkt);
        if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF)
            break;
        else if(ret<0){
            fprintf(stderr,"Error encoding a frame\n");
            exit(1);
        }
        av_packet_rescale_ts(&pkt,c->time_base,st->time_base);
        pkt.stream_index=st->index;
        ret = av_interleaved_write_frame(fmt_ctx,&pkt);
        av_packet_unref(&pkt);
        if(ret<0){
            fprintf(stderr,"Error while writing output packet\n");
            exit(1);
        }
    }
    return ret==AVERROR_EOF?1:0;
}
