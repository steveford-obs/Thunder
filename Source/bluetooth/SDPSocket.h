/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Module.h"
#include "UUID.h"

#include <limits>
#include <stack>


namespace WPEFramework {

namespace Bluetooth {

    class SDPSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    public:
        static constexpr uint8_t SDP_PSM = 1;

    private:
        SDPSocket(const SDPSocket&) = delete;
        SDPSocket& operator=(const SDPSocket&) = delete;

        class Callback : public Core::IOutbound::ICallback
        {
        public:
            Callback() = delete;
            Callback(const Callback&) = delete;
            Callback& operator= (const Callback&) = delete;

            Callback(SDPSocket& parent)
                : _parent(parent)
            {
            }
            ~Callback() = default;

        public:
            void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
                _parent.CommandCompleted(data, error_code);
            }

        private:
            SDPSocket& _parent;
        }; // class Callback

    public:
        static constexpr uint32_t CommunicationTimeout = 2000; /* 2 seconds. */

        struct use_descriptor_t{};
        static constexpr use_descriptor_t use_descriptor = use_descriptor_t(); // for selecting a proper overload

        class Record {
        public:
            enum elementtype : uint8_t {
                NIL = 0x00,
                UINT = 0x08,
                INT = 0x10,
                UUID = 0x18,
                TEXT = 0x20,
                BOOL = 0x28,
                SEQ = 0x30,
                ALT = 0x38,
                URL = 0x40,
            };

        public:
            using Builder = std::function<void(Record&)>;
            using Inspector = std::function<void(const Record&)>;

            enum class Continuation : uint8_t {
                ABSENT = 0,
                FOLLOWS
            };

        public:
            Record()
                : _buffer(nullptr)
                , _bufferSize(0)
                , _filledSize(0)
                , _readerOffset(0)
                , _writerOffset(0)
            {
            }
            Record(const Record& buffer)
                : _buffer(const_cast<uint8_t*>(buffer.Data()))
                , _bufferSize(buffer.Length())
                , _filledSize(_bufferSize)
                , _readerOffset(0)
                , _writerOffset(_bufferSize)
            {
            }
            Record(uint8_t buffer[], const uint32_t bufferSize, const uint32_t filledSize = 0)
                : _buffer(buffer)
                , _bufferSize(bufferSize)
                , _filledSize(filledSize)
                , _readerOffset(0)
                , _writerOffset(filledSize)
            {
                ASSERT(_buffer != nullptr);
                ASSERT(_bufferSize != 0);
                ASSERT(_bufferSize >= _filledSize);
            }
            Record& operator=(const Record&) = delete;

            ~Record() = default;

        public:
            const string ToString() const
            {
                string val;
                Core::ToHexString(Data(), Length(), val);
                return (val);
            }

        public:
            uint32_t Available() const
            {
                return (_writerOffset > _readerOffset? (_writerOffset - _readerOffset) : 0);
            }
            uint32_t Length() const
            {
                return (_writerOffset);
            }
            const uint8_t* Data() const
            {
                return (_buffer);
            }
            void Assign(uint8_t buffer[], const uint32_t bufferSize)
            {
                _buffer = buffer;
                _bufferSize = bufferSize;
                _filledSize = bufferSize;
                _writerOffset = bufferSize;
                Rewind();
            }
            void Clear()
            {
                _writerOffset = 0;
                _filledSize = 0;
                Rewind();
            }
            void Rewind() const
            {
                _readerOffset = 0;
            }

        public:
            void Push(const Continuation cont, const string& data = _T(""))
            {
                if (cont == Continuation::ABSENT) {
                    Push(static_cast<uint8_t>(0));
                } else {
                    ASSERT(data.length() <= 16)
                    Push(static_cast<uint8_t>(data.length()));
                    Push(data);
                }
            }
            void Pop(Continuation& cont, std::string& data) const
            {
                uint8_t size = 0;
                Pop(size);
                cont = (size == 0? Continuation::ABSENT : Continuation::FOLLOWS);
                if (size != 0) {
                    Pop(data, size);
                } else {
                    data.clear();
                }
            }

