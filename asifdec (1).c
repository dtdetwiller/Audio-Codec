/*
 * ASIF
 * Authors: Daniel Detwiller & Clarence Park
 * Version: 0.0.1
 * Date: 4/21/2020
 */

#include "libavutil/channel_layout.h"
#include "avcodec.h"
#include "bytestream.h"
#include "get_bits.h"
#include "internal.h"
#include "thread.h"
#include "unary.h"
#include "dsd.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/opt.h"
#include "put_bits.h"
/**
 * @file
 * ASIF audio decoder
 */

/*
 * This method decodes the packets and creates one large frame. 
 * 
 */
static int asif_decode(AVCodecContext *avctx, void *data,
		       int *got_frame_ptr, AVPacket *avpkt)
{
  const uint8_t *src = avpkt->data;
  int ret, c;
  int buf_size = avpkt->size;
  uint8_t *frame_data;
  AVFrame *frame = data;
  
  // Get the first 10 bytes (header).
  bytestream_get_le32(&src);
  avctx->sample_rate = bytestream_get_le32(&src); 
  avctx->channels = bytestream_get_le16(&src);
  frame->nb_samples = bytestream_get_le32(&src);
  
  // Set the sample format to U8P.
  avctx->sample_fmt = AV_SAMPLE_FMT_U8P;
  
  if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
    return ret;
  
  uint8_t *sample = src;

  // Go through all the samples in each channel and put the data in frame->extended_data.
  for(c = 0; c < avctx->channels; c++)
  {
    frame_data = frame->extended_data[c];
    bytestream_put_byte(&frame_data, *sample);
        
    uint8_t sample_value = *sample;
    sample++;

    for(int i = 0; i < frame->nb_samples - 1; i++)
    {
      // accounts for the deltas.
      sample_value += bytestream_get_byte(&sample);
      bytestream_put_byte(&frame_data, sample_value);
    }

  }
  
  // Set got_frame_ptr to 1 to indicate that this method is complete.
  *got_frame_ptr = 1;
  
  return buf_size;
}

AVCodec ff_asif_decoder = {
  .name           = "asif",
  .long_name      = NULL_IF_CONFIG_SMALL("ASIF Decoder"),
  .type           = AVMEDIA_TYPE_AUDIO,
  .id             = AV_CODEC_ID_ASIF,
  .decode         = asif_decode,
  .capabilities   = AV_CODEC_CAP_DR1,
  .sample_fmts    = (const enum AVSampleFormat[]){ AV_SAMPLE_FMT_U8P,
						   AV_SAMPLE_FMT_NONE },
};
