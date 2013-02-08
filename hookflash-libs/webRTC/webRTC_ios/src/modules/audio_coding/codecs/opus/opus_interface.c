
#include "opus_interface.h"
#include "opus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_FRAME_SIZE 960*6
#define MAX_PACKET 1275

short readbuffer[MAX_FRAME_SIZE];
short writebuffer[MAX_FRAME_SIZE];

typedef struct OPUS_Enc_Inst_t_ {
  OpusEncoder *enc;

} OPUS_Enc_Inst_t;

typedef struct OPUS_Dec_Inst_t_ {
  OpusDecoder *dec;
  
} OPUS_Dec_Inst_t;

WebRtc_Word16 WebRtcOpus_CreateEnc(OPUS_encinst_t** inst, WebRtc_Word16 samplFreq) {

  *inst=(OPUS_encinst_t*)malloc(sizeof(OPUS_Enc_Inst_t));
  if (*inst==NULL) {
    return(-1);
  }
  
  int err;
  
  ((OPUS_Enc_Inst_t*)*inst)->enc = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, &err);
  if(err != OPUS_OK || ((OPUS_Enc_Inst_t*)*inst)->enc==NULL) {
    return(-1);
  }

  return(0);
}

WebRtc_Word16 WebRtcOpus_FreeEnc(OPUS_encinst_t *inst){

  opus_encoder_destroy(((OPUS_Enc_Inst_t*)inst)->enc);
  free(inst);
  return(0);
}

WebRtc_Word16 WebRtcOpus_CreateDec(OPUS_decinst_t** inst, WebRtc_Word16 samplFreq){
  
  *inst=(OPUS_decinst_t*)malloc(sizeof(OPUS_Dec_Inst_t));
  if (*inst==NULL) {
    return(-1);
  }
  
  int err;
  
  ((OPUS_Dec_Inst_t*)*inst)->dec = opus_decoder_create(16000, 1, &err);
  if(err != OPUS_OK || ((OPUS_Dec_Inst_t*)*inst)->dec==NULL) {
    return (-1);
  }
  
  return(0);
}

WebRtc_Word16 WebRtcOpus_FreeDec(OPUS_decinst_t* inst){

  opus_decoder_destroy(((OPUS_Dec_Inst_t*)inst)->dec);
  return (0);
}

WebRtc_Word16 WebRtcOpus_Encode(OPUS_encinst_t* encInst,
                                WebRtc_Word16* input,
                                WebRtc_Word16* output,
                                WebRtc_Word16 len,
                                WebRtc_Word16 byteLen){
  
  return opus_encode(((OPUS_Enc_Inst_t*)encInst)->enc, (short*)input, len, (unsigned char*)output, MAX_PACKET);
}

WebRtc_Word16 WebRtcOpus_EncoderInit(OPUS_encinst_t* encInst,
                                     WebRtc_Word16 samplFreq,
                                     WebRtc_Word16 mode,
                                     WebRtc_Word16 vbrFlag){

  opus_encoder_ctl(((OPUS_Enc_Inst_t*)encInst)->enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
  
  return (0);
}

WebRtc_Word16 WebRtcOpus_Decode(OPUS_decinst_t *decInst,
                                WebRtc_UWord8 *encoded, WebRtc_Word16 len,
                                WebRtc_Word16 *decoded, WebRtc_Word16 *speechType){
  return opus_decode(((OPUS_Dec_Inst_t*)decInst)->dec, encoded, len, decoded, MAX_FRAME_SIZE, 0);;
}

WebRtc_Word16 WebRtcOpus_DecoderInit(OPUS_decinst_t *decInst){
  return (0);
}

void WebRtcOpus_Version(char *version)
{
  strcpy((char*)version, "1.0.1");
}
