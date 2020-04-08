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

        class Command : public Core::IOutbound, public Core::IInbound {
        public:
            class PDU {
            public:
                static constexpr uint8_t HEADER_SIZE = 5;

                class Serializer {
                private:
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
                    using Builder = std::function<void(Serializer&)>;
                    using Inspector = std::function<void(const Serializer&)>;

                    enum class Continuation : uint8_t {
                        ABSENT = 0,
                        FOLLOWS
                    };

                public:
                    Serializer(uint8_t buffer[], const uint32_t bufferSize, const uint32_t filledSize = 0)
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

                    virtual ~Serializer() = default;

                public:
                    uint32_t Length() const
                    {
                        return (_writerOffset);
                    }
                    const uint8_t* Data() const
                    {
                        return (_buffer);
                    }
                    void Clear()
                    {
                        _writerOffset = 0;
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
                        }
                    }

                public:
                    void Push(const bool descriptor = false)
                    {
                        if (descriptor == true) {
                            _writerOffset += PushDescriptor(&_buffer[_writerOffset], NIL);
                        }
                    }
                    void Push(const Bluetooth::UUID& value, const bool descriptor = false)
                    {
                        if (descriptor == true) {
                            _writerOffset += PushDescriptor(&_buffer[_writerOffset], UUID, value.Length());
                        }
                        uint8_t size = value.Length();
                        while (size-- > 0) {
                            _buffer[_writerOffset++] = value.Data()[size]; // reverse!
                        }
                    }
                    void Push(const string& value, const bool descriptor = false, bool url = false)
                    {
                        if (descriptor == true) {
                            _writerOffset += PushDescriptor(&_buffer[_writerOffset], (url? URL : TEXT), value.length());
                        }
                        ::memcpy(&_buffer[_writerOffset], value.data(), value.length()); // no nul-terminator!
                        _writerOffset += value.length();
                    }
                    void Push(const bool value, const bool descriptor = false)
                    {
                        if (descriptor == true) {
                            _writerOffset += PushDescriptor(&_buffer[_writerOffset], BOOL, 1);
                        }
                        Push(static_cast<uint8_t>(value));
                    }
                    template<typename TYPE, /* dummy */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
                    void Push(const TYPE value, const bool descriptor = false)
                    {
                        ASSERT(sizeof(TYPE) <= 4);
                        if (descriptor == true) {
                            _writerOffset += PushDescriptor(&_buffer[_writerOffset],
                                                            (std::numeric_limits<TYPE>::is_signed? INT : UINT),
                                                            sizeof(TYPE));
                        }
                        PushIntegerValue(static_cast<typename std::make_unsigned<TYPE>::type>(value));
                    }
                    template<typename TYPE, /* dummy */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
                    void Push(const TYPE value, const bool descriptor = false)
                    {
                        ASSERT(sizeof(TYPE) <= 4);
                        if (descriptor == true) {
                            _writerOffset += PushDescriptor(&_buffer[_writerOffset], UINT, sizeof(TYPE));
                        }
                        PushIntegerValue(static_cast<typename std::underlying_type<TYPE>::type>(value));
                    }
                    void Push(const Serializer& sequence, const bool descriptor = false, const bool alternative = false)
                    {
                        if (descriptor == true) {
                            _writerOffset += PushDescriptor(&_buffer[_writerOffset], (alternative? ALT : SEQ), sequence.Length());
                        }
                        ::memcpy(&_buffer[_writerOffset], sequence.Data(), sequence.Length());
                        _writerOffset += sequence.Length();
                    }
                    void Push(const Builder& builder, const bool descriptor = false, const bool alternative = false,
                              const uint16_t scratchPadSize = 2048)
                    {
                        uint8_t scratchPad[scratchPadSize];
                        Serializer sequence(scratchPad, sizeof(scratchPad));
                        builder(sequence);
                        Push(sequence, descriptor, alternative);
                    }
                    template<typename TYPE>
                    void Push(const std::list<TYPE>& list, const bool descriptor = false, const bool alternative = false,
                              const uint16_t scratchPadSize = 2048)
                    {
                        Push([&](Serializer& sequence){
                            for (auto item : list) {
                                sequence.Push(item, descriptor);
                            }
                        }, descriptor, alternative, scratchPadSize);
                    }

                public:
                    void Pop(string& value, const uint16_t length) const
                    {
                        value.copy(reinterpret_cast<char*>(&_buffer[_readerOffset]), length);
                        _readerOffset += length;
                    }
                    template<typename TYPE, /* dummy */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
                    void Pop(TYPE& value)
                    {
                        ASSERT(sizeof(TYPE) <= 4);
                        typename std::underlying_type<TYPE>::type temp;
                        Pop(temp);
                        value = static_cast<TYPE>(temp);
                    }
                    void Pop(uint8_t& value) const
                    {
                        value = _buffer[_readerOffset++];
                    }
                    void Pop(uint16_t& value) const
                    {
                        value = ((_buffer[_readerOffset] << 8) | _buffer[_readerOffset + 1]);
                        _readerOffset += 2;
                    }
                    void Pop(uint32_t& value) const
                    {
                        value = ((_buffer[_readerOffset] << 24) | (_buffer[_readerOffset + 1] << 16)
                                    | (_buffer[_readerOffset + 2] << 8) | _buffer[_readerOffset + 3]);
                        _readerOffset += 4;
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
                    void Pop(const Inspector& inspector) const
                    {
                        uint32_t size = 0;
                        elementtype type;
                        _readerOffset += PopDescriptor(&_buffer[_readerOffset], type, size);
                        if (type == SEQ) {
                            Serializer sequence(&_buffer[_readerOffset], size);
                            inspector(sequence);
                        }
                        _readerOffset += size;
                    }

                private:
                    uint8_t PushDescriptor(uint8_t buffer[], const elementtype type, const uint32_t size = 0);
                    uint8_t PopDescriptor(const uint8_t buffer[], elementtype& type, uint32_t& size) const;

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

                   private:
                    uint8_t* _buffer;
                    uint32_t _bufferSize;
                    uint32_t _filledSize;
                    mutable uint32_t _readerOffset;
                    uint32_t _writerOffset;
                }; // class Serializer

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
                    DeserializationFailed
                };

            public:
                PDU(const uint32_t bufferSize = 4096)
                    : _bufferSize(bufferSize)
                    , _buffer(static_cast<uint8_t*>(::malloc(_bufferSize)))
                    , _size(0)
                {
                    ASSERT(_buffer != nullptr);
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
                    ::memset(_buffer, 0, PDU::HEADER_SIZE);
                    _size = 0;
                }
                bool IsValid() const
                {
                    return ((_buffer != nullptr) && (_bufferSize > PDU::HEADER_SIZE) && (Type() != Invalid));
                }
                uint32_t Length() const
                {
                    return (_size);
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
                    return (IsValid()? ((_buffer[1] << 8) | (_buffer[2])) : 0);
                }
                void Construct(const PDU::pdutype type, const Serializer& parameters)
                {
                    ASSERT(_bufferSize >= (PDU::HEADER_SIZE + parameters.Length()));

                    Clear();

                    if (_bufferSize >= (PDU::HEADER_SIZE + parameters.Length())) {
                        PDU::Serializer header(_buffer, _bufferSize);
                        header.Push(type);
                        header.Push(static_cast<uint16_t>(0)); // transaction id not yet known
                        header.Push(static_cast<uint16_t>(parameters.Length()));

                        ::memcpy(_buffer + header.Length(), parameters.Data(), parameters.Length());
                        _size = (header.Length() + parameters.Length());
                    } else {
                        TRACE(Trace::Error, (_T("Parameters to large to fit in PDU [%d]"), parameters.Length()));
                    }
                }
                void Construct(const PDU::pdutype type, const Serializer::Builder& builder, const uint32_t scratchPadSize = 2048)
                {
                    uint8_t scratchPad[scratchPadSize];
                    Serializer parameters(scratchPad, sizeof(scratchPad));
                    builder(parameters);
                    Construct(type, parameters);
                }
                void Finalize(const uint16_t transactionId)
                {
                    ASSERT(_buffer != nullptr);
                    if (IsValid() == true) {
                        _buffer[1] = (transactionId >> 8);
                        _buffer[2] = transactionId;
                    }
                }

            private:
                uint32_t _bufferSize;
                uint8_t* _buffer;
                uint32_t _size;
            }; // class PDU

            class Request {
            public:
                Request(const Request&) = delete;
                Request& operator=(const Request&) = delete;
                Request()
                    : _pdu()
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
                bool IsSent() const
                {
                    return (_pdu.TransactionId() != 0);
                }
                void Finalize(const uint16_t transactionId)
                {
                    _pdu.Finalize(transactionId);
                }

            public:
                void ServiceSearch(const std::list<UUID>& services, const uint16_t maxResults)
                {
                    ASSERT(services.size() <= 12); // As per spec

                    _pdu.Construct(PDU::ServiceSearchRequest, [&](PDU::Serializer& parameters) {
                        parameters.Push(services, true);
                        parameters.Push(maxResults);
                        parameters.Push(PDU::Serializer::Continuation::ABSENT); // Will always fit 12 UUIDs in a single PDU
                    });
                }
                void ServiceAttribute(const uint32_t serviceHandle, const std::list<uint32_t>& attributeIdRanges)
                {
                    ASSERT(attributeIdRanges.size() <= 256);

                    _pdu.Construct(PDU::ServiceAttributeRequest, [&](PDU::Serializer& parameters) {
                        parameters.Push(serviceHandle);
                        parameters.Push(static_cast<uint16_t>(0xFFFFF - PDU::HEADER_SIZE));
                        parameters.Push(attributeIdRanges, true);
                        parameters.Push(PDU::Serializer::Continuation::ABSENT);
                    });
                }
                void ServiceSearchAttribute(const std::list<UUID>& services, const std::list<uint32_t>& attributeIdRanges)
                {
                    ASSERT(services.size() <= 12);
                    ASSERT(attributeIdRanges.size() <= 256);

                    _pdu.Construct(PDU::ServiceSearchAttributeRequest, [&](PDU::Serializer& parameters) {
                        parameters.Push(services, true);
                        parameters.Push(static_cast<uint16_t>(0xFFFFF - PDU::HEADER_SIZE));
                        parameters.Push(attributeIdRanges, true);
                        parameters.Push(PDU::Serializer::Continuation::ABSENT);
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

                typedef std::pair<uint16_t, std::pair<uint16_t, uint16_t> > Entry;

            public:
                Response()
                    : _type(PDU::Invalid)
                    , _status(PDU::Reserved)
                    , _transaction(0)
                    , _length(0)
                {
                }
                ~Response() = default;

            public:
                void Clear()
                {
                    _status = PDU::Reserved;
                    _type = PDU::Invalid;
                    _length = 0;
                    _handles.clear();
                }
                PDU::pdutype Type() const
                {
                    return (_type);
                }
                PDU::errorid Status() const
                {
                    return (_status);
                }
                uint16_t Length() const
                {
                    return (_length);
                }
                const std::list<uint32_t>& Handles() const
                {
                    return _handles;
                }
                const std::map<uint16_t, string>& Attributes() const
                {
                    return _attributes;
                }
                uint16_t Deserialize(const uint8_t stream[], const uint16_t length)
                {
                    uint16_t result = 0;

                    printf("L2CAP received [%d]: ", length);
                    for (uint8_t index = 0; index < (length - 1); index++) { printf("%02X:", stream[index]); } printf("%02X\n", stream[length - 1]);

                    if (length >= PDU::HEADER_SIZE) {
                        PDU::Serializer header(const_cast<uint8_t*>(stream), PDU::HEADER_SIZE, PDU::HEADER_SIZE);
                        header.Pop(_type);
                        header.Pop(_transaction);
                        header.Pop(_length);

                        if (length >= header.Length() + _length) {
                            PDU::Serializer parameters(const_cast<uint8_t*>(stream + header.Length()), _length, _length);

                            switch(_type) {
                            case PDU::ErrorResponse:
                                parameters.Pop(_status);
                                break;
                            case PDU::ServiceSearchResponse:
                                _status = DeserializeServiceSearchResponse(parameters);
                                break;
                            case PDU::ServiceAttributeResponse:
                                _status = DeserializeServiceAttributeResponse(parameters);
                                break;
                            case PDU::ServiceSearchAttributeResponse:
                                _status = DeserializeServicSearchAttributeResponse(parameters);
                                break;
                            default:
                                _status = PDU::DeserializationFailed;
                                break;
                            }

                            if (_status == PDU::Success) {
                                PDU::Serializer::Continuation cont;
                                string contData;
                                parameters.Pop(cont, contData);

                                if (cont != PDU::Serializer::Continuation::ABSENT) {
                                    TRACE(Trace::Error, (_T("Continuation on response not supported")));
                                     _status = PDU::DeserializationFailed;
                                }
                            }

                            result = length;
                            printf("deserialize finished\n");
                        }
                    }

                    return (result);
                }

            private:
                PDU::errorid DeserializeServiceSearchResponse(const PDU::Serializer& params)
                {
                    PDU::errorid result = PDU::DeserializationFailed;

                    ASSERT(Type() == PDU::ServiceSearchResponse);

                    if (params.Length() >= 5) {
                        uint16_t totalCount = 0;
                        uint16_t currentCount = 0;

                        params.Pop(totalCount);
                        params.Pop(currentCount);
                        params.Pop(_handles, currentCount);

                        if (currentCount == _handles.size()) {
                            result = PDU::Success;
                        }
                    }

                    return (result);
                }
                PDU::errorid DeserializeServiceAttributeResponse(const PDU::Serializer& params)
                {
                    PDU::errorid result = PDU::DeserializationFailed;

                    ASSERT(Type() == PDU::ServiceAttributeResponse);

                    if (params.Length() >= 2) {
                        uint16_t byteCount = 0;
                        params.Pop(byteCount);
                        params.Pop([&](const PDU::Serializer& sequence) {
                            // TODO
                        });
                    }

                    return (result);
                }
                PDU::errorid DeserializeServicSearchAttributeResponse(const PDU::Serializer& params)
                {
                    PDU::errorid result = PDU::DeserializationFailed;

                    ASSERT(Type() == PDU::ServiceSearchAttributeResponse);

                    if (params.Length() >= 2) {
                        // TODO
                    }

                    return (result);
                }

            private:
                PDU::pdutype _type;
                PDU::errorid _status;
                uint16_t _transaction;
                uint16_t _length;
                std::list<uint32_t> _handles;
                std::map<uint16_t, string> _attributes;
            }; // class Response

        public:
            Command(const Command&) = delete;
            Command& operator=(const Command&) = delete;
            Command()
                : _status(~0)
                , _request()
                , _response()
            {
            }
            ~Command() = default;

        public:
            void ServiceSearch(const UUID& serviceId, const uint16_t maxResults = 256) // single
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceSearch(std::list<UUID>{serviceId}, maxResults);
            }
            void ServiceSearch(const std::list<UUID>& services, const uint16_t maxResults = 256) // list
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceSearch(services, maxResults);
            }
            void ServiceAttribute(const uint32_t serviceHandle) // all
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceAttribute(serviceHandle, std::list<uint32_t>{0x0000FFFF});
            }
            void ServiceAttribute(const uint32_t serviceHandle, const uint16_t attributeId) // single
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceAttribute(serviceHandle, std::list<uint32_t>{(static_cast<uint32_t>(attributeId) << 16) | attributeId});
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
            void Finalize(const uint16_t transactionId)
            {
                _request.Finalize(transactionId);
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
                return (_response.Deserialize(stream, length));
            }
            Core::IInbound::state IsCompleted() const override
            {
                return (_response.Type() != PDU::Invalid? Core::IInbound::COMPLETED
                                                        : (_request.IsSent() ? Core::IInbound::INPROGRESS : Core::IInbound::RESEND));
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
        class Profile {
        public:
            enum serviceid {
		        ServiceDiscoveryServerServiceClassID = 0x1000,
                BrowseGroupDescriptorServiceClassID = 0x1001,
                PublicBrowseRoot = 0x1002,
                SerialPort = 0x1101,
                LANAccessUsingPPP = 0x1102,
                DialupNetworking = 0x1103,
                IrMCSync = 0x1104,
                OBEXObjectPush = 0x1105,
                OBEXFileTransfer = 0x1106,
                IrMCSyncCommand = 0x1107,
                HeadsetHSP = 0x1108,
                CordlessTelephony = 0x1109,
                AudioSource = 0x110A,
                AudioSink = 0x110B,
                AVRemoteControlTarget = 0x110C,
                AdvancedAudioDistribution = 0x110D,
                AVRemoteControl = 0x110E,
                AVRemoteControlController = 0x110F,
                Intercom = 0x1110,
                Fax = 0x1111,
                HeadsetAudioGateway = 0x1112,
                WAP = 0x1113,
                WAPClient = 0x1114,
                PANU = 0x1115,
                NAP = 0x1116,
                GN = 0x1117,
                DirectPrinting = 0x1118,
                ReferencePrinting = 0x1119,
                BasicImagingProfile = 0x111A,
                ImagingResponder = 0x111B,
                ImagingAutomaticArchive = 0x111C,
                ImagingReferencedObjects = 0x111D,
                Handsfree = 0x111E,
                HandsfreeAudioGateway = 0x111F,
                DirectPrintingReferenceObjectsService = 0x1120,
                ReflectedUI = 0x1121,
                BasicPrinting = 0x1122,
                PrintingStatus = 0x1123,
                HumanInterfaceDeviceService = 0x1124,
                HardcopyCableReplacement = 0x1125,
                HCRPrint = 0x1126,
                HCRScan = 0x1127,
                CommonISDNAccess = 0x1128,
                SIMAccess = 0x112D,
                PhonebookAccessPCE = 0x112E,
                PhonebookAccessPSE = 0x112F,
                PhonebookAccess = 0x1130,
                HeadsetHS = 0x1131,
                MessageAccessServer = 0x1132,
                MessageNotificationServer = 0x1133,
                MessageAccessProfile = 0x1134,
                GNSS = 0x1135,
                GNSSServer = 0x1136,
                ThreeDDisplay = 0x1137,
                ThreeDGlasses = 0x1138,
                ThreeDSynchronisation = 0x1339,
                MPSProfile = 0x113A,
                MPSSC = 0x113B,
                CTNAccessService = 0x113C,
                CTNNotificationService = 0x113D,
                CTNProfile= 0x113E,
                PnPInformation = 0x1200,
                GenericNetworking = 0x1201,
                GenericFileTransfer = 0x1202,
                GenericAudio = 0x1203,
                GenericTelephony = 0x1204,
                UPNPService = 0x1205,
                UPNPIPService = 0x1206,
                ESDPUPNPIPPAN = 0x1300,
                ESDPUPNPIPLAP = 0x1301,
                ESDPUPNPL2CAP = 0x1302,
                VideoSource = 0x1303,
                VideoSink = 0x1304,
                VideoDistribution = 0x1305,
                HDP = 0x1400,
                HDPSource = 0x1401,
                HDPSink = 0x1402
            };

        public:
            typedef std::function<void(const uint32_t)> Handler;

            class Service {
                friend class Profile;

            private:
                enum class attributeid : uint16_t {
                    // universal attributes
                    ServiceRecordHandle = 0,
                    ServiceClassIDList = 1,
                    ServiceRecordState = 2,
                    ServiceID = 3,
                    ProtocolDescriptorList = 4,
                    BrowseGroupList = 5,
                    LanguageBaseAttributeIDList = 6,
                    ServiceInfoTimeToLive = 7,
                    ServiceAvailability = 8,
                    BluetoothProfileDescriptorList = 9,
                    DocumentationURL = 10,
                    ClientExecutableURL = 11,
                    IconURL = 12
                };

            public:
                Service(const uint32_t handle)
                    : _recordHandle(handle)
                {
                }
                ~Service() = default;

            public:
                bool HasAttribute(uint16_t index) const
                {
                    auto it = _attributes.find(index);
                    return (it != _attributes.end());
                }
                const string& Attribute(uint16_t index) const
                {
                    static string empty{};
                    auto it = _attributes.find(index);
                    return (it != _attributes.end()? (*it).second : empty);
                }

            public:
                uint32_t ServiceRecordHandle() const
                {
                    return (_recordHandle);
                }
                const std::list<UUID>& ServiceClassIDList() const
                {
                    return (_classIDList);
                }

            private:
                void Attribute(const uint16_t id, const string& data)
                {
                    _attributes.emplace(id, data);
                    // TODO
                }

            private:
                uint32_t _recordHandle;
                std::list<UUID> _classIDList;
                std::map<uint16_t, string> _attributes;
            }; // class Service

        public:
            Profile()
                : _socket(nullptr)
                , _command()
                , _handler(nullptr)
                , _services()
                , _servicesIterator(_services.end())
                , _expired(0)
            {
            }
            ~Profile() = default;

        public:
            uint32_t Discover(const uint32_t waitTime, SDPSocket& socket, const std::list<UUID>& uuids, const Handler& handler)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                _handler = handler;
                _socket = &socket;
                _services.clear();
                _expired = Core::Time::Now().Add(waitTime).Ticks();

                _command.ServiceSearch(uuids);
                _socket->Execute(waitTime, _command, [&](const SDPSocket::Command& cmd) {
                    if ((cmd.Status() == Core::ERROR_NONE)
                            && (cmd.Result().Status() == Command::PDU::Success)
                            && (cmd.Result().Type() == Command::PDU::ServiceSearchResponse)) {
                                ServiceSearchFinished(cmd.Result());
                    } else {
                        Report(Core::ERROR_GENERAL);
                    }
                });

                return (result);
            }
            const std::list<Service>& Services() const
            {
                return (_services);
            }

        private:
            void ServiceSearchFinished(const Command::Response& response)
            {
                if (response.Handles().empty() == false) {
                    for (uint32_t const& handle : response.Handles()) {
                        _services.emplace_back(handle);
                    }

                    _servicesIterator = _services.begin();
                    RetrieveAttributes();
                } else {
                    Report(Core::ERROR_UNAVAILABLE);
                }
            }
            void RetrieveAttributes()
            {
                if (_servicesIterator != _services.end()) {
                    const uint32_t waitTime = AvailableTime();
                    if (waitTime > 0) {
                        _command.ServiceAttribute((*_servicesIterator).ServiceRecordHandle());
                        _socket->Execute(waitTime, _command, [&](const SDPSocket::Command& cmd) {
                            if ((cmd.Status() == Core::ERROR_NONE)
                                && (cmd.Result().Status() == Command::PDU::Success)
                                && (cmd.Result().Type() == Command::PDU::ServiceAttributeResponse)) {
                                    ServiceAttributeFinished(cmd.Result());
                            } else {
                                Report(Core::ERROR_GENERAL);
                            }
                        });
                    }
                } else {
                    Report(Core::ERROR_NONE);
                }
            }
            void ServiceAttributeFinished(const Command::Response& response)
            {
                for (auto const& attr : response.Attributes()) {
                    (*_servicesIterator).Attribute(attr.first, attr.second);
                }

                _servicesIterator++;
                RetrieveAttributes();
            }
            void Report(const uint32_t result)
            {
                if (_socket != nullptr) {
                    Handler caller = _handler;
                    _socket = nullptr;
                    _handler = nullptr;
                    _expired = 0;

                    caller(result);
                }
            }
            uint32_t AvailableTime()
            {
                uint64_t now = Core::Time::Now().Ticks();
                uint32_t result = (now >= _expired ? 0 : static_cast<uint32_t>((_expired - now) / Core::Time::TicksPerMillisecond));
                if (result == 0) {
                    Report(Core::ERROR_TIMEDOUT);
                }
                return (result);
            }

        public:
            SDPSocket* _socket;
            SDPSocket::Command _command;
            Handler _handler;
            std::list<Service> _services;
            std::list<Service>::iterator _servicesIterator;
            uint64_t _expired;
        }; // class Profile

    public:
        SDPSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t maxMTU)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, maxMTU, maxMTU)
            , _adminLock()
            , _callback(*this)
            , _queue()
            , _transactionId(0)
        {
        }
        ~SDPSocket() = default;

    public:
        void Execute(const uint32_t waitTime, Command& cmd, const Handler& handler)
        {
            printf("Execute\n");
            _adminLock.Lock();

            if (cmd.IsValid() == true) {
                cmd.Finalize(++_transactionId);
                _queue.emplace_back(waitTime, cmd, handler);
                if (_queue.size() == 1) {
                    printf("Send\n");
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

            if (availableData >= 1) {
            }
            else {
                TRACE_L1("**** Unexpected data for deserialization [%d] !!!!", availableData);
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
        uint16_t _transactionId;
    }; // class SDPSocket

} // namespace Bluetooth

} // namespace WPEFramework
