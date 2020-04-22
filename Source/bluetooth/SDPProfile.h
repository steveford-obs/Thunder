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

#include "SDPSocket.h"


namespace WPEFramework {

namespace Bluetooth {

    class SDPProfile {
    public:
        class ClassID {
        public:
            enum id : uint16_t {
                Undefined   = 0x0000,

                // Protocols
                SDP         = 0x0001,
                UDP         = 0x0002,
                RFCOMM      = 0x0003,
                TCP         = 0x0004,
                TCS_BIN     = 0x0005,
                TCS_AT      = 0x0006,
                ATT         = 0x0007,
                OBEX        = 0x0008,
                IP          = 0x0009,
                FTP         = 0x000a,
                HTTP        = 0x000c,
                WSP         = 0x000e,
                BNEP        = 0x000f,
                UPNP        = 0x0010,
                HIDP        = 0x0011,
                HCRP_CTRL   = 0x0012,
                HCRP_DATA   = 0x0014,
                HCRP_NOTE   = 0x0016,
                AVCTP       = 0x0017,
                AVDTP       = 0x0019,
                CMTP        = 0x001b,
                UDI         = 0x001d,
                MCAP_CTRL   = 0x001e,
                MCAP_DATA   = 0x001f,
                L2CAP       = 0x0100,

                // SDP itself
                ServiceDiscoveryServerServiceClassID    = 0x1000, // Service
                BrowseGroupDescriptorServiceClassID     = 0x1001, // Service
                PublicBrowseRoot                        = 0x1002, // Service

                // Services and Profiles
                SerialPort                              = 0x1101, // Service + Profile
                LANAccessUsingPPP                       = 0x1102, // Service + Profile
                DialupNetworking                        = 0x1103, // Service + Profile
                IrMCSync                                = 0x1104, // Service + Profile
                OBEXObjectPush                          = 0x1105, // Service + Profile
                OBEXFileTransfer                        = 0x1106, // Service + Profile
                IrMCSyncCommand                         = 0x1107, // Service
                HeadsetHSP                              = 0x1108, // Service + Profile
                CordlessTelephony                       = 0x1109, // Service + Profile
                AudioSource                             = 0x110A, // Service
                AudioSink                               = 0x110B, // Service
                AVRemoteControlTarget                   = 0x110C, // Service
                AdvancedAudioDistribution               = 0x110D, //           Profile
                AVRemoteControl                         = 0x110E, // Service + Profile
                AVRemoteControlController               = 0x110F, // Service
                Intercom                                = 0x1110, // Service + Profile
                Fax                                     = 0x1111, // Service + Profile
                HeadsetAudioGateway                     = 0x1112, // Service
                WAP                                     = 0x1113, // Service
                WAPClient                               = 0x1114, // Service
                PANU                                    = 0x1115, // Service + Profile
                NAP                                     = 0x1116, // Service + Profile
                GN                                      = 0x1117, // Service + Profile
                DirectPrinting                          = 0x1118, // Service
                ReferencePrinting                       = 0x1119, // Service
                BasicImagingProfile                     = 0x111A, //           Profile
                ImagingResponder                        = 0x111B, // Service
                ImagingAutomaticArchive                 = 0x111C, // Service
                ImagingReferencedObjects                = 0x111D, // Service
                Handsfree                               = 0x111E, // Service + Profile
                HandsfreeAudioGateway                   = 0x111F, // Service
                DirectPrintingReferenceObjects          = 0x1120, // Service
                ReflectedUI                             = 0x1121, // Service
                BasicPrinting                           = 0x1122, //           Profile
                PrintingStatus                          = 0x1123, // Service
                HumanInterfaceDeviceService             = 0x1124, // Service + Profile
                HardcopyCableReplacement                = 0x1125, //           Profile
                HCRPrint                                = 0x1126, // Service
                HCRScan                                 = 0x1127, // Service
                CommonISDNAccess                        = 0x1128, // Service + Profile
                SIMAccess                               = 0x112D, // Service + Profile
                PhonebookAccessPCE                      = 0x112E, // Service
                PhonebookAccessPSE                      = 0x112F, // Service
                PhonebookAccess                         = 0x1130, //           Profile
                HeadsetHS                               = 0x1131, // Service
                MessageAccessServer                     = 0x1132, // Service
                MessageNotificationServer               = 0x1133, // Service
                MessageAccess                           = 0x1134, //           Profile
                GNSS                                    = 0x1135, //           Profile
                GNSSServer                              = 0x1136, // Service
                ThreeDDisplay                           = 0x1137, // Service
                ThreeDGlasses                           = 0x1138, // Service
                ThreeDSynchronisation                   = 0x1339, //           Profile
                MPS                                     = 0x113A, //           Profile
                MPSSC                                   = 0x113B, // Service
                CTNAccessService                        = 0x113C, // Service
                CTNNotificationService                  = 0x113D, // Service
                CTN                                     = 0x113E, //           Profile
                PnPInformation                          = 0x1200, // Service
                GenericNetworking                       = 0x1201, // Service
                GenericFileTransfer                     = 0x1202, // Service
                GenericAudio                            = 0x1203, // Service
                GenericTelephony                        = 0x1204, // Service
                UPNPService                             = 0x1205, // Service
                UPNPIPService                           = 0x1206, // Service
                ESDPUPNPIPPAN                           = 0x1300, // Service
                ESDPUPNPIPLAP                           = 0x1301, // Service
                ESDPUPNPL2CAP                           = 0x1302, // Service
                VideoSource                             = 0x1303, // Service
                VideoSink                               = 0x1304, // Service
                VideoDistribution                       = 0x1305, //          Profile
                HDP                                     = 0x1400, //          Profile
                HDPSource                               = 0x1401, // Service
                HDPSink                                 = 0x1402  // Service
            };

