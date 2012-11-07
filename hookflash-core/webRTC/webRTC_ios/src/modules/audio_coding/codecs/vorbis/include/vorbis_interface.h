
#ifndef MODULES_AUDIO_CODING_CODECS_VORBIS_MAIN_INTERFACE_VORBIS_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_VORBIS_MAIN_INTERFACE_VORBIS_INTERFACE_H_

#include "typedefs.h"

typedef struct VORBIS_encinst_t_ VORBIS_encinst_t;
typedef struct VORBIS_decinst_t_ VORBIS_decinst_t;

#ifdef __cplusplus
extern "C" {
#endif
  
WebRtc_Word16 WebRtcVorbis_CreateEnc(VORBIS_encinst_t **VORBISenc_inst,
                                     WebRtc_Word32 fs);
WebRtc_Word16 WebRtcVorbis_FreeEnc(VORBIS_encinst_t *VORBISenc_inst);
WebRtc_Word16 WebRtcVorbis_CreateDec(VORBIS_decinst_t **VORBISdec_inst,
                                     WebRtc_Word32 fs,
                                     WebRtc_Word16 enh_enabled);
WebRtc_Word16 WebRtcVorbis_FreeDec(VORBIS_decinst_t *VORBISdec_inst);
WebRtc_Word16 WebRtcVorbis_Encode(VORBIS_encinst_t *VORBISenc_inst,
                                  const WebRtc_Word16* speechIn, 
                                  WebRtc_Word16 len,
                                  WebRtc_UWord8* encoded);
WebRtc_Word16 WebRtcVorbis_EncoderInit(VORBIS_encinst_t *VORBISenc_inst);
WebRtc_Word16 WebRtcVorbis_Decode(VORBIS_decinst_t *VORBISdec_inst,
                                  WebRtc_UWord8 *encoded, WebRtc_Word16 len,
                                  WebRtc_Word16 *decoded, WebRtc_Word16 *speechType);
WebRtc_Word16 WebRtcVorbis_DecoderInit(VORBIS_decinst_t *VORBISdec_inst);
void WebRtcVorbis_Version(char *version);

#ifdef __cplusplus
}
#endif


#endif /* MODULES_AUDIO_CODING_CODECS_VORBIS_MAIN_INTERFACE_VORBIS_INTERFACE_H_ */
