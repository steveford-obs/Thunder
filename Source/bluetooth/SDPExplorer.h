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

    class Explorer {
    public:
        enum serviceid : uint16_t {
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
            friend class Explorer;

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
                : _serviceRecordHandle(handle)
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
                return (_serviceRecordHandle);
            }
            const std::list<UUID>& ServiceClassIDList() const
            {
                return (_serviceClassIDList);
            }
            const std::list<std::pair<UUID, uint16_t>>& BluetoothProfileDescriptorList() const
            {
                return (_bluetoothProfileDescriptorList);
            }

        private:
            void Attribute(const uint16_t id, const string& value)
            {
                using PDU = SDPSocket::Command::PDU;

                PDU::Serializer data((uint8_t*)value.data(), value.size(), value.size());

                // Lets deserialize some of the universal attributes...
                switch (static_cast<attributeid>(id)) {
                case attributeid::ServiceRecordHandle:
                    data.Pop(_serviceRecordHandle, true);
                    break;
                case attributeid::ServiceClassIDList:
                    data.Pop([&](const PDU::Serializer& sequence) {
                        while (sequence.Available()) {
                            UUID uuid;
                            sequence.Pop(uuid);
                            _serviceClassIDList.emplace_back(uuid);
                        }
                    });
                    break;
                case attributeid::BluetoothProfileDescriptorList:
                    data.Pop([&](const PDU::Serializer& sequence) {
                        sequence.Pop([&](const PDU::Serializer& pair) {
                            UUID uuid;
                            uint16_t version;
                            pair.Pop(uuid);
                            pair.Pop(version, true);
                            _bluetoothProfileDescriptorList.emplace_back(std::make_pair(uuid, version));
                        });
                    });
                default:
                    // Just store the rest, let interested parties deserialize when required
                    _attributes.emplace(id, value);
                    break;
                }
            }

        private:
            uint32_t _serviceRecordHandle;
            std::map<uint16_t, string> _attributes;
            std::list<UUID> _serviceClassIDList;
            std::list<std::pair<UUID, uint16_t>> _bluetoothProfileDescriptorList;
        }; // class Service

    public:
        Explorer()
            : _socket(nullptr)
            , _command()
            , _handler(nullptr)
            , _services()
            , _servicesIterator(_services.end())
            , _expired(0)
        {
        }
        ~Explorer() = default;

    public:
        uint32_t Discover(const uint32_t waitTime, SDPSocket& socket, const std::list<UUID>& uuids, const Handler& handler)
        {
            using PDU = SDPSocket::Command::PDU;

            uint32_t result = Core::ERROR_INPROGRESS;

            _handler = handler;
            _socket = &socket;
            _expired = Core::Time::Now().Add(waitTime).Ticks();

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
    }; // class Explorer

}

}