        public:
            ClassID(const id& classId)
                : _id(UUID(classId))
            {
            }
            ClassID(const UUID& uuid)
                : _id(uuid)
            {
            }
            ~ClassID() = default;

        public:
            const UUID& Type() const
            {
                return (_id);
            }
            const string Name() const
            {
                string name;
                if (_id.HasShort() == true) {
                    id input = static_cast<id>(_id.Short());
                    Core::EnumerateType<id> value(input);
                    name = (value.IsSet() == true? string(value.Data()) : _id.ToString(false));
                }
                if (name.empty() == true) {
                    name = _id.ToString();
                }
                return (name);
            }

        private:
            UUID _id;
        };

    public:
        class ClassDescriptor : public ClassID {
            // Describes a class that a service conforms to.
        public:
            ClassDescriptor(const UUID& id)
                : ClassID(id)
            {
            }
            ~ClassDescriptor() = default;
        }; // class ClassDescriptor

    public:
        class ProfileDescriptor : public ClassID {
            // Describes a profile the service conforms to.
        public:
            ProfileDescriptor(const UUID& id, const uint16_t version = 0x0100)
                : ClassID(id)
                , _version(version)
            {
            }
            ~ProfileDescriptor() = default;

        public:
            uint16_t Version() const
            {
                return (_version);
            }

        private:
            uint16_t _version;
        }; // class ProfileDescriptor

    public:
        class ProtocolDescriptor : public ClassID {
            // Describes a protocol stack that can be used to gain access to the service.
        public:
            ProtocolDescriptor(const UUID& id, const SDPSocket::Record& parameters)
                : ClassID(id)
                , _parameters(parameters)
            {
            }
            ~ProtocolDescriptor() = default;

        public:
            const SDPSocket::Record& Parameters() const
            {
                return (_parameters);
            }

        private:
            SDPSocket::Record _parameters;
        }; // class ProtocolDescriptor

    public:
        typedef std::function<void(const uint32_t)> Handler;

    public:
        class Service {
            friend class SDPProfile;

