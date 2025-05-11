#ifndef SAVEVIDEOFILETHREAD_H
#define SAVEVIDEOFILETHREAD_H

#include <QThread>
#include "picinpic_read.h"
#include"audio_read.h"
#include<QMutex>

//#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE FRAME_RATE /* 15 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    uint8_t* frameBuffer;
    int frameBufferSize;
    /*暂未使用*/
    AVFrame *tmp_frame;
    float t, tincr, tincr2;
    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;


typedef struct av_infor{
    QString filename;
    bool have_camera;
    bool have_screen;
    bool have_audio;
    ////////////视频///////////////
    int w;
    int h;
    int frame_rate;
    int videoBiteRate;
    ////////////音频///////////////

    //构造函数
    av_infor():have_audio(true),have_camera(true),have_screen(true){
    }
}av_infor;

struct BufferDataNode
{
    uint8_t * buffer;
    int bufferSize;
    int64_t time; //视频帧用于稳帧, 比较时间
};

class SaveVideoFileThread : public QThread
{
    Q_OBJECT
public:
    explicit SaveVideoFileThread(QObject *parent = nullptr);
    av_infor infor() const;

    void run();
signals:
    void SIG_sendVideoFrame( QImage img ); // 用于预览
    void SIG_sendPicInPic( QImage img ); //用于显示画中画
    void SIG_open();
    void SIG_close();
public slots:
    void SLO_saveVideoFrameData( uint8_t* picture_buf, int buffer_size ); //采集的数据格式 YUV420P
    void SLO_setInfor(av_infor& infor);
    void slot_openVideo(); //开启采集
    void slot_closeVideo(); //关闭采集
private:
    PicInPic_Read* m_PicInPic_Reader;
    Audio_Read*    m_Audio_Reader;
    av_infor m_infor;
    OutputStream video_st = { 0 }, audio_st = { 0 };
    int ret;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;

    QList<BufferDataNode*> m_videoDataList;
    QList<BufferDataNode*> m_audioDataList;
    BufferDataNode* lastVideoNode;
    int mAudioOneFrameSize;
    bool m_videoBeginFlag;
    qint64 m_videoBeginTime;
    QMutex m_videoMutex;
    QMutex m_audioMutex;
    bool isStop;
    int64_t video_pts;
    int64_t audio_pts;
    void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, AVCodecID codec_id);
    void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost);
    void close_stream(AVFormatContext *oc, OutputStream *ost);
    int write_frame(AVFormatContext *fmt_ctx,AVCodecContext *c, AVStream *st,AVFrame* frame );
    void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost);
    void slot_saveAudioFrameData(uint8_t *picture_buf, int buffer_size);
    void videoDataQuene_Input(uint8_t *buffer, int size, int64_t time);
    BufferDataNode *videoDataQuene_get(int64_t time);
    void audioDataQuene_Input(const uint8_t *buffer, const int &size);
    BufferDataNode *audioDataQuene_get();
    int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame, int64_t &pts, OutputStream *ost);
    bool write_video_frame(AVFormatContext *oc, OutputStream *ost, double time);
    bool write_audio_frame(AVFormatContext *oc, OutputStream *ost);
};

#endif // SAVEVIDEOFILETHREAD_H
