
#include "vorbis_interface.h"
#include "vorbis/vorbisenc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define READ 1024
signed char readbuffer[READ*4+44];

typedef struct VORBIS_Enc_Inst_t_ {
  ogg_stream_state os;
  
  ogg_page         og;
  ogg_packet       op;
  
  vorbis_info      vi;
  
  vorbis_comment   vc;
  
  vorbis_dsp_state vd;
  vorbis_block     vb;
  
  char             started;
} VORBIS_Enc_Inst_t;

WebRtc_Word16 WebRtcVorbis_CreateEnc(VORBIS_encinst_t **VORBISenc_inst,
                                     WebRtc_Word32 fs) {
  *VORBISenc_inst=(VORBIS_encinst_t*)malloc(sizeof(VORBIS_Enc_Inst_t));
  if (*VORBISenc_inst!=NULL) {
    return(0);
  } else {
    return(-1);
  }
}

WebRtc_Word16 WebRtcVorbis_FreeEnc(VORBIS_encinst_t *VORBISenc_inst){
  
  ogg_stream_clear(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os));
  vorbis_block_clear(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vb));
  vorbis_dsp_clear(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd));
  vorbis_comment_clear(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vc));
  vorbis_info_clear(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vi));

  free(VORBISenc_inst);
  return(0);
}

WebRtc_Word16 WebRtcVorbis_CreateDec(VORBIS_decinst_t **VORBISdec_inst,
                                     WebRtc_Word32 fs,
                                     WebRtc_Word16 enh_enabled){
  return 0;
}

WebRtc_Word16 WebRtcVorbis_FreeDec(VORBIS_decinst_t *VORBISdec_inst){
  return 0;
}

WebRtc_Word16 WebRtcVorbis_Encode(VORBIS_encinst_t *VORBISenc_inst,
                                  const WebRtc_Word16* speechIn, 
                                  WebRtc_Word16 len,
                                  WebRtc_UWord8* encoded){
    
  long encodedLen = 0;
  
  if (!((VORBIS_Enc_Inst_t*)VORBISenc_inst)->started && len != 0){

    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;
    
    vorbis_analysis_headerout(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vc),
                              &header,&header_comm,&header_code);
    ogg_stream_packetin(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os),&header);
    ogg_stream_packetin(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os),&header_comm);
    ogg_stream_packetin(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os),&header_code);
    
    while(1){
      int result=ogg_stream_flush(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og));
      if(result==0)break;
      memcpy(encoded+encodedLen,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).header,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).header_len);
      encodedLen += (((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).header_len;
      memcpy(encoded+encodedLen,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).body,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).body_len);
      encodedLen += (((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).body_len;
    }
    ((VORBIS_Enc_Inst_t*)VORBISenc_inst)->started = 1;
  }
  
  long i;
  long bytes=len*2;
  memcpy(readbuffer,speechIn,bytes);

  if(bytes==0){
    vorbis_analysis_wrote(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),0);
  }else{
    float **buffer=vorbis_analysis_buffer(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),READ);
    
    for(i=0;i<bytes/2;i++){
      buffer[0][i]=((readbuffer[i*2+1]<<8)|
                    (0x00ff&(int)readbuffer[i*2]))/32768.f;
    }

    vorbis_analysis_wrote(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),i);
  }
  
  while(vorbis_analysis_blockout(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vb))==1){
    
    vorbis_analysis(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vb),NULL);
    vorbis_bitrate_addblock(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vb));
    
    while(vorbis_bitrate_flushpacket(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->op))){
      
      ogg_stream_packetin(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->op));
      
      while(1){
        int result=ogg_stream_pageout(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og));
        if(result==0)break;
        memcpy(encoded+encodedLen,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).header,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).header_len);
        encodedLen += (((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).header_len;
        memcpy(encoded+encodedLen,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).body,(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).body_len);
        encodedLen += (((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og).body_len;
                
        if(ogg_page_eos(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->og))) goto end;
      }
    }
  }