        public:
            void Push()
            {
            }
            void Push(use_descriptor_t)
            {
                _writerOffset += PushDescriptor(&_buffer[_writerOffset], NIL);
            }
            void Push(const Bluetooth::UUID& value)
            {
                uint8_t size = value.Length();
                while (size-- > 0) {
                    _buffer[_writerOffset++] = value.Data()[size]; // reverse!
                }
            }
            void Push(use_descriptor_t, const Bluetooth::UUID& value)
            {
                _writerOffset += PushDescriptor(&_buffer[_writerOffset], UUID, value.Length());
                Push(value);
            }
            void Push(const string& value)
            {
                ::memcpy(&_buffer[_writerOffset], value.data(), value.length()); // no nul-terminator!
                _writerOffset += value.length();
            }
            void Push(use_descriptor_t, const string& value, bool url = false)
            {
                _writerOffset += PushDescriptor(&_buffer[_writerOffset], (url? URL : TEXT), value.length());
                Push(value);
            }
            void Push(const bool value)
            {
                Push(static_cast<uint8_t>(value));
            }
            void Push(use_descriptor_t, const bool value)
            {
                _writerOffset += PushDescriptor(&_buffer[_writerOffset], BOOL, 1);
                Push(value);
            }
            template<typename TYPE, /* enable if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
            void Push(const TYPE value)
            {
                ASSERT(sizeof(TYPE) <= 4);
                PushIntegerValue(static_cast<typename std::make_unsigned<TYPE>::type>(value));
            }
            template<typename TYPE, /* enable if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
            void Push(use_descriptor_t, const TYPE value)
            {
                _writerOffset += PushDescriptor(&_buffer[_writerOffset],
                                                (std::numeric_limits<TYPE>::is_signed? INT : UINT),
                                                sizeof(TYPE));
                Push(value);
            }
            template<typename TYPE, /* enable if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
            void Push(const TYPE value)
            {
                ASSERT(sizeof(TYPE) <= 4);
                PushIntegerValue(static_cast<typename std::underlying_type<TYPE>::type>(value));
            }
            template<typename TYPE, /* enable if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
            void Push(use_descriptor_t, const TYPE value)
            {
                _writerOffset += PushDescriptor(&_buffer[_writerOffset], UINT, sizeof(TYPE));
                Push(value);
            }
            void Push(const Record& sequence)
            {
                ::memcpy(&_buffer[_writerOffset], sequence.Data(), sequence.Length());
                _writerOffset += sequence.Length();
            }
            void Push(use_descriptor_t, const Record& sequence, const bool alternative = false)
            {
                _writerOffset += PushDescriptor(&_buffer[_writerOffset], (alternative? ALT : SEQ), sequence.Length());
                Push(sequence);
            }
            void Push(const Builder& builder, const uint16_t scratchPadSize = 2048)
            {
                uint8_t scratchPad[scratchPadSize];
                Record sequence(scratchPad, sizeof(scratchPad));
                builder(sequence);
                Push(sequence);
            }
            void Push(use_descriptor_t, const Builder& builder, const bool alternative = false, const uint16_t scratchPadSize = 2048)
            {
                uint8_t scratchPad[scratchPadSize];
                Record sequence(scratchPad, sizeof(scratchPad));
                builder(sequence);
                Push(use_descriptor, sequence, alternative);
            }
            template<typename TYPE>
            void Push(const std::list<TYPE>& list, const uint16_t scratchPadSize = 2048)
            {
                Push([&](Record& sequence){
                    for (auto item : list) {
                        sequence.Push(item);
                    }
                }, scratchPadSize);
            }
            template<typename TYPE>
            void Push(use_descriptor_t, const std::list<TYPE>& list, const bool alternative = false, const uint16_t scratchPadSize = 2048)
            {
                Push(use_descriptor, [&](Record& sequence){
                    for (auto item : list) {
                        sequence.Push(use_descriptor, item);
                    }
                }, alternative, scratchPadSize);
            }

        public:
            void Pop(string& value, const uint16_t length) const
            {
                value.assign(reinterpret_cast<char*>(&_buffer[_readerOffset]), length);
                _readerOffset += length;
            }
            void Pop(use_descriptor_t, string& value) const
            {
                elementtype type;
                uint32_t length = 0;
                _readerOffset += PopDescriptor(type, length);
                if ((type == TEXT) || (type == URL)) {
                    Pop(value, length);
                } else {
                    TRACE_L1(_T("Unexpected descriptor in SDP payload [0x%02x], expected TEXT or URL"), type);
                }
            }
            template<typename TYPE, /* enable if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
            void Pop(use_descriptor_t, TYPE& value)
            {
                elementtype type;
                uint32_t size = 0;
                _readerOffset += PopDescriptor(type, size);
                if (type == UINT) {
                    typename std::underlying_type<TYPE>::type temp;
                    if (size != sizeof(temp)) {
                        TRACE_L1(_T("Warning: enum underlying type size does not match!"));
                    }
                    Pop(temp);
                    value = static_cast<TYPE>(temp);
                } else {
                    TRACE_L1(_T("Unexpected integer type for enum"));
                }
            }
            template<typename TYPE, /* enable if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
            void Pop(TYPE& value)
            {
                ASSERT(sizeof(TYPE) <= 4);
                typename std::underlying_type<TYPE>::type temp;
                Pop(temp);
                value = static_cast<TYPE>(temp);
            }
            template<typename TYPE, /* enable if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
            void Pop(TYPE& value) const
            {
                PopIntegerValue(value);
            }
            template<typename TYPE, /* enable if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
            void Pop(use_descriptor_t, TYPE& value, uint32_t* outSize = nullptr) const
            {
                elementtype type;
                uint32_t size = 0;
                _readerOffset += PopDescriptor(type, size);
                if (type == (std::numeric_limits<TYPE>::is_signed? INT : UINT)) {
                    if (size > sizeof(TYPE)) {
                        TRACE_L1(_T("Warning: integer value possibly truncated!"));
                    }
                    if (size == 1) {
                        uint8_t val; PopIntegerValue(val); value = val;
                    } else if (size == 2) {
                        uint16_t val; PopIntegerValue(val); value = val;
                    } else if (size == 4) {
                        uint32_t val; PopIntegerValue(val); value = val;
                    } else {
                        TRACE_L1(_T("Unexpected integer size"));
                    }
                    if (outSize != nullptr) {
                        (*outSize) = size;
                    }
                } else {
                    TRACE_L1(_T("Unexpected integer type"));
                }
            }
            template<typename TYPE>
            void Pop(std::list<TYPE>& list, const uint16_t count) const
            {
                uint16_t i = count;
                while (i-- > 0) {
                    TYPE item;
                    Pop(item);
                    list.push_back(item);
                }
            }
            template<typename TYPE>
            void Pop(use_descriptor_t, std::list<TYPE>& list, const uint16_t count) const
            {
                uint16_t i = count;
                while (i-- > 0) {
                    TYPE item;
                    Pop(use_descriptor, item);
                    list.push_back(item);
                }
            }
            void Pop(use_descriptor_t, Bluetooth::UUID& uuid) const
            {
                uint32_t size = 0;
                elementtype type;
                _readerOffset += PopDescriptor(type, size);
                if (size == 2) {
                    uuid = Bluetooth::UUID((_buffer[_readerOffset] << 8) | _buffer[_readerOffset + 1]);
                } else {
                    uint8_t buffer[size];
                    uint8_t i = size;
                    while (i-- > 0) {
                        buffer[i] = _buffer[_readerOffset++];
                    }
                    uuid = Bluetooth::UUID(buffer);
                }
                _readerOffset += size;
            }
            void Pop(use_descriptor_t, const Inspector& inspector) const
            {
                uint32_t size = 0;
                elementtype type;
                _readerOffset += PopDescriptor(type, size);
                if (type == SEQ) {
                    Record sequence(&_buffer[_readerOffset], size, size);
                    inspector(sequence);
                } else {
                    TRACE_L1(_T("Unexpected descriptor in SDP payload [0x%02x], expected SEQ"), type);
                }
                _readerOffset += size;
            }
            void Pop(use_descriptor_t, Record& element) const
            {
                elementtype elemType;
                uint32_t elemSize;
                uint8_t descriptorSize = PopDescriptor(elemType, elemSize);
                if (Available() >= (descriptorSize + elemSize)) {
                    element.Assign(&_buffer[_readerOffset], (descriptorSize + elemSize));
                    _readerOffset += (descriptorSize + elemSize);
                } else {
                    TRACE_L1(_T("Truncated SDP payload"));
                    _readerOffset = _writerOffset;
                }
            }
            void Pop(Record& element, const uint32_t size) const
            {
                if (Available() >= size) {
                    element.Assign(&_buffer[_readerOffset], size);
                    _readerOffset += size;
                } else {
                    TRACE_L1(_T("Truncated SDP payload"));
                    _readerOffset = _writerOffset;
                }
            }

        private:
            uint8_t PushDescriptor(uint8_t buffer[], const elementtype type, const uint32_t size = 0);
            uint8_t PopDescriptor(elementtype& type, uint32_t& size) const;

            void PushIntegerValue(const uint8_t value)
            {
                _buffer[_writerOffset++] = value;
            }
            void PushIntegerValue(const uint16_t value)
            {
                _buffer[_writerOffset++] = (value >> 8);
                _buffer[_writerOffset++] = value;
            }
            void PushIntegerValue(const uint32_t value)
            {
                _buffer[_writerOffset++] = (value >> 24);
                _buffer[_writerOffset++] = (value >> 16);
                _buffer[_writerOffset++] = (value >> 8);
                _buffer[_writerOffset++] = value;
            }
            void PopIntegerValue(uint8_t& value) const
            {
                value = _buffer[_readerOffset++];
            }
            void PopIntegerValue(uint16_t& value) const
            {
                value = ((_buffer[_readerOffset] << 8) | _buffer[_readerOffset + 1]);
                _readerOffset += 2;
            }
            void PopIntegerValue(uint32_t& value) const
            {
                value = ((_buffer[_readerOffset] << 24) | (_buffer[_readerOffset + 1] << 16)
                            | (_buffer[_readerOffset + 2] << 8) | _buffer[_readerOffset + 3]);
                _readerOffset += 4;
            }

            private:
            uint8_t* _buffer;
            uint32_t _bufferSize;
            uint32_t _filledSize;
            mutable uint32_t _readerOffset;
            uint32_t _writerOffset;
        }; // class Record

        class Command : public Core::IOutbound, public Core::IInbound {
        public:
            class PDU {
            public:
                static constexpr uint16_t DEFAULT_SCRATCHPAD_SIZE = 4096;
                static constexpr uint8_t HEADER_SIZE = 5;
                static constexpr uint8_t MAX_CONTINUATION_INFO_SIZE = 16;

            public:
                enum pdutype : uint8_t {
                    Invalid = 0,
                    ErrorResponse = 1,
                    ServiceSearchRequest = 2,
                    ServiceSearchResponse = 3,
                    ServiceAttributeRequest = 4,
                    ServiceAttributeResponse = 5,
                    ServiceSearchAttributeRequest = 6,
                    ServiceSearchAttributeResponse = 7,
                };

                enum errorid : uint16_t {
                    Success = 0,
                    UnsupportedSdpVersion = 1,
                    InvalidServiceRecordHandle = 2,
                    InvalidRequestSyntax = 3,
                    InvalidPduSize = 4,
                    InvalidContinuationState = 5,
                    InsufficientResources = 6,
                    Reserved = 255,
                    DeserializationFailed,
                    PacketContinuation
                };

            public:
                static constexpr uint16_t MIN_BUFFER_SIZE = (HEADER_SIZE + 1 + MAX_CONTINUATION_INFO_SIZE);

                explicit PDU(const uint16_t bufferSize)
                    : _bufferSize(bufferSize)
                    , _buffer(static_cast<uint8_t*>(::malloc(_bufferSize)))
                    , _size(0)
                    , _transactionId(0)
                    , _continuationOffset(0)
                    , _payloadSize(0)
                {
                    ASSERT(_buffer != nullptr);
                    ASSERT(_bufferSize > MIN_BUFFER_SIZE);
                }
                ~PDU()
                {
                    if (_buffer != nullptr) {
                        ::free(_buffer);
                    }
                }

            public:
                void Clear()
                {
                    ::memset(_buffer, 0, HEADER_SIZE);
                    _size = 0;
                    _continuationOffset = 0;
                    _payloadSize = 0;
                }
                bool IsValid() const
                {
                    return ((_buffer != nullptr) && (_bufferSize > MIN_BUFFER_SIZE) && (Type() != Invalid));
                }
                uint32_t Length() const
                {
                    return (_size);
                }
                uint16_t Capacity() const
                {
                    return (_bufferSize - MIN_BUFFER_SIZE);
                }
                const uint8_t* Data() const
                {
                    return (_buffer);
                }
                pdutype Type() const
                {
                    return (static_cast<pdutype>(_buffer[0]));
                }
                uint16_t TransactionId() const
                {
                    return ((_buffer[1] << 8) | _buffer[2]);
                }
                void Finalize(const string& continuation = "")
                {
                    // Called during command construction or by retrigger due to continuation
                    ASSERT(_size >= HEADER_SIZE);
                    ASSERT(_continuationOffset >= HEADER_SIZE);

                    // Increment transaction ID
                    _transactionId++;
                    _buffer[1] = (_transactionId >> 8);
                    _buffer[2] = _transactionId;

                    // Update size
                    const uint16_t payloadSize = (_payloadSize + 1 + continuation.size());
                    _buffer[3] = (payloadSize >> 8);
                    _buffer[4] = payloadSize;

                    // Attach continuation information
                    _buffer[_continuationOffset] = continuation.size();
                    ::memcpy(_buffer + _continuationOffset + 1, continuation.data(), continuation.size());
                    _size = (HEADER_SIZE + payloadSize);
                }
                void Construct(const pdutype type, const Record& parameters)
                {
                    ASSERT(Capacity() >= (parameters.Length() + MIN_BUFFER_SIZE));

                    Clear();

                    if (Capacity() >= (parameters.Length() + MIN_BUFFER_SIZE)) {
                        ::memset(_buffer, 0, HEADER_SIZE);
                        ::memcpy(_buffer + HEADER_SIZE, parameters.Data(), parameters.Length());
                        _buffer[0] = type;
                        _payloadSize = parameters.Length();
                        _size = (HEADER_SIZE + _payloadSize);
                        _continuationOffset = _size;

                        Finalize();
                    } else {
                        TRACE(Trace::Error, (_T("Parameters to large to fit in PDU [%d]"), parameters.Length()));
                    }
                }
                void Construct(const pdutype type, const Record::Builder& builder, const uint32_t scratchPadSize = DEFAULT_SCRATCHPAD_SIZE)
                {
                    uint8_t scratchPad[scratchPadSize];
                    Record parameters(scratchPad, sizeof(scratchPad));
                    builder(parameters);
                    Construct(type, parameters);
                }

            private:
                uint16_t _bufferSize;
                uint8_t* _buffer;
                uint16_t _size;
                uint16_t _transactionId;
                uint16_t _continuationOffset;
                uint16_t _payloadSize;
            }; // class PDU

            class Request {
            public:
                Request(const Request&) = delete;
                Request& operator=(const Request&) = delete;
                explicit Request(uint16_t pduBufferSize)
                    : _pdu(pduBufferSize)
                    , _offset(0)
                {
                }
                ~Request() = default;

            public:
                void Reload() const
                {
                    _offset = 0;
                }
                bool IsValid() const
                {
                    return (_pdu.IsValid());
                }
                uint16_t Serialize(uint8_t stream[], const uint16_t length) const
                {
                    uint16_t result = std::min(_pdu.Length(), static_cast<uint32_t>(length));
                    if (result > 0) {
                        ::memcpy(stream, (_pdu.Data() + _offset), result);
                        _offset += result;

                        printf("SDP send [%d]: ", result);
                        for (uint8_t index = 0; index < (result - 1); index++) { printf("%02X:", stream[index]); } printf("%02X\n", stream[result - 1]);
                    }
                    return (result);
                }
                void Finalize(const string& cont)
                {
                    _pdu.Finalize(cont);
                }
                uint16_t TransactionId() const
                {
                    return (_pdu.TransactionId());
                }

            public:
                void ServiceSearch(const std::list<UUID>& services, const uint16_t maxResults)
                {
                    ASSERT(services.size() <= 12); // As per spec

                    _pdu.Construct(PDU::ServiceSearchRequest, [&](Record& parameters) {
                        parameters.Push(use_descriptor, services);
                        parameters.Push(maxResults);
                    });
                }
                void ServiceAttribute(const uint32_t serviceHandle, const std::list<uint32_t>& attributeIdRanges)
                {
                    ASSERT(attributeIdRanges.size() <= 256);

                    _pdu.Construct(PDU::ServiceAttributeRequest, [&](Record& parameters) {
                        parameters.Push(serviceHandle);
                        parameters.Push(_pdu.Capacity());
                        parameters.Push(use_descriptor, attributeIdRanges);
                    });
                }
                void ServiceSearchAttribute(const std::list<UUID>& services, const std::list<uint32_t>& attributeIdRanges)
                {
                    ASSERT(services.size() <= 12);
                    ASSERT(attributeIdRanges.size() <= 256);

                    _pdu.Construct(PDU::ServiceSearchAttributeRequest, [&](Record& parameters) {
                        parameters.Push(use_descriptor, services);
                        parameters.Push(_pdu.Capacity());
                        parameters.Push(use_descriptor, attributeIdRanges);
                    });
                }

            private:
                PDU _pdu;
                mutable uint32_t _offset;
            }; // class Request

        public:
            class Response {
            private:
                Response(const Response&) = delete;
                Response& operator=(const Response&) = delete;

                typedef std::pair<uint16_t, std::pair<uint16_t, uint16_t>> Entry;

            public:
                explicit Response(uint32_t payloadSize)
                    : _type(PDU::Invalid)
                    , _status(PDU::Reserved)
                    , _scratchPad(static_cast<uint8_t*>(::malloc(payloadSize)))
                    , _payload(_scratchPad, payloadSize)
                {
                }
                ~Response()
                {
                    if (_scratchPad != nullptr) {
                        ::free(_scratchPad);
                    }
                }

            public:
                void Clear()
                {
                    _status = PDU::Reserved;
                    _type = PDU::Invalid;
                    _handles.clear();
                    _attributes.clear();
                    _continuationData.clear();
                    _payload.Clear();
                }
                PDU::pdutype Type() const
                {
                    return (_type);
                }
                PDU::errorid Status() const
                {
                    return (_status);
                }
                const std::list<uint32_t>& Handles() const
                {
                    return (_handles);
                }
                const std::map<uint16_t, Record>& Attributes() const
                {
                    return (_attributes);
                }
                const string& Continuation() const
                {
                    return (_continuationData);
                }

                uint16_t Deserialize(const uint16_t reqTransactionId, const uint8_t stream[], const uint16_t length);

            private:
                PDU::errorid DeserializeServiceSearchResponse(const Record& params);
                PDU::errorid DeserializeServiceAttributeResponse(const Record& params);

            private:
                PDU::pdutype _type;
                PDU::errorid _status;
                std::list<uint32_t> _handles;
                std::map<uint16_t, Record> _attributes;
                uint8_t* _scratchPad;
                Record _payload;
                string _continuationData;
            }; // class Response

        public:
            Command(const Command&) = delete;
            Command& operator=(const Command&) = delete;
            Command()
                : _status(~0)
                , _request(/* PDU buffer size */ PDU::MIN_BUFFER_SIZE + PDU::DEFAULT_SCRATCHPAD_SIZE)
                , _response(/* receive payload buffer size */ 8192)
            {
            }
            ~Command() = default;

