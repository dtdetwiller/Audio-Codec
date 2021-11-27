/*
 * ASIF demuxer
 * Authors: Daniel Detwiller & Clarence Park
 * Version: 0.0.1
 * Date: 4/21/2020
 */

#include "avformat.h"
#include "internal.h"
#include "pcm.h"
#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "libavutil/avassert.h"
#include "avio_internal.h"
#include "rawdec.h"

int asif_read_header(AVFormatContext *s);

int asif_read_packet(AVFormatContext *s, AVPacket *pkt);

/*
 * The asif read probe method.
 */
static int asif_read_probe(const AVProbeData *p)
{
  if(!memcmp(p->buf, "asif", 4))
    return AVPROBE_SCORE_MAX;
  
    return 0;
}

/*
 * The asif read header method.
 */
int asif_read_header(AVFormatContext *s)
{
  AVStream *st = avformat_new_stream(s, NULL);

  if(!st)
    return AVERROR(ENOMEM);

  st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
  st->codecpar->codec_id = AV_CODEC_ID_ASIF;
  
  st->start_time = 0;

  return 0;
}

/*
 * The asif read packet method.
 */
int asif_read_packet(AVFormatContext *s, AVPacket *pkt)
{
  int ret, size;
  size = avio_size(s->pb);

  if((ret=av_new_packet(pkt, size)) < 0)
  {
    av_log(NULL, AV_LOG_INFO, "     <--- asifdemux.c size <0    \n");
    return 1;
  }
  
  pkt-> pos= avio_tell(s->pb);
  pkt->stream_index = 0;
  
  ret = avio_read(s->pb, pkt->data, size);
  if(ret <0)
  {
    av_log(NULL, AV_LOG_INFO, "     <--- asifdemux.c ret <0    \n");
    av_packet_unref(pkt);
    return ret;
  }

  return 0;
}

AVInputFormat ff_asif_demuxer = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("The ASIF Demuxer."),
    .priv_data_size = 0,
    .read_header    = asif_read_header,
    .read_packet    = asif_read_packet,
    .read_probe      = asif_read_probe,
    .extensions     = "asif",
    .raw_codec_id   = AV_CODEC_ID_ASIF,
};
