#include <vector>
#include <brotli/encode.h>
#include <brotli/decode.h>

// Brotli压缩函数
bool BrotliCompress(const std::vector<char>& input, std::vector<char>& output) {
    size_t encoded_size = BrotliEncoderMaxCompressedSize(input.size());
    output.resize(encoded_size);
    
    if (BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW,
                             BROTLI_DEFAULT_MODE, input.size(),
                             reinterpret_cast<const uint8_t*>(input.data()),
                             &encoded_size,
                             reinterpret_cast<uint8_t*>(output.data()))) {
        output.resize(encoded_size);
        return true;
    }
    return false;
}

// Brotli解压函数
bool BrotliDecompress(const std::vector<char>& input, std::vector<char>& output) {
    size_t decoded_size = 0;
    if (BrotliDecoderDecompress(input.size(),
                               reinterpret_cast<const uint8_t*>(input.data()),
                               &decoded_size, nullptr)) {
        output.resize(decoded_size);
        return BrotliDecoderDecompress(
            input.size(), reinterpret_cast<const uint8_t*>(input.data()),
            &decoded_size, reinterpret_cast<uint8_t*>(output.data()));
    }
    return false;
}


