#include<iostream>

#ifndef INT64_C 
#define INT64_C(c) (c ## LL) 
#define UINT64_C(c) (c ## ULL) 
#endif 

extern "C" {
/*Include ffmpeg header file*/
#include <libavformat/avformat.h> 
#include <libavcodec/avcodec.h> 
#include <libswscale/swscale.h> 

};

int main()
{
	AVFormatContext *p_format_context_;//描述媒体文件或媒体流构成的基本信息。

	AVCodecContext *p_avcode_context_;//描述了编解码器上下文信息的结构体。

	AVCodec *p_avcodec_;//每种音频或视屏对应的编解码器结构体。

	AVFrame *p_avframe_,*p_avframe_rtsp_;//存储解码后的像素数据。

	AVPacket *p_avpacket_;//存储压缩编码后的数据。
	
	struct SwsContext *s_wscontext_;//视频像素转换结构体。

	std::string input_url = "rtsp://admin:Wootion123456@192.168.100.79";

	av_register_all();//注册所有组件

	avformat_network_init();//打开网络视频流

	p_format_context_ = avformat_alloc_context();

	if(avformat_open_input(&p_format_context_,input_url.c_str(),NULL,NULL)!=0)//读取文件头部把信息保存到AVFormatContext结构体
	{
		std::cerr<<"can't open input stream!"<<std::endl;
		return -1;
	}

	if(avformat_find_stream_info(p_format_context_,NULL)<0)//为pFormatCtx->streams填充上正确的信息
	{
		std::cerr<<"can't find stream information！"<<std::endl;
		return -1;
	}

	int videoindex = -1;
	for(int i = 0;i < p_format_context_->nb_streams;i++)
	{
		if(p_format_context_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;//找到视频的数组位置
			break;
		}
	}
	if(videoindex == -1){
		std::cerr<<"Didn't find video stream!\n"<<std::endl;
		return -1;
	}

	p_avcode_context_ = p_format_context_->streams[videoindex]->codec;
	p_avcodec_ = avcodec_find_decoder(p_avcode_context_->codec_id);//查找解码器
	if(p_avcodec_ == NULL)
	{
		std::cerr<<"Didn't find codec"<<std::endl;
		return -1;
	}

	if(avcodec_open2(p_avcode_context_,p_avcodec_,NULL)!=0)
	{
		std::cerr<<"open avcodec failed!"<<std::endl;
		return -1;
	}
	std::cout<<"视频时长："<<p_format_context_->duration<<std::endl;


	p_avframe_ = av_frame_alloc();
	p_avframe_rtsp_ = av_frame_alloc();
	uint8_t *out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUVJ420P,\
											p_avcode_context_->width,p_avcode_context_->height));
	avpicture_fill((AVPicture *)p_avframe_rtsp_,out_buffer,AV_PIX_FMT_YUVJ420P,\
											p_avcode_context_->width,p_avcode_context_->height);
	p_avpacket_ = (AVPacket *)av_malloc(sizeof(AVPacket));

	std::cout<<"*********************File information*************************************"<<std::endl;
	av_dump_format(p_format_context_,0,input_url.c_str(),0);
	std::cout<<"**************************************************************************"<<std::endl;

	s_wscontext_ = sws_getContext(p_avcode_context_->width,p_avcode_context_->height,p_avcode_context_->pix_fmt,\
				   p_avcode_context_->width,p_avcode_context_->height,AV_PIX_FMT_YUVJ420P,SWS_BICUBIC,NULL,NULL,NULL);
	
	int frame_cnt = 0;
	int got_picture;
	while(av_read_frame(p_format_context_,p_avpacket_) >= 0)
	{
		if(p_avpacket_->stream_index == videoindex)
		{
			int ret = avcodec_decode_video2(p_avcode_context_,p_avframe_,&got_picture,p_avpacket_);//解码一帧压缩数据
			if(ret < 0)
			{
				std::cerr<<"Decode error!"<<std::endl;
				return -1;
			}

			if(got_picture)
			{
				sws_scale(s_wscontext_,(const uint8_t* const*)p_avframe_->data,p_avframe_->linesize,\
										0,p_avcode_context_->height,p_avframe_rtsp_->data,p_avframe_rtsp_->linesize);
				std::cout<<"Decode frame index:"<<frame_cnt<<std::endl;
				
				frame_cnt++;
			}
		}
		av_free_packet(p_avpacket_);
		
	}

	sws_freeContext(s_wscontext_);

	av_free(out_buffer);
	av_frame_free(&p_avframe_);
	av_frame_free(&p_avframe_rtsp_);
	avcodec_close(p_avcode_context_);//关闭解码器
	avformat_close_input(&p_format_context_);//关闭输入视频文件或媒体流


}
