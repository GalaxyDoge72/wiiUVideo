// Wii U Libraries //
#include <wut.h>

// Decoding Libraries //
#include <theora/codec.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <ogg/ogg.h>

// Filesystem Libraries //
#include <stdio.h>


// Definitions //
#define READ_BUFFER_SIZE 4096

class TheoraDecoder {
    public: 
        th_info    m_info;
        th_comment m_comment;
        th_setup_info* m_setup;
        th_dec_ctx* m_context;

        ogg_sync_state m_ogg_sync;
        ogg_stream_state m_ogg_stream;
        FILE* m_file;

        ogg_int64_t m_granule_pos;

        TheoraDecoder() : m_setup(nullptr), m_context(nullptr), m_file(nullptr), m_granule_pos(0){
            th_info_init(&m_info);
            th_comment_init(&m_comment);
            ogg_sync_init(&m_ogg_sync);
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

            ogg_sync_clear(&m_ogg_sync); // Cleanup Ogg sync state
            if (m_file) {
                fclose(m_file);
            }
        }

        bool OpenAndFindStream(const char* filePath);
        bool ProcessHeaders();
        // int DecodeNextFrame(th_ycbcr_buffer& yuv_buffer);
};

bool TheoraDecoder::OpenAndFindStream(const char* filePath) {
    m_file = fopen(filePath, "rb");
    if (!m_file) {
        return false; 
    }

    ogg_page page;
    ogg_packet packet;
    
    while (true) {
        // Read file data into the Ogg sync buffer
        char *buffer = ogg_sync_buffer(&m_ogg_sync, READ_BUFFER_SIZE);
        int bytes = fread(buffer, 1, READ_BUFFER_SIZE, m_file);
        ogg_sync_wrote(&m_ogg_sync, bytes);
        
        if (bytes == 0) return false; // Unexpected EOF
        
        // Try to extract a complete page
        if (ogg_sync_pageout(&m_ogg_sync, &page) == 1) {
            // Found a page. Check if it's the beginning of a stream (BOS)
            if (ogg_page_bos(&page)) {
                
                // Initialize the stream state with this page's serial number
                ogg_stream_init(&m_ogg_stream, ogg_page_serialno(&page));

                // Feed the page back into the stream state
                if (ogg_stream_pagein(&m_ogg_stream, &page) < 0) {
                    ogg_stream_clear(&m_ogg_stream);
                    continue; // Bad page, try next one
                }

                if (ogg_stream_packetout(&m_ogg_stream, &packet) == 1) {
                    // Check if it's a Theora header
                    if (th_decode_headerin(&m_info, &m_comment, &m_setup, &packet) > 0) {
                        // Success! This is the Theora stream.
                        return true; 
                    }
                }
                // If it wasn't Theora, clear the stream and continue looping for a new stream
                ogg_stream_clear(&m_ogg_stream);
            }
        }
    }
    // Should be unreachable if successful
    return false; 
}

bool TheoraDecoder::ProcessHeaders() {
    int ret = 0;
    int header_count = 1; // First header was processed in OpenAndFindStream

    while (header_count < 3) {
        ogg_packet packet;
        int result = ogg_stream_packetout(&m_ogg_stream, &packet);

        if (result == 0) {
            // Need more data in the stream. Read more pages from the file.
            ogg_page page;
            while(ogg_sync_pageout(&m_ogg_sync, &page) != 1) {
                char *buffer = ogg_sync_buffer(&m_ogg_sync, READ_BUFFER_SIZE);
                int bytes = fread(buffer, 1, READ_BUFFER_SIZE, m_file);
                ogg_sync_wrote(&m_ogg_sync, bytes);
                if (bytes == 0) return false; // Unexpected EOF
            }
            ogg_stream_pagein(&m_ogg_stream, &page);
            continue;
        } 
        
        if (result < 0) return false; // Ogg stream error

        ret = th_decode_headerin(&m_info, &m_comment, &m_setup, &packet);
        if (ret < 0) return false; // Bad header packet
        
        header_count++;
    }
    m_context = th_decode_alloc(&m_info, m_setup);
    th_setup_free(m_setup);
    m_setup = nullptr;
    return m_context != nullptr;
}

int main() {
    TheoraDecoder decoder;
    if (decoder.OpenAndFindStream("sd:/video.ogv")) {
        if (decoder.ProcessHeaders()) {

        }
    }
    return 0;
}