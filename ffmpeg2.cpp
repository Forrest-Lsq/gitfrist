#include<iostream>
#include<thread>
#include<atomic>
#include<unistd.h>

#ifndef INT64_C 
#define INT64_C(c) (c ## LL) 
#define UINT64_C(c) (c ## ULL) 
#endif 

extern "C" {
/*Include ffmpeg header file*/
#include <libavformat/avformat.h> 
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libavutil/mathematics.h> 
#include <libavutil/time.h> 
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
};

std::atomic<bool> time_flag;

void DurationTime()
{
    sleep(3);
    time_flag = true;
    return;
}

static void log_packet(const AVFormatContext *ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &ctx->streams[pkt->stream_index]->time_base;
    char str_[AV_TS_MAX_STRING_SIZE] = {0};
    printf("%s:pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",tag,
            av_ts_make_string(str_,pkt->pts),av_ts_make_time_string(str_,pkt->pts,time_base),
            av_ts_make_string(str_,pkt->dts),av_ts_make_time_string(str_,pkt->dts,time_base),
            av_ts_make_string(str_,pkt->duration),av_ts_make_time_string(str_,pkt->duration,time_base),
            pkt->stream_index);
}
int main(int argc,char **argv)
{
    AVOutputFormat *av_out_format = NULL;
    AVFormatContext *av_iformat_ctx = NULL, *av_oformat_ctx = NULL;
    AVPacket pkt;
  
    const std::string in_url = "rtsp://admin:Wootion123456@192.168.100.79";
    const std::string out_filename = "recieve.mp4";

    AVDictionary* options = NULL;
    av_dict_set(&options, "buffer_size", "1024000", 0);
    av_dict_set(&options, "max_delay", "5000", 0);
    av_dict_set(&options, "stimeout", "20000000", 0);  //设置超时断开连接时间
    av_dict_set(&options, "rtsp_transport", "tcp", 0);  //以udp方式打开，如果以tcp方式打开将udp替换为tcp 
    av_dict_set(&options, "rtbufsize", "10000", 0);

    std::thread *time_thread_;
    time_flag = false;
    //av_register_all();

    avformat_network_init();

    int ret = 0,videoindex = -1 , i;
    if(avformat_open_input(&av_iformat_ctx,in_url.c_str(),NULL,&options) !=0 )
    {
        std::cerr<<"can't open input stream!"<<std::endl;
		goto end;
    }

    if(avformat_find_stream_info(av_iformat_ctx,&options) < 0)
    {
        std::cerr<<"can't find input stream information"<<std::endl;
        goto end;
    }

    videoindex = av_find_best_stream(av_iformat_ctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    if(videoindex < 0)
    {
        std::cerr<<"can't find a video stream in the input file\n"<<std::endl;
        goto end;
    }

    std::cout<<"*********************input information*************************************"<<std::endl;
	av_dump_format(av_iformat_ctx,0,in_url.c_str(),0);
	std::cout<<"**************************************************************************"<<std::endl;
    
    avformat_alloc_output_context2(&av_oformat_ctx,NULL,NULL,out_filename.c_str());
    if(!av_oformat_ctx)
    {
        std::cerr<<"Could not Greate output context\n"<<std::endl;
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    av_out_format = av_oformat_ctx->oformat;
    for(i = 0; i < av_iformat_ctx->nb_streams;i++)
    {
        AVStream *in_stream = av_iformat_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(av_oformat_ctx,NULL);
        AVCodecParameters *in_codecpar = in_stream->codecpar;
        if(!out_stream)
        {
            std::cerr<<"Failed allocating output stram\n"<<std::endl;
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar,in_codecpar);
        if(ret < 0)
        {
            std::cerr<<"Failed to copy codec parameters"<<std::endl;
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    std::cout<<"********************output information*************************************"<<std::endl;
    av_dump_format(av_oformat_ctx,0,out_filename.c_str(),1);
    std::cout<<"****************************************************************************"<<std::endl;

    if(!(av_oformat_ctx->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&av_oformat_ctx->pb, out_filename.c_str(),AVIO_FLAG_WRITE);
        if(ret < 0)
        {
            std::cerr<<"Could not open output file:"<<out_filename<<std::endl;
            goto end;
        }
    }

    ret = avformat_write_header(av_oformat_ctx,&options);
    if(ret < 0)
    {
        std::cerr<<"Error occurred when opening output file"<<std::endl;
        goto end;
    }
    
    time_thread_ = new std::thread(DurationTime);
    videoindex = 0;
    while (!time_flag) {
        AVStream *in_stream ,*out_stream;

        ret = av_read_frame(av_iformat_ctx,&pkt);
        if(ret < 0)
            break;
        in_stream = av_iformat_ctx->streams[pkt.stream_index];
        if(in_stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
        {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = 0;
        out_stream = av_oformat_ctx->streams[pkt.stream_index];
        log_packet(av_iformat_ctx,&pkt,"in");

        pkt.pts =  av_rescale_q_rnd(pkt.pts,in_stream->time_base,out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts =  av_rescale_q_rnd(pkt.dts,in_stream->time_base,out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration,in_stream->time_base,out_stream->time_base);
        pkt.pos = -1;

        log_packet(av_oformat_ctx,&pkt,"out");

        if(pkt.pts > 3600 && videoindex == 0){
            videoindex = 1;
            continue;
        }

        ret = av_interleaved_write_frame(av_oformat_ctx,&pkt);
        if(ret < 0 )
        {
            std::cerr<<"Rrror muxing packet"<<std::endl;
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(av_oformat_ctx);

end:
    avformat_close_input(&av_iformat_ctx);
    
    if(av_oformat_ctx && !(av_oformat_ctx->flags & AVFMT_NOFILE)){
        avio_close(av_oformat_ctx->pb);
    }

    avformat_free_context(av_oformat_ctx);
    time_thread_->join();
    delete time_thread_;
    if(ret < 0){
        char error_str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret,error_str,AV_ERROR_MAX_STRING_SIZE);
        std::cerr<<"Error occurred:"<<error_str<<std::endl;
        return 1;
    }
    return 0;

}
