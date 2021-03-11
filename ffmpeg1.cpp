#include<iostream>

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
//#include <libavcodec/avcodec.h>
};

struct buffer_data {
    uint8_t *ptr;
    size_t size;
};
static int read_packet(void *opaque,uint8_t *buf,int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size,bd->size);

    if(!buf_size)
        return AVERROR_EOF;
    printf("ptr:%p size:%zu\n",bd->ptr, bd->size);

    memcpy(buf,bd->ptr,buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;

    return buf_size;
}
int main(int argc,char **argv)
{
    AVOutputFormat *av_out_format = NULL;
    AVFormatContext *av_iformat_ctx = NULL, *av_oformat_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    AVPacket pkt;
    struct buffer_data bd = { 0 };
    const std::string in_url = "rtsp://admin:Wootion123456@192.168.100.79";
    const std::string out_filename = "recieve.mp4";

    //av_register_all();

    avformat_network_init();

    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
    int ret,videoindex ;
    ret = 0;
    videoindex = -1;

    ret = av_file_map(in_url.c_str(),&buffer,&buffer_size,0,NULL);
    
    if(ret < 0)
    {
        goto end;
    }

    bd.ptr = buffer;
    bd.size = buffer_size;
    
    if(!(av_iformat_ctx = avformat_alloc_context())){
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);
    if(!avio_ctx_buffer){
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx = avio_alloc_context(avio_ctx_buffer,avio_ctx_buffer_size,
                                 0,&bd,&read_packet,NULL,NULL);
    if(!avio_ctx)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    av_iformat_ctx->pb = avio_ctx;

    if(avformat_open_input(&av_iformat_ctx,NULL,NULL,NULL) !=0 )
    {
        std::cerr<<"can't open input stream!"<<std::endl;
		goto end;
    }

    if(avformat_find_stream_info(av_iformat_ctx,NULL) < 0)
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
    
    
end:
    avformat_close_input(&av_iformat_ctx);

    if(avio_ctx)
    {
        av_freep(&avio_ctx->buffer);
    }
    avio_context_free(&avio_ctx);
    av_file_unmap(buffer,buffer_size);

    if(ret < 0){
        char error_str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret,error_str,AV_ERROR_MAX_STRING_SIZE);
        std::cerr<<"Error occurred:"<<error_str<<std::endl;
        return 1;
    }
    std::cerr<<"test++++++++++++++++"<<std::endl;
    return 0;

}