end:
  return encodedLen;
}

WebRtc_Word16 WebRtcVorbis_EncoderInit(VORBIS_encinst_t *VORBISenc_inst){
  
  int ret;
  
  ((VORBIS_Enc_Inst_t*)VORBISenc_inst)->started = 0;
  
  vorbis_info_init(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vi));
  
  ret=vorbis_encode_init_vbr(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vi),1,16000,0.1);
  
  if(ret)
    return(-1);

  vorbis_comment_init(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vc));
  vorbis_comment_add_tag(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vc),"ENCODER","hookflash");
  
  vorbis_analysis_init(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vi));
  vorbis_block_init(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vd),&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->vb));
  
  srand(time(NULL));
  ogg_stream_init(&(((VORBIS_Enc_Inst_t*)VORBISenc_inst)->os),rand());

  return(0);
}

WebRtc_Word16 WebRtcVorbis_Decode(VORBIS_decinst_t *VORBISdec_inst,
                                  WebRtc_UWord8 *encoded, WebRtc_Word16 len,
                                  WebRtc_Word16 *decoded, WebRtc_Word16 *speechType){
  return 0;
}

WebRtc_Word16 WebRtcVorbis_DecoderInit(VORBIS_decinst_t *VORBISdec_inst){
  return 0;
}

void WebRtcVorbis_Version(char *version)
{
  strcpy((char*)version, "1.3.3");
}