        public:
            class AttributeDescriptor {
            public:
                enum id : uint16_t {
                    // universal attributes
                    ServiceRecordHandle             = 0x0000,
                    ServiceClassIDList              = 0x0001,
                    ServiceRecordState              = 0x0002,
                    ServiceID                       = 0x0003,
                    ProtocolDescriptorList          = 0x0004,
                    BrowseGroupList                 = 0x0005,
                    LanguageBaseAttributeIDList     = 0x0006,
                    ServiceInfoTimeToLive           = 0x0007,
                    ServiceAvailability             = 0x0008,
                    BluetoothProfileDescriptorList  = 0x0009,
                    DocumentationURL                = 0x000a,
                    ClientExecutableURL             = 0x000b,
                    IconURL                         = 0x000c
                };

            public:
                AttributeDescriptor(const uint16_t id, const SDPSocket::Record& value)
                    : _id(id)
                    , _value(value)
                {
                }
                ~AttributeDescriptor() = default;

                uint32_t Type() const
                {
                    return (_id);
                }
                const SDPSocket::Record& Value() const
                {
                    return (_value);
                }

            private:
                uint16_t _id;
                SDPSocket::Record _value;
            }; // class AttributeDescriptor

        public:
            Service(const uint32_t handle)
                : _handle(handle)
                , _attributes()
                , _classes()
                , _profiles()
                , _protocols()
            {
            }
            ~Service() = default;

        public:
            uint32_t Handle() const
            {
                return (_handle);
            }
            const std::list<ClassDescriptor>& Classes() const
            {
                return (_classes);
            }
            const std::list<ProfileDescriptor>& Profiles() const
            {
                return (_profiles);
            }
            const std::list<ProtocolDescriptor>& Protocols() const
            {
                return (_protocols);
            }

        public:
            bool IsClassSupported(const UUID& uuid) const
            {
                return (std::any_of(_classes.cbegin(), _classes.cend(), [&](const ClassDescriptor& p) { return (p.Type() == uuid); }));
            }
            const AttributeDescriptor* Attribute(const uint16_t index) const
            {
                auto const it = _attributes.find(index);
                return (it != _attributes.end()? &(*it).second : nullptr);
            }
            const ProfileDescriptor* Profile(const UUID& uuid) const
            {
                auto const it = std::find_if(_profiles.cbegin(), _profiles.cend(), [&](const ProfileDescriptor& p) { return (p.Type() == uuid); });
                return (it == _profiles.cend()? nullptr : &(*it));
            }
            const ProtocolDescriptor* Protocol(const UUID& uuid) const
            {
                auto const it = std::find_if(_protocols.cbegin(), _protocols.cend(), [&](const ProtocolDescriptor& p) { return (p.Type() == uuid); });
                return (it == _protocols.cend()? nullptr : &(*it));
            }

