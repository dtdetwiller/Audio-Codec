/*
 * ASIF muxer
 * Authors: Daniel Detwiller & Clarence Park
 * Version: 0.0.1
 * Date: 4/21/2020
 */
#include "avformat.h"
#include "rawenc.h"

/*
 * Writes the packet data
 */
static int asif_write_packet(AVFormatContext *s, AVPacket *pkt)
{
  avio_write(s->pb, pkt->data, pkt->size);
  return 0;
}

AVOutputFormat ff_asif_muxer = {            
    .name         = "asif",                  
    .long_name    = NULL_IF_CONFIG_SMALL("The ASIF Muxer."),
    .extensions   = "asif",                             
    .audio_codec  = AV_CODEC_ID_ASIF,                           
    .video_codec  = AV_CODEC_ID_NONE,                
    .write_packet = asif_write_packet,             
    .flags        = AVFMT_NOTIMESTAMPS,              
};
