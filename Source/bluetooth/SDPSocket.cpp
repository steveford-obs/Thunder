#include "SDPSocket.h"

namespace WPEFramework {

namespace Bluetooth {

    enum sizetype {
        SIZE_8 = 0,
        SIZE_16 = 1,
        SIZE_32 = 2,
        SIZE_64 = 3,
        SIZE_128 = 4,
        SIZE_U8_FOLLOWS = 5,
        SIZE_U16_FOLLOWS = 6,
        SIZE_U32_FOLLOWS = 7
    };

    uint8_t SDPSocket::Command::PDU::Serializer::PushDescriptor(uint8_t buffer[], const elementtype type, const uint32_t size)
    {
        uint8_t offset = 0;
        buffer[offset++] = (type | SIZE_8);

        switch (type) {
            case NIL:
                ASSERT(size == 0);
                // Exception: even if size descriptor says BYTE for NIL type,
                // actually there's no data following.
                break;
            case BOOL:
                ASSERT(size == 1);
                break;
            case INT:
            case UINT:
                if (size == 1) {
                    // already set
                } else if (size == 2) {
                    buffer[0] |= SIZE_16;
                } else if (size == 4) {
                    buffer[0] |= SIZE_32;
                } else if (size == 8) {
                    buffer[0] |= SIZE_64;
                } else {
                    ASSERT(false && "Invalid INT size");
                }
                break;
            case UUID:
                if (size == 2) {
                    buffer[0] |= SIZE_16;
                } else if (size == 4) {
                    buffer[0] |= SIZE_32;
                } else if (size == 16) {
                    buffer[0] |= SIZE_128;
                } else {
                    ASSERT(false && "Invalid UUID size");
                }
                break;
            case TEXT:
            case SEQ:
            case ALT:
            case URL:
                if (size <= 0xFF) {
                    buffer[0] |= SIZE_U8_FOLLOWS;
                } else if (size <= 0xFFFF) {
                    buffer[0] |= SIZE_U16_FOLLOWS;
                    buffer[offset++] = (size >> 8);
                } else {
                    buffer[0] |= SIZE_U32_FOLLOWS;
                    buffer[offset++] = (size >> 24);
                    buffer[offset++] = (size >> 16);
                    buffer[offset++] = (size >> 8);
                }
                buffer[offset++] = size;
                break;
        }

        return (offset);
    }

    uint8_t SDPSocket::Command::PDU::Serializer::PopDescriptor(elementtype& type, uint32_t& size) const
    {
        uint8_t offset = 0;
        uint8_t t = _buffer[_readerOffset + offset++];

        switch (t & 7) {
        case SIZE_8:
            size = 1;
            break;
        case SIZE_16:
            size = 2;
            break;
        case SIZE_32:
            size = 4;
            break;
        case SIZE_64:
            size = 8;
            break;
        case SIZE_128:
            size = 16;
            break;
        case SIZE_U8_FOLLOWS:
            size = _buffer[_readerOffset + offset++];
            break;
        case SIZE_U16_FOLLOWS:
            size = (_buffer[_readerOffset + offset++] << 8);
            size |= _buffer[_readerOffset + offset++];
            break;
        case SIZE_U32_FOLLOWS:
            size = (_buffer[_readerOffset + offset++] << 24);
            size |= (_buffer[_readerOffset + offset++] << 16);
            size |= (_buffer[_readerOffset + offset++] << 8);
            size |= _buffer[_readerOffset + offset++];
            break;
        default:
            TRACE_L1(_T("Unexpected descriptor size in SDP payload [0x%01x]"), (t & 7));
            size = 0;
            break;
        }

        type = static_cast<elementtype>(t & 0xF8);
        if (type == NIL) {
            size = 0;
        }

        return (offset);
    }
} // namespace Bluetooth

}