WebRtc_Word16 WebRtcVorbis_Test()
{
  ogg_stream_state os; /* take physical pages, weld into a logical
                        stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */
  
  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
                        settings */
  vorbis_comment   vc; /* struct that stores all the user comments */
  
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */
  
  int eos=0,ret;
  int i, founddata;
  
  /* we cheat on the WAV header; we just bypass 44 bytes (simplest WAV
   header is 44 bytes) and assume that the data is 44.1khz, stereo, 16 bit
   little endian pcm samples. This is just an example, after all. */
  
  /* we cheat on the WAV header; we just bypass the header and never
   verify that it matches 16bit/stereo/44.1kHz.  This is just an
   example, after all. */
  
  readbuffer[0] = '\0';
  for (i=0, founddata=0; i<30 && ! feof(stdin) && ! ferror(stdin); i++)
  {
    fread(readbuffer,1,2,stdin);
    
    if ( ! strncmp((char*)readbuffer, "da", 2) ){
      founddata = 1;
      fread(readbuffer,1,6,stdin);
      break;
    }
  }
  
  /********** Encode setup ************/
  
  vorbis_info_init(&vi);
  
  /* choose an encoding mode.  A few possibilities commented out, one
   actually used: */
  
  /*********************************************************************
   Encoding using a VBR quality mode.  The usable range is -.1
   (lowest quality, smallest file) to 1. (highest quality, largest file).
   Example quality mode .4: 44kHz stereo coupled, roughly 128kbps VBR
   
   ret = vorbis_encode_init_vbr(&vi,2,44100,.4);
   
   ---------------------------------------------------------------------
   
   Encoding using an average bitrate mode (ABR).
   example: 44kHz stereo coupled, average 128kbps VBR
   
   ret = vorbis_encode_init(&vi,2,44100,-1,128000,-1);
   
   ---------------------------------------------------------------------
   
   Encode using a quality mode, but select that quality mode by asking for
   an approximate bitrate.  This is not ABR, it is true VBR, but selected
   using the bitrate interface, and then turning bitrate management off:
   
   ret = ( vorbis_encode_setup_managed(&vi,2,44100,-1,128000,-1) ||
   vorbis_encode_ctl(&vi,OV_ECTL_RATEMANAGE2_SET,NULL) ||
   vorbis_encode_setup_init(&vi));
   
   *********************************************************************/
  
  ret=vorbis_encode_init_vbr(&vi,2,44100,0.1);
  
  /* do not continue if setup failed; this can happen if we ask for a
   mode that libVorbis does not support (eg, too low a bitrate, etc,
   will return 'OV_EIMPL') */
  
  if(ret)exit(1);
  
  /* add a comment */
  vorbis_comment_init(&vc);
  vorbis_comment_add_tag(&vc,"ENCODER","vorbis_interface.c");
  
  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init(&vd,&vi);
  vorbis_block_init(&vd,&vb);
  
  /* set up our packet->stream encoder */
  /* pick a random serial number; that way we can more likely build
   chained streams just by concatenation */
  srand(time(NULL));
  ogg_stream_init(&os,rand());
  
  /* Vorbis streams begin with three headers; the initial header (with
   most of the codec setup parameters) which is mandated by the Ogg
   bitstream spec.  The second header holds any comment fields.  The
   third header holds the bitstream codebook.  We merely need to
   make the headers, then pass them to libvorbis one at a time;
   libvorbis handles the additional Ogg bitstream constraints */
  
  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;
    
    vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
    ogg_stream_packetin(&os,&header); /* automatically placed in its own
                                       page */
    ogg_stream_packetin(&os,&header_comm);
    ogg_stream_packetin(&os,&header_code);
    
    /* This ensures the actual
     * audio data will start on a new page, as per spec
     */
    while(!eos){
      int result=ogg_stream_flush(&os,&og);
      if(result==0)break;
      fwrite(og.header,1,og.header_len,stdout);
      fwrite(og.body,1,og.body_len,stdout);
    }
    
  }
  
  while(!eos){
    long i;
    long bytes=fread(readbuffer,1,READ*4,stdin); /* stereo hardwired here */
    
    if(bytes==0){
      /* end of file.  this can be done implicitly in the mainline,
       but it's easier to see here in non-clever fashion.
       Tell the library we're at end of stream so that it can handle
       the last frame and mark end of stream in the output properly */
      vorbis_analysis_wrote(&vd,0);
      
    }else{
      /* data to encode */
      
      /* expose the buffer to submit data */
      float **buffer=vorbis_analysis_buffer(&vd,READ);
      
      /* uninterleave samples */
      for(i=0;i<bytes/4;i++){
        buffer[0][i]=((readbuffer[i*4+1]<<8)|
                      (0x00ff&(int)readbuffer[i*4]))/32768.f;
        buffer[1][i]=((readbuffer[i*4+3]<<8)|
                      (0x00ff&(int)readbuffer[i*4+2]))/32768.f;
      }
      
      /* tell the library how much we actually submitted */
      vorbis_analysis_wrote(&vd,i);
    }
    
    /* vorbis does some data preanalysis, then divvies up blocks for
     more involved (potentially parallel) processing.  Get a single
     block for encoding now */
    while(vorbis_analysis_blockout(&vd,&vb)==1){
      
      /* analysis, assume we want to use bitrate management */
      vorbis_analysis(&vb,NULL);
      vorbis_bitrate_addblock(&vb);
      
      while(vorbis_bitrate_flushpacket(&vd,&op)){
        
        /* weld the packet into the bitstream */
        ogg_stream_packetin(&os,&op);
        
        /* write out pages (if any) */
        while(!eos){
          int result=ogg_stream_pageout(&os,&og);
          if(result==0)break;
          fwrite(og.header,1,og.header_len,stdout);
          fwrite(og.body,1,og.body_len,stdout);
          
          /* this could be set above, but for illustrative purposes, I do
           it here (to show that vorbis does know where the stream ends) */
          
          if(ogg_page_eos(&og))eos=1;
        }
      }
    }
  }
  
  /* clean up and exit.  vorbis_info_clear() must be called last */
  
  ogg_stream_clear(&os);
  vorbis_block_clear(&vb);
  vorbis_dsp_clear(&vd);
  vorbis_comment_clear(&vc);
  vorbis_info_clear(&vi);
  
  /* ogg_page and ogg_packet structs always point to storage in
   libvorbis.  They're never freed or manipulated directly */
  
  fprintf(stderr,"Done.\n");
  return(0);
}