        private:
            void AddAttribute(const uint16_t id, const SDPSocket::Record& value)
            {
                // Lets deserialize some of the universal attributes...
                switch (static_cast<AttributeDescriptor::id>(id)) {
                case AttributeDescriptor::ServiceRecordHandle:
                    value.Pop(SDPSocket::use_descriptor, _handle);
                    break;
                case AttributeDescriptor::id::ServiceClassIDList:
                    value.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Record& sequence) {
                        while (sequence.Available()) {
                            UUID uuid;
                            sequence.Pop(SDPSocket::use_descriptor, uuid);
                            _classes.emplace_back(uuid);
                        }
                    });
                    break;
                case AttributeDescriptor::BluetoothProfileDescriptorList:
                    value.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Record& sequence) {
                        while (sequence.Available()) {
                            sequence.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Record& descriptor) {
                                UUID uuid;
                                uint16_t version;
                                descriptor.Pop(SDPSocket::use_descriptor, uuid);
                                descriptor.Pop(SDPSocket::use_descriptor, version);
                                _profiles.emplace_back(uuid, version);
                            });
                        }
                    });
                    break;
                case AttributeDescriptor::ProtocolDescriptorList:
                    value.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Record& sequence) {
                        while (sequence.Available()) {
                            sequence.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Record& descriptor) {
                                UUID uuid;
                                SDPSocket::Record params;
                                descriptor.Pop(SDPSocket::use_descriptor, uuid);
                                descriptor.Pop(SDPSocket::use_descriptor, params);
                                _protocols.emplace_back(uuid, params);
                            });
                        }
                    });
                    break;
                default:
                    break;
                }

                _attributes.emplace(std::piecewise_construct,
                                    std::forward_as_tuple(id),
                                    std::forward_as_tuple(id, value));
            }

        private:
            uint32_t _handle;
            std::map<uint16_t, AttributeDescriptor> _attributes;
            std::list<ClassDescriptor> _classes;
            std::list<ProfileDescriptor> _profiles;
            std::list<ProtocolDescriptor> _protocols;
        }; // class Service

    public:
        SDPProfile(const ClassID& id)
            : _socket(nullptr)
            , _classId(id)
            , _command()
            , _handler(nullptr)
            , _services()
            , _servicesIterator(_services.end())
            , _expired(0)
        {
        }
        SDPProfile(const SDPProfile&) = delete;
        SDPProfile& operator=(const SDPProfile&) = delete;
        ~SDPProfile() = default;

    public:
        const ClassID& Class() const
        {
            return (_classId);
        }
        uint32_t Discover(const uint32_t waitTime, SDPSocket& socket, const std::list<UUID>& uuids, const Handler& handler)
        {
            using PDU = SDPSocket::Command::PDU;

            uint32_t result = Core::ERROR_INPROGRESS;

            _handler = handler;
            _socket = &socket;
            _expired = Core::Time::Now().Add(waitTime).Ticks();

            // Firstly, pick up available services
            _command.ServiceSearch(uuids);
            _socket->Execute(waitTime, _command, [&](const SDPSocket::Command& cmd) {
                if ((cmd.Status() == Core::ERROR_NONE)
                        && (cmd.Result().Status() == PDU::Success)
                        && (cmd.Result().Type() == PDU::ServiceSearchResponse)) {
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
        void ServiceSearchFinished(const SDPSocket::Command::Response& response)
        {
            if (response.Handles().empty() == false) {
                for (uint32_t const& handle : response.Handles()) {
                    _services.emplace_back(handle);
                }

                _servicesIterator = _services.begin();
                RetrieveAttributes();
            } else {
                Report(Core::ERROR_NONE);
            }
        }
        void RetrieveAttributes()
        {
            // Secondly, for each service pick up attributes
            if (_servicesIterator != _services.end()) {
                const uint32_t waitTime = AvailableTime();
                if (waitTime > 0) {
                    _command.ServiceAttribute((*_servicesIterator).Handle());
                    _socket->Execute(waitTime, _command, [&](const SDPSocket::Command& cmd) {
                        if ((cmd.Status() == Core::ERROR_NONE)
                            && (cmd.Result().Status() == SDPSocket::Command::PDU::Success)
                            && (cmd.Result().Type() == SDPSocket::Command::PDU::ServiceAttributeResponse)) {
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
        void ServiceAttributeFinished(const SDPSocket::Command::Response& response)
        {
            for (auto const& attr : response.Attributes()) {
                (*_servicesIterator).AddAttribute(attr.first, attr.second);
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
        ClassID _classId;
        SDPSocket::Command _command;
        Handler _handler;
        std::list<Service> _services;
        std::list<Service>::iterator _servicesIterator;
        uint64_t _expired;
    }; // class SDPProfile

    class A2DPProfile : public SDPProfile {
    public:
        A2DPProfile(SDPSocket& socket)
            : SDPProfile(ClassID::AdvancedAudioDistribution)
            , _socket(socket)
            , _handler()
            , _audioServices()
        {
        }
        A2DPProfile(const A2DPProfile&) = delete;
        A2DPProfile& operator=(const A2DPProfile&) = delete;
        ~A2DPProfile() = default;

    public:
        class AudioService {
        private:
            enum attributeid : uint16_t {
                SupportedFeatures = 0x0311
            };

        public:
            enum type {
                INVALID = 0,
                SOURCE  = 1,
                SINK    = 2
            };

            enum features : uint16_t {
                NONE        = 0,
                HEADPHONE   = (1 << 1),
                SPEAKER     = (1 << 2),
                RECORDER    = (1 << 3),
                AMPLIFIER   = (1 << 4),
                PLAYER      = (1 << 5),
                MICROPHONE  = (1 << 6),
                TUNER       = (1 << 7),
                MIXER       = (1 << 8)
            };

        public:
            AudioService(const Service& service)
                : _l2capPsm(0)
                , _avdtpVersion(0)
                , _a2dpVersion(0)
                , _features(NONE)
                , _type(SOURCE)
            {
                const ProfileDescriptor* a2dp = service.Profile(ClassID::AdvancedAudioDistribution);
                ASSERT(a2dp != nullptr);
                if (a2dp != nullptr) {
                    _a2dpVersion = a2dp->Version();
                    ASSERT(_a2dpVersion != 0);

                    const ProtocolDescriptor* l2cap = service.Protocol(ClassID::L2CAP);
                    ASSERT(l2cap != nullptr);
                    if (l2cap != nullptr) {
                        SDPSocket::Record params(l2cap->Parameters());
                        params.Pop(SDPSocket::use_descriptor, _l2capPsm);
                        ASSERT(_l2capPsm != 0);

                        const ProtocolDescriptor* avdtp = service.Protocol(ClassID::AVDTP);
                        ASSERT(avdtp != nullptr);
                        if (avdtp != nullptr) {
                            SDPSocket::Record params(avdtp->Parameters());
                            params.Pop(SDPSocket::use_descriptor, _avdtpVersion);
                            ASSERT(_avdtpVersion != 0);

                            // By now it's A2DP service using L2CAP and AVDTP protocols; finally confirm class ID
                            if (service.IsClassSupported(ClassID::AudioSink)) {
                                _type = SINK;
                            } else if (service.IsClassSupported(ClassID::AudioSource)) {
                                _type = SOURCE;
                            }

                            // This one is optional...
                            const Service::AttributeDescriptor* supportedFeatures = service.Attribute(SupportedFeatures);
                            if (supportedFeatures != nullptr) {
                                SDPSocket::Record value(supportedFeatures->Value());
                                value.Pop(SDPSocket::use_descriptor, _features);
                                if (service.IsClassSupported(ClassID::AudioSource)) {
                                    _features = static_cast<features>((static_cast<uint8_t>(_features) << 4));
                                }
                            }
                        }
                    }
                }
            }
            AudioService(const AudioService&) = delete;
            AudioService& operator=(const AudioService&) = delete;
            ~AudioService() = default;

        public:
            type Type() const
            {
                return (_type);
            }
            uint16_t PSM() const
            {
                return (_l2capPsm);
            }
            uint16_t TransportVersion() const
            {
                return (_avdtpVersion);
            }
            uint16_t ProfileVersion() const
            {
                return (_a2dpVersion);
            }
            features Features() const
            {
                return (_features);
            }

        private:
            UUID _class;
            uint16_t _l2capPsm;
            uint16_t _avdtpVersion;
            uint16_t _a2dpVersion;
            features _features;
            type _type;
        }; // class AudioService

    public:
        uint32_t Discover(const uint32_t waitTime, const AudioService::type& serviceType, const Handler& handler)
        {
            _handler = handler;

            return (SDPProfile::Discover(waitTime, _socket, std::list<UUID>{ serviceType == AudioService::SINK? ClassID::AudioSink : ClassID::AudioSource }, [&](const uint32_t status) {
                for (auto const& service : Services()) {
                    _audioServices.emplace_back(service);
                }

                // All SDP parsing done, pass the result to the caller finally.
                Handler caller = _handler;
                _handler = nullptr;
                caller(status);
            }));
        }

    public:
        const std::list<AudioService>& AudioServices() const
        {
            return (_audioServices);
        }

    private:
        SDPSocket& _socket;
        Handler _handler;
        std::list<AudioService> _audioServices;
    }; // class A2DPProfile

} // namespace Bluetooth

}