        public:
            void ServiceSearch(const UUID& serviceId, const uint16_t maxResults = 256) // single
            {
                ServiceSearch(std::list<UUID>{serviceId}, maxResults);
            }
            void ServiceSearch(const std::list<UUID>& services, const uint16_t maxResults = 256) // list
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceSearch(services, maxResults);
            }
            void ServiceAttribute(const uint32_t serviceHandle) // all
            {
                ServiceAttribute(serviceHandle, std::list<uint32_t>{0x0000FFFF});
            }
            void ServiceAttribute(const uint32_t serviceHandle, const uint16_t attributeId) // single
            {
                ServiceAttribute(serviceHandle, std::list<uint32_t>{(static_cast<uint32_t>(attributeId) << 16) | attributeId});
            }
            void ServiceAttribute(const uint32_t serviceHandle, const std::list<uint32_t>& attributeIdRanges) // ranges
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceAttribute(serviceHandle, attributeIdRanges);
            }

        public:
            Response& Result() {
                return (_response);
            }
            const Response& Result() const
            {
                return (_response);
            }
            uint32_t Status() const
            {
                return(_status);
            }
            bool IsValid() const
            {
                return (_request.IsValid());
            }
            void Status(const uint32_t code)
            {
                _status = code;
            }

        private:
            void Reload() const override
            {
                _request.Reload();
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                return (_request.Serialize(stream, length));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
            {
                uint16_t result = _response.Deserialize(_request.TransactionId(), stream, length);
                if ((_response.Continuation().empty() == false)) {
                    // Will be retriggered, so update transaction ID and continuation
                    _request.Finalize(_response.Continuation());
                }
                return (result);
            }
            Core::IInbound::state IsCompleted() const override
            {
                if (_response.Continuation().empty() == false) {
                    return (Core::IInbound::RESEND);
                } else {
                    return (_response.Type() != PDU::Invalid? Core::IInbound::COMPLETED : Core::IInbound::INPROGRESS);
                }
            }

        private:
            uint32_t _status;
            Request _request;
            Response _response;
        }; // class Command

    private:
        typedef std::function<void(const Command&)> Handler;

        class Entry {
        public:
            Entry() = delete;
            Entry(const Entry&) = delete;
            Entry& operator= (const Entry&) = delete;
            Entry(const uint32_t waitTime, Command& cmd, const Handler& handler)
                : _waitTime(waitTime)
                , _cmd(cmd)
                , _handler(handler)
            {
            }
            ~Entry() = default;

        public:
            Command& Cmd()
            {
                return (_cmd);
            }
            uint32_t WaitTime() const
            {
                return (_waitTime);
            }
            bool operator==(const Core::IOutbound* rhs) const
            {
                return (rhs == &_cmd);
            }
            bool operator!=(const Core::IOutbound* rhs) const
            {
                return (!operator==(rhs));
            }
            void Completed(const uint32_t error_code)
            {
                _cmd.Status(error_code);
                _handler(_cmd);
            }

        private:
            uint32_t _waitTime;
            Command& _cmd;
            Handler _handler;
        }; // class Entry

    public:
        SDPSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t maxMTU)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, maxMTU, maxMTU)
            , _adminLock()
            , _callback(*this)
            , _queue()
        {
        }
        ~SDPSocket() = default;

    public:
        void Execute(const uint32_t waitTime, Command& cmd, const Handler& handler)
        {
            _adminLock.Lock();

            if (cmd.IsValid() == true) {
                _queue.emplace_back(waitTime, cmd, handler);
                if (_queue.size() == 1) {
                    Send(waitTime, cmd, &_callback, &cmd);
                }
            } else {
                cmd.Status(Core::ERROR_BAD_REQUEST);
                handler(cmd);
            }

            _adminLock.Unlock();
        }
        void Revoke(const Command& cmd)
        {
            Revoke(cmd);
        }

    private:
        virtual void Operational() = 0;

        void StateChange() override
        {
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();

            if (IsOpen() == true) {
                socklen_t len = sizeof(_connectionInfo);
                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);

                Operational();
            }
        }

        uint16_t Deserialize(const uint8_t dataFrame[], const uint16_t availableData) override
        {
            uint32_t result = 0;

            if (availableData == 0) {
                TRACE_L1("Unexpected data for deserialization [%d]", availableData);
            }

            return (result);
        }
        void CommandCompleted(const Core::IOutbound& data, const uint32_t error_code)
        {
            _adminLock.Lock();

            if ((_queue.size() == 0) || (*(_queue.begin()) != &data)) {
                ASSERT (false && _T("Always the first one should be the one to be handled!!"));
            }
            else {
                _queue.begin()->Completed(error_code);
                _queue.erase(_queue.begin());

                if (_queue.size() > 0) {
                    Entry& entry(*(_queue.begin()));
                    Command& cmd (entry.Cmd());

                    Send(entry.WaitTime(), cmd, &_callback, &cmd);
                }
            }

            _adminLock.Unlock();
        }

    private:
        Core::CriticalSection _adminLock;
        Callback _callback;
        std::list<Entry> _queue;
        struct l2cap_conninfo _connectionInfo;
    }; // class SDPSocket

} // namespace Bluetooth

} // namespace WPEFramework
