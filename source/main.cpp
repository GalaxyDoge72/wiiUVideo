// Wii U Libraries //
#include <wut.h>

// Decoding Libraries //
#include <theora/codec.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <ogg/ogg.h>

// Filesystem Libraries //
#include <stdio.h>

class TheoraDecoder {
    public: 
        th_info    m_info;
        th_comment m_comment;
        th_setup_info* m_setup;
        th_dec_ctx* m_context;

        TheoraDecoder() : m_setup(nullptr), m_context(nullptr) {
            th_info_init(&m_info);
            th_comment_init(&m_comment);
        }

        ~TheoraDecoder() {
            if (m_context) {
                th_decode_free(m_context);
            }
            if (m_setup) {
                th_setup_free(m_setup);
            }
            th_comment_clear(&m_comment);
            th_info_clear(&m_info);
        }
};

int main() {
    TheoraDecoder decoder;
    return 0;
}