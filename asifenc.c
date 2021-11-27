/*
 * ASIF audio encoder
 * Authors: Daniel Detwiller
 * Version: 0.0.1
 * Date: 4/21/2020
 */

#define BITSTREAM_WRITER_LE
 
#include "libavutil/intreadwrite.h"
#include "libavutil/opt.h"
#include "avcodec.h"
#include "internal.h"
#include "put_bits.h"
#include "bytestream.h"

/*
 * Private data struct to store samples from frame's extended data.
 */
typedef struct asif_encoder_data
{
  void *data;
  int last_frame_received;
  int number_of_samples;
  int number_of_channels;
  
  uint8_t *buffer[16];
  int buf_count[16];

  int number_of_bytes;
  int bytes_used;
  int flag_endoffile;

}asif_encoder_data;

/*
 * Initializes the private asif_encoder_data and the frame size.
 */
static av_cold int asif_encode_init(AVCodecContext *avctx)
{
  // Initialize the struct data.
  asif_encoder_data *my_data = avctx->priv_data;
  
  for (int i = 0; i < 16; i++)
    my_data->buf_count[i] = 0;

  my_data->flag_endoffile = 0;
  my_data->last_frame_received = 0;
  my_data->bytes_used = 0;
  my_data->number_of_bytes = 8000;
  my_data->number_of_samples = 0;
  
  for(int i = 0; i < avctx->channels; i++)
    my_data->buffer[i] = malloc(8000*sizeof(uint8_t));  

  avctx->frame_size = 8000;
  return 0;
}
/*
 * Sends collects sample data from every frame for use by receive_packet.
 */
static int asif_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{
  uint8_t *buf;
  asif_encoder_data *my_data =  avctx->priv_data;
  
  if(frame == NULL)
  {
    my_data->last_frame_received = 1;
  }
  else
    {
      my_data->number_of_samples += frame->nb_samples;
      my_data->bytes_used = my_data->bytes_used + frame->nb_samples;

      //check to see if new memory needs to be allocated
      while(my_data->number_of_bytes < my_data->bytes_used)
	{
	  for(int i = 0; i < 16; i++)
	    my_data->buffer[i] = realloc(my_data->buffer[i], my_data->number_of_bytes * 2);
      
	  // Double number of bytes when reallocating.
	  my_data->number_of_bytes *= 2;
	}
    //get all sample data from frames and store them in
    //our asif_encoder_data
    for(int i = 0; i < avctx->channels;i++)
    {
      buf = my_data->buffer[i] + my_data->buf_count[i];
      for (int j = 0; j < frame->nb_samples ; j++)
      {
	bytestream_put_byte(&buf, frame->extended_data[i][j]);
	my_data->buf_count[i]++;
      }
    }
  }
  
  return 0;
}

/*
 *converts data into asif form and stores it in one large packet.
 */
static int asif_receive_packet(AVCodecContext *avctx, AVPacket *avpkt)
{
  int n;
  asif_encoder_data *my_data =  avctx->priv_data;

  if(my_data->flag_endoffile == 1)
    return AVERROR_EOF;

  if(!my_data->last_frame_received)
    return AVERROR(EAGAIN);
  
  uint8_t *ret, *buf;
  //Find how much memory needs to be allocated
  n = my_data->number_of_samples * avctx->channels;
  
  if ((ret = ff_alloc_packet2(avctx, avpkt, n+14, 0)) < 0)
      return ret;
  
  buf = avpkt->data;
  
  // Header, first 14 bytes.
  bytestream_put_buffer(&buf, "asif", 4);
  bytestream_put_le32(&buf, avctx->sample_rate);
  bytestream_put_le16(&buf, avctx->channels);
  bytestream_put_le32(&buf, my_data->number_of_samples);

  // The next 14+ bytes
  for (int c = 0; c < avctx->channels; c++)
  {
    int catchup_flag, catchup_value = 0;
    
    // Get the first sample and put it on the buffer.
    uint8_t *sample;
    sample = my_data->buffer[c]; 
    bytestream_put_byte(&buf, *sample);
    
    // Get all of the samples and calculate the deltas and
    // put them on the buffer.
    for(int i = 0; i < my_data->number_of_samples-1; i++)
    {
      uint8_t prev_sample = *sample;
      sample++;
    
      int delta = *sample - prev_sample;
      //if there is overflow from last sample, add it to current sample
      if(catchup_flag == 1)
      {
	  catchup_flag = 0;
	  if(prev_sample == 127)
	    delta += catchup_value;
	  if(prev_sample == -128)
	    delta -= catchup_value;
      }
  
      //Check for overflow
      if (delta > 127)
      {
	catchup_flag = 1;
	catchup_value = delta-127;
	delta = 127;
      }
      else if (delta < -128)
      {
	catchup_flag = 1;
	catchup_value = delta + 128;
	delta = -128;
      }
      
      bytestream_put_byte(&buf,(uint8_t) delta);
    }
  }

  my_data->flag_endoffile = 1;
 
  return 0;
}

/*
 * Ends the encoder.
 */
static av_cold int asif_encode_close(AVCodecContext *avctx)
{
  return 0;
}

AVCodec ff_asif_encoder = {
    .name           = "asif",
    .priv_data_size = sizeof(asif_encoder_data),
    .long_name      = NULL_IF_CONFIG_SMALL("Our ASIF Encoder."),
    .type           = AVMEDIA_TYPE_AUDIO,
    .id             = AV_CODEC_ID_ASIF,
    .init           = asif_encode_init,
    .encode2        = NULL,
    .send_frame     = asif_send_frame,
    .receive_packet = asif_receive_packet,
    .close          = asif_encode_close,
    .capabilities   = AV_CODEC_CAP_SMALL_LAST_FRAME | AV_CODEC_CAP_DELAY,
    .sample_fmts    = (const enum AVSampleFormat[]){ AV_SAMPLE_FMT_U8P,
                                                     AV_SAMPLE_FMT_NONE },
};
