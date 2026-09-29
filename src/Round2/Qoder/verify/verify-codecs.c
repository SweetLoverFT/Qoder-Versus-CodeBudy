#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

static void probe(enum AVCodecID id, const char *name) {
    const AVCodec *c = avcodec_find_decoder(id);
    printf("%-22s -> %s\n", name, c ? c->name : "(not available)");
}

int main(void) {
    printf("libavutil version  : %s\n", av_version_info());
    printf("libavcodec version : %u.%u.%u\n",
           avcodec_version() >> 16, (avcodec_version() >> 8) & 0xff, avcodec_version() & 0xff);

    probe(AV_CODEC_ID_MP3,      "MP3");
    probe(AV_CODEC_ID_SBC,      "SBC");
    probe(AV_CODEC_ID_PCM_S16LE, "PCM_S16LE (wav)");
    probe(AV_CODEC_ID_PCM_S24LE, "PCM_S24LE");
    probe(AV_CODEC_ID_PCM_F32LE, "PCM_F32LE");
    probe(AV_CODEC_ID_PCM_U8,   "PCM_U8");
    probe(AV_CODEC_ID_AAC,      "AAC (should be absent)");
    return 0;
}
