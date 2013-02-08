
#ifndef MODULES_AUDIO_CODING_CODECS_OPUS_MAIN_INTERFACE_OPUS_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_OPUS_MAIN_INTERFACE_OPUS_INTERFACE_H_

#include "typedefs.h"

typedef struct OPUS_encinst_t_ OPUS_encinst_t;
typedef struct OPUS_decinst_t_ OPUS_decinst_t;

#ifdef __cplusplus
extern "C" {
#endif
  
WebRtc_Word16 WebRtcOpus_CreateEnc(OPUS_encinst_t** inst, WebRtc_Word16 samplFreq);
WebRtc_Word16 WebRtcOpus_FreeEnc(OPUS_encinst_t *inst);
WebRtc_Word16 WebRtcOpus_CreateDec(OPUS_decinst_t** inst, WebRtc_Word16 samplFreq);
WebRtc_Word16 WebRtcOpus_FreeDec(OPUS_decinst_t* inst);
WebRtc_Word16 WebRtcOpus_Encode(OPUS_encinst_t* encInst,
                                WebRtc_Word16* input,
                                WebRtc_Word16* output,
                                WebRtc_Word16 len,
                                WebRtc_Word16 byteLen);
WebRtc_Word16 WebRtcOpus_EncoderInit(OPUS_encinst_t* encInst,
                                     WebRtc_Word16 samplFreq,
                                     WebRtc_Word16 mode,
                                     WebRtc_Word16 vbrFlag);
WebRtc_Word16 WebRtcOpus_Decode(OPUS_decinst_t *decInst,
                                WebRtc_UWord8 *encoded, WebRtc_Word16 len,
                                WebRtc_Word16 *decoded, WebRtc_Word16 *speechType);
WebRtc_Word16 WebRtcOpus_DecoderInit(OPUS_decinst_t *decInst);
void WebRtcOpus_Version(char *version);

#ifdef __cplusplus
}
#endif


#endif /* MODULES_AUDIO_CODING_CODECS_OPUS_MAIN_INTERFACE_OPUS_INTERFACE_H_ */
