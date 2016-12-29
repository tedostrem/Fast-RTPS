// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fastrtps/rtps/security/common/Handle.h>
#include <rtps/security/MockAuthenticationPlugin.h>
#include <rtps/security/MockCryptographyPlugin.h>
#include <rtps/participant/RTPSParticipantImpl.h>
#include <fastrtps/rtps/writer/StatelessWriter.h>
#include <fastrtps/rtps/writer/StatefulWriter.h>
#include <fastrtps/rtps/history/WriterHistory.h>
#include <fastrtps/rtps/reader/StatelessReader.h>
#include <fastrtps/rtps/reader/StatefulReader.h>
#include <fastrtps/rtps/history/ReaderHistory.h>
#include <fastrtps/rtps/builtin/data/ParticipantProxyData.h>
#include <rtps/security/SecurityPluginFactory.h>
#include <rtps/security/SecurityManager.h>

#include <gtest/gtest.h>

using namespace eprosima::fastrtps::rtps;
using namespace ::security;
using namespace ::testing;

using ::testing::DefaultValue;

class MockIdentity
{
    public:

        static const char* const class_id_;
};

const char* const MockIdentity::class_id_ = "MockIdentityHandle";

typedef HandleImpl<MockIdentity> MockIdentityHandle;

class MockHandshake
{
    public:

        static const char* const class_id_;
};

const char* const MockHandshake::class_id_ = "MockHandshakeHandle";

typedef HandleImpl<MockHandshake> MockHandshakeHandle;

class MockParticipantCrypto
{
    public:

        static const char* const class_id_;
};

const char* const MockParticipantCrypto::class_id_ = "MockParticipantCryptoHandle";

typedef HandleImpl<MockParticipantCrypto> MockParticipantCryptoHandle;

// Default Values
RTPSParticipantAttributes pattr;
GUID_t guid;

void fill_participant_key(GUID_t& participant_key)
{
    participant_key.guidPrefix.value[0] = 1;
    participant_key.guidPrefix.value[1] = 2;
    participant_key.guidPrefix.value[2] = 3;
    participant_key.guidPrefix.value[3] = 4;
    participant_key.guidPrefix.value[4] = 5;
    participant_key.guidPrefix.value[5] = 6;
    participant_key.guidPrefix.value[6] = 7;
    participant_key.guidPrefix.value[7] = 8;
    participant_key.guidPrefix.value[8] = 9;
    participant_key.guidPrefix.value[9] = 10;
    participant_key.guidPrefix.value[10] = 11;
    participant_key.guidPrefix.value[11] = 12;
    participant_key.entityId.value[0] = 0x0;
    participant_key.entityId.value[1] = 0x0;
    participant_key.entityId.value[2] = 0x1;
    participant_key.entityId.value[3] = 0xc1;
}

class SecurityAuthenticationTest : public ::testing::Test
{
    protected:

        virtual void SetUp()
        {
            SecurityPluginFactory::set_auth_plugin(auth_plugin_);
            SecurityPluginFactory::set_crypto_plugin(crypto_plugin_);
            fill_participant_key(guid);
        }

        virtual void TearDown()
        {
            SecurityPluginFactory::release_auth_plugin();
            SecurityPluginFactory::release_crypto_plugin();
        }

        void initialization_ok()
        {
            DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
            DefaultValue<const GUID_t&>::Set(guid);
            stateless_writer_ = new NiceMock<StatelessWriter>(&participant_);
            stateless_reader_ = new NiceMock<StatelessReader>();
            volatile_writer_ = new NiceMock<StatefulWriter>(&participant_);
            volatile_reader_ = new NiceMock<StatefulReader>();
            MockIdentityHandle identity_handle;
            MockIdentityHandle* p_identity_handle = &identity_handle;
            MockParticipantCryptoHandle local_participant_crypto_handle;

            EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(1).
                WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
            EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, register_local_participant(_,_,_,_)).Times(1).
                WillOnce(Return(&local_participant_crypto_handle));
            EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, unregister_participant(&local_participant_crypto_handle,_)).Times(1).
                WillOnce(Return(true));
            EXPECT_CALL(participant_, createWriter_mock(_,_,_,_,_,_)).Times(2).
                WillOnce(DoAll(SetArgPointee<0>(stateless_writer_), Return(true))).
                WillOnce(DoAll(SetArgPointee<0>(volatile_writer_), Return(true)));
            EXPECT_CALL(participant_, createReader_mock(_,_,_,_,_,_,_)).Times(2).
                WillOnce(DoAll(SetArgPointee<0>(stateless_reader_), Return(true))).
                WillOnce(DoAll(SetArgPointee<0>(volatile_reader_), Return(true)));

            ASSERT_TRUE(manager_.init());
        }

        void request_process_ok(CacheChange_t** request_message_change = nullptr)
        {
            initialization_ok();

            MockIdentityHandle identity_handle;
            MockIdentityHandle* p_identity_handle = &identity_handle;
            MockHandshakeHandle handshake_handle;
            MockHandshakeHandle* p_handshake_handle = &handshake_handle;
            HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
            CacheChange_t* change = new CacheChange_t(200);

            EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
                WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
            EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
                WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                            SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
            EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
                WillOnce(Return(change));
            EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change)).Times(1).
                WillOnce(Return(true));
            EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

            ParticipantProxyData participant_data;
            fill_participant_key(participant_data.m_guid);
            ASSERT_TRUE(manager_.discovered_participant(&participant_data));

            if(request_message_change != nullptr)
                *request_message_change = change;
            else
                delete change;
            delete p_handshake_message;
        }

        void reply_process_ok(CacheChange_t** reply_message_change = nullptr)
        {
            initialization_ok();

            MockIdentityHandle identity_handle;
            MockIdentityHandle* p_identity_handle = &identity_handle;

            EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
                WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));

            ParticipantProxyData participant_data;
            fill_participant_key(participant_data.m_guid);
            ASSERT_TRUE(manager_.discovered_participant(&participant_data));

            ParticipantGenericMessage message;
            message.message_identity().source_guid(participant_data.m_guid);
            message.destination_participant_key(participant_data.m_guid);
            message.message_class_id("dds.sec.auth");
            HandshakeMessageToken token;
            message.message_data().push_back(token);
            CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
            CDRMessage_t aux_msg(0);
            aux_msg.wraps = true;
            aux_msg.buffer = change->serializedPayload.data;
            aux_msg.max_size = change->serializedPayload.max_size;
            aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
            ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
            change->serializedPayload.length = aux_msg.length;

            MockHandshakeHandle handshake_handle;
            MockHandshakeHandle* p_handshake_handle = &handshake_handle;
            HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
            CacheChange_t* change2 = new CacheChange_t(200);

            EXPECT_CALL(*auth_plugin_, begin_handshake_reply_rvr(_,_,_,_,_,_,_)).Times(1).
                WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                            SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
            EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
                WillOnce(Return(change2));
            EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change2)).Times(1).
                WillOnce(Return(true));
            EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
                WillOnce(Return(true));
            EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

            stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

            if(reply_message_change != nullptr)
                *reply_message_change = change2;
            else
                delete change2;
            delete p_handshake_message;
        }

        void final_message_process_ok(CacheChange_t** final_message_change = nullptr)
        {
            request_process_ok();

            EXPECT_CALL(*stateless_writer_->history_, remove_change(SequenceNumber_t{0,1})).Times(1).
                WillOnce(Return(true));

            GUID_t remote_participant_key;
            fill_participant_key(remote_participant_key);

            ParticipantGenericMessage message;
            message.message_identity().source_guid(remote_participant_key);
            message.related_message_identity().source_guid(remote_participant_key);
            message.related_message_identity().sequence_number(1);
            message.destination_participant_key(remote_participant_key);
            message.message_class_id("dds.sec.auth");
            HandshakeMessageToken token;
            message.message_data().push_back(token);
            CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
            CDRMessage_t aux_msg(0);
            aux_msg.wraps = true;
            aux_msg.buffer = change->serializedPayload.data;
            aux_msg.max_size = change->serializedPayload.max_size;
            aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
            ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
            change->serializedPayload.length = aux_msg.length;

            HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
            CacheChange_t* change2 = new CacheChange_t(200);

            EXPECT_CALL(*auth_plugin_, process_handshake_rvr(_,_,_,_)).Times(1).
                WillOnce(DoAll(SetArgPointee<0>(p_handshake_message), Return(ValidationResult_t::VALIDATION_OK_WITH_FINAL_MESSAGE)));
            EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
                WillOnce(Return(change2));
            EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change2)).Times(1).
                WillOnce(Return(true));
            EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
                WillOnce(Return(true));
            EXPECT_CALL(*participant_.pdpsimple(), notifyAboveRemoteEndpoints(_)).Times(1);

            RTPSParticipantAuthenticationInfo info;
            info.status(AUTHORIZED_RTPSPARTICIPANT);
            info.guid(remote_participant_key);
            EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

            stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

            if(final_message_change == nullptr)
                delete change2;
            else
                *final_message_change = change2;

            delete p_handshake_message;
        }

    public:

        SecurityAuthenticationTest() : auth_plugin_(new MockAuthenticationPlugin()),
        crypto_plugin_(new MockCryptographyPlugin()),
        stateless_writer_(nullptr), stateless_reader_(nullptr),
        volatile_writer_(nullptr), volatile_reader_(nullptr),
        manager_(&participant_) {}

        ~SecurityAuthenticationTest()
        {
        }

        MockAuthenticationPlugin* auth_plugin_;
        MockCryptographyPlugin* crypto_plugin_;
        NiceMock<RTPSParticipantImpl> participant_;
        NiceMock<StatelessWriter>* stateless_writer_;
        NiceMock<StatelessReader>* stateless_reader_;
        NiceMock<StatefulWriter>* volatile_writer_;
        NiceMock<StatefulReader>* volatile_reader_;
        SecurityManager manager_;
};

TEST_F(SecurityAuthenticationTest, initialization_auth_nullptr)
{
    SecurityPluginFactory::release_auth_plugin();
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);

    ASSERT_TRUE(manager_.init());
}

TEST_F(SecurityAuthenticationTest, initialization_auth_failed)
{
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);

    EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));

    ASSERT_FALSE(manager_.init());
}

TEST_F(SecurityAuthenticationTest, initialization_register_local_participant_error)
{
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);
    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, register_local_participant(_,_,_,_)).Times(1).
        WillOnce(Return(nullptr));

    ASSERT_FALSE(manager_.init());
}

TEST_F(SecurityAuthenticationTest, initialization_fail_participant_stateless_message_writer)
{
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);
    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockParticipantCryptoHandle local_participant_crypto_handle;

    EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, register_local_participant(_,_,_,_)).Times(1).
        WillOnce(Return(&local_participant_crypto_handle));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, unregister_participant(&local_participant_crypto_handle,_)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(participant_, createWriter_mock(_,_,_,_,_,_)).Times(1).
        WillOnce(Return(false));

    ASSERT_FALSE(manager_.init());
}

TEST_F(SecurityAuthenticationTest, initialization_fail_participant_stateless_message_reader)
{
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);
    NiceMock<StatelessWriter>* stateless_writer = new NiceMock<StatelessWriter>(&participant_);
    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockParticipantCryptoHandle local_participant_crypto_handle;

    EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, register_local_participant(_,_,_,_)).Times(1).
        WillOnce(Return(&local_participant_crypto_handle));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, unregister_participant(&local_participant_crypto_handle,_)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(participant_, createWriter_mock(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(stateless_writer), Return(true)));
    EXPECT_CALL(participant_, createReader_mock(_,_,_,_,_,_,_)).Times(1).
        WillOnce(Return(false));

    ASSERT_FALSE(manager_.init());
}

TEST_F(SecurityAuthenticationTest, initialization_fail_participant_volatile_message_writer)
{
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);
    NiceMock<StatelessWriter>* stateless_writer = new NiceMock<StatelessWriter>(&participant_);
    NiceMock<StatelessReader>* stateless_reader = new NiceMock<StatelessReader>();
    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockParticipantCryptoHandle local_participant_crypto_handle;

    EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, register_local_participant(_,_,_,_)).Times(1).
        WillOnce(Return(&local_participant_crypto_handle));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, unregister_participant(&local_participant_crypto_handle,_)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(participant_, createWriter_mock(_,_,_,_,_,_)).Times(2).
        WillOnce(DoAll(SetArgPointee<0>(stateless_writer), Return(true))).
        WillOnce(Return(false));
    EXPECT_CALL(participant_, createReader_mock(_,_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(stateless_reader), Return(true)));

    ASSERT_FALSE(manager_.init());
}

TEST_F(SecurityAuthenticationTest, initialization_fail_participant_volatile_message_reader)
{
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);
    NiceMock<StatelessWriter>* stateless_writer = new NiceMock<StatelessWriter>(&participant_);
    NiceMock<StatelessReader>* stateless_reader = new NiceMock<StatelessReader>();
    NiceMock<StatefulWriter>* volatile_writer = new NiceMock<StatefulWriter>(&participant_);
    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockParticipantCryptoHandle local_participant_crypto_handle;

    EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, register_local_participant(_,_,_,_)).Times(1).
        WillOnce(Return(&local_participant_crypto_handle));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, unregister_participant(&local_participant_crypto_handle,_)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(participant_, createWriter_mock(_,_,_,_,_,_)).Times(2).
        WillOnce(DoAll(SetArgPointee<0>(stateless_writer), Return(true))).
        WillOnce(DoAll(SetArgPointee<0>(volatile_writer), Return(true)));
    EXPECT_CALL(participant_, createReader_mock(_,_,_,_,_,_,_)).Times(2).
        WillOnce(DoAll(SetArgPointee<0>(stateless_reader), Return(true))).
        WillOnce(Return(false));

    ASSERT_FALSE(manager_.init());
}

TEST_F(SecurityAuthenticationTest, initialization_auth_retry)
{
    DefaultValue<const RTPSParticipantAttributes&>::Set(pattr);
    DefaultValue<const GUID_t&>::Set(guid);
    NiceMock<StatelessWriter>* stateless_writer = new NiceMock<StatelessWriter>(&participant_);
    NiceMock<StatelessReader>* stateless_reader = new NiceMock<StatelessReader>();
    NiceMock<StatefulWriter>* volatile_writer = new NiceMock<StatefulWriter>(&participant_);
    NiceMock<StatefulReader>* volatile_reader = new NiceMock<StatefulReader>();
    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockParticipantCryptoHandle local_participant_crypto_handle;

    EXPECT_CALL(*auth_plugin_, validate_local_identity(_,_,_,_,_,_)).Times(2).
        WillOnce(Return(ValidationResult_t::VALIDATION_PENDING_RETRY)).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, register_local_participant(_,_,_,_)).Times(1).
        WillOnce(Return(&local_participant_crypto_handle));
    EXPECT_CALL(crypto_plugin_->cryptokeyfactory_, unregister_participant(&local_participant_crypto_handle,_)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(participant_, createWriter_mock(_,_,_,_,_,_)).Times(2).
        WillOnce(DoAll(SetArgPointee<0>(stateless_writer), Return(true))).
        WillOnce(DoAll(SetArgPointee<0>(volatile_writer), Return(true)));
    EXPECT_CALL(participant_, createReader_mock(_,_,_,_,_,_,_)).Times(2).
        WillOnce(DoAll(SetArgPointee<0>(stateless_reader), Return(true))).
        WillOnce(DoAll(SetArgPointee<0>(volatile_reader), Return(true)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    ASSERT_TRUE(manager_.init());
}


TEST_F(SecurityAuthenticationTest, initialization_ok)
{
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    initialization_ok();
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_fail)
{
    initialization_ok();

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    RTPSParticipantAuthenticationInfo info;
    info.status(UNAUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    ASSERT_FALSE(manager_.discovered_participant(&participant_data));
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_ok)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_pending_handshake_message)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_pending_handshake_request_fail)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    RTPSParticipantAuthenticationInfo info;
    info.status(UNAUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    ASSERT_FALSE(manager_.discovered_participant(&participant_data));
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_pending_handshake_request_ok)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), notifyAboveRemoteEndpoints(_)).Times(1);
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    RTPSParticipantAuthenticationInfo info;
    info.status(AUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    ASSERT_TRUE(manager_.discovered_participant(&participant_data));
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_new_change_fail)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(nullptr));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_FALSE(manager_.discovered_participant(&participant_data));

    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_add_change_fail)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change)).Times(1).
        WillOnce(Return(false));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_FALSE(manager_.discovered_participant(&participant_data));

    delete change;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_pending_handshake_request_pending_message)
{
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    request_process_ok();
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_pending_handshake_request_pending_message_resent)
{
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    CacheChange_t* request_message_change = nullptr;
    request_process_ok(&request_message_change);

    EXPECT_CALL(*stateless_writer_->history_, remove_change_and_reuse(request_message_change->sequenceNumber)).Times(1).
        WillOnce(Return(request_message_change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(request_message_change)).Times(1).
        WillOnce(Return(true));
    stateless_writer_->history_->reset_samples_number();
    ASSERT_TRUE(stateless_writer_->history_->wait_for_some_sample(std::chrono::seconds(1)));

    delete request_message_change;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validation_remote_identity_pending_handshake_request_ok_with_final_message)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_OK_WITH_FINAL_MESSAGE)));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), notifyAboveRemoteEndpoints(_)).Times(1);
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    RTPSParticipantAuthenticationInfo info;
    info.status(AUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    delete change;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_ok)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    delete change;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_validate_remote_fail_and_then_ok)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    RTPSParticipantAuthenticationInfo info;
    info.status(UNAUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    ASSERT_FALSE(manager_.discovered_participant(&participant_data));

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    delete change;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_begin_handshake_request_fail_and_then_ok)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;
    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST)));
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    RTPSParticipantAuthenticationInfo info;
    info.status(UNAUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ASSERT_FALSE(manager_.discovered_participant(&participant_data));

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(0);
    EXPECT_CALL(*auth_plugin_, begin_handshake_request(_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    delete change;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_not_remote_participant_key)
{
    initialization_ok();

    ParticipantGenericMessage message;
    message.message_class_id("dds.sec.auth");
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(1).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_bad_message_class_id)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    ParticipantGenericMessage message;
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_not_expecting_request)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_OK)));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    ParticipantGenericMessage message;
    message.message_identity().source_guid(participant_data.m_guid);
    message.destination_participant_key(participant_data.m_guid);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_fail_begin_handshake_reply)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    ParticipantGenericMessage message;
    message.message_identity().source_guid(participant_data.m_guid);
    message.destination_participant_key(participant_data.m_guid);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, begin_handshake_reply_rvr(_,_,_,_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    RTPSParticipantAuthenticationInfo info;
    info.status(UNAUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_ok_begin_handshake_reply)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    ParticipantGenericMessage message;
    message.message_identity().source_guid(participant_data.m_guid);
    message.destination_participant_key(participant_data.m_guid);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;

    EXPECT_CALL(*auth_plugin_, begin_handshake_reply_rvr(_,_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    Return(ValidationResult_t::VALIDATION_OK)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*participant_.pdpsimple(), notifyAboveRemoteEndpoints(_)).Times(1);
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    RTPSParticipantAuthenticationInfo info;
    info.status(AUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_new_change_fail)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    ParticipantGenericMessage message;
    message.message_identity().source_guid(participant_data.m_guid);
    message.destination_participant_key(participant_data.m_guid);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();

    EXPECT_CALL(*auth_plugin_, begin_handshake_reply_rvr(_,_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(nullptr));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_add_change_fail)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    ParticipantGenericMessage message;
    message.message_identity().source_guid(participant_data.m_guid);
    message.destination_participant_key(participant_data.m_guid);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change2 = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, begin_handshake_reply_rvr(_,_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change2));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change2)).Times(1).
        WillOnce(Return(false));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

    delete change2;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_pending_handshake_reply_pending_message)
{
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    reply_process_ok();
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_pending_handshake_reply_pending_message_resent)
{
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    CacheChange_t* reply_message_change = nullptr;
    reply_process_ok(&reply_message_change);

    EXPECT_CALL(*stateless_writer_->history_, remove_change_and_reuse(reply_message_change->sequenceNumber)).Times(1).
        WillOnce(Return(reply_message_change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(reply_message_change)).Times(1).
        WillOnce(Return(true));
    stateless_writer_->history_->reset_samples_number();
    ASSERT_TRUE(stateless_writer_->history_->wait_for_some_sample(std::chrono::seconds(1)));

    delete reply_message_change;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_pending_handshake_reply_ok_with_final_message)
{
    initialization_ok();

    MockIdentityHandle identity_handle;
    MockIdentityHandle* p_identity_handle = &identity_handle;

    EXPECT_CALL(*auth_plugin_, validate_remote_identity_rvr(_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_identity_handle), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));

    ParticipantProxyData participant_data;
    fill_participant_key(participant_data.m_guid);
    ASSERT_TRUE(manager_.discovered_participant(&participant_data));

    ParticipantGenericMessage message;
    message.message_identity().source_guid(participant_data.m_guid);
    message.destination_participant_key(participant_data.m_guid);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    MockHandshakeHandle handshake_handle;
    MockHandshakeHandle* p_handshake_handle = &handshake_handle;
    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change2 = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, begin_handshake_reply_rvr(_,_,_,_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_handle), 
                    SetArgPointee<1>(p_handshake_message), Return(ValidationResult_t::VALIDATION_OK_WITH_FINAL_MESSAGE)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change2));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change2)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*participant_.pdpsimple(), notifyAboveRemoteEndpoints(_)).Times(1);
    EXPECT_CALL(*participant_.pdpsimple(), get_participant_proxy_data_serialized(BIGEND)).Times(1);

    RTPSParticipantAuthenticationInfo info;
    info.status(AUTHORIZED_RTPSPARTICIPANT);
    info.guid(participant_data.m_guid);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

    delete change2;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_fail_process_handshake_reply)
{
    request_process_ok();

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, process_handshake_rvr(_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    RTPSParticipantAuthenticationInfo info;
    info.status(UNAUTHORIZED_RTPSPARTICIPANT);
    info.guid(remote_participant_key);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_ok_process_handshake_reply)
{
    request_process_ok();

    EXPECT_CALL(*stateless_writer_->history_, remove_change(SequenceNumber_t{0,1})).Times(1).
        WillOnce(Return(true));

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, process_handshake_rvr(_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*participant_.pdpsimple(), notifyAboveRemoteEndpoints(_)).Times(1);

    RTPSParticipantAuthenticationInfo info;
    info.status(AUTHORIZED_RTPSPARTICIPANT);
    info.guid(remote_participant_key);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_process_handshake_reply_new_change_fail)
{
    request_process_ok();

    EXPECT_CALL(*stateless_writer_->history_, remove_change(SequenceNumber_t{0,1})).Times(1).
        WillOnce(Return(true));

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();

    EXPECT_CALL(*auth_plugin_, process_handshake_rvr(_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(nullptr));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_process_handshake_reply_add_change_fail)
{
    request_process_ok();

    EXPECT_CALL(*stateless_writer_->history_, remove_change(SequenceNumber_t{0,1})).Times(1).
        WillOnce(Return(true));

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    HandshakeMessageToken* p_handshake_message = new HandshakeMessageToken();
    CacheChange_t* change2 = new CacheChange_t(200);

    EXPECT_CALL(*auth_plugin_, process_handshake_rvr(_,_,_,_)).Times(1).
        WillOnce(DoAll(SetArgPointee<0>(p_handshake_message), Return(ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE)));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_writer_, new_change(_,_,_)).Times(1).
        WillOnce(Return(change2));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(change2)).Times(1).
        WillOnce(Return(false));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

    delete change2;
    delete p_handshake_message;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_process_handshake_reply_ok_with_final_message)
{
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    final_message_process_ok();
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_process_handshake_reply_ok_with_final_message_resent)
{
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));

    CacheChange_t* final_message_change = nullptr;
    final_message_process_ok(&final_message_change);

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*stateless_writer_->history_, remove_change_and_reuse(final_message_change->sequenceNumber)).Times(1).
        WillOnce(Return(final_message_change));
    EXPECT_CALL(*stateless_writer_->history_, add_change_mock(final_message_change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);

    delete final_message_change;
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_bad_related_guid)
{
    reply_process_ok();

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);
    remote_participant_key.guidPrefix.value[0] = 0xFF;

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_bad_related_sequence_number)
{
    reply_process_ok();

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(10);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_fail_process_handshake_final)
{
    reply_process_ok();

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, process_handshake_rvr(_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_FAILED));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    RTPSParticipantAuthenticationInfo info;
    info.status(UNAUTHORIZED_RTPSPARTICIPANT);
    info.guid(remote_participant_key);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

TEST_F(SecurityAuthenticationTest, discovered_participant_process_message_ok_process_handshake_final)
{
    reply_process_ok();

    EXPECT_CALL(*stateless_writer_->history_, remove_change(SequenceNumber_t{0,1})).Times(1).
        WillOnce(Return(true));

    GUID_t remote_participant_key;
    fill_participant_key(remote_participant_key);

    ParticipantGenericMessage message;
    message.message_identity().source_guid(remote_participant_key);
    message.related_message_identity().source_guid(remote_participant_key);
    message.related_message_identity().sequence_number(1);
    message.destination_participant_key(remote_participant_key);
    message.message_class_id("dds.sec.auth");
    HandshakeMessageToken token;
    message.message_data().push_back(token);
    CacheChange_t* change = new CacheChange_t(ParticipantGenericMessageHelper::serialized_size(message));
    CDRMessage_t aux_msg(0);
    aux_msg.wraps = true;
    aux_msg.buffer = change->serializedPayload.data;
    aux_msg.max_size = change->serializedPayload.max_size;
    aux_msg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND : LITTLEEND;
    ASSERT_TRUE(CDRMessage::addParticipantGenericMessage(&aux_msg, message));
    change->serializedPayload.length = aux_msg.length;

    EXPECT_CALL(*auth_plugin_, process_handshake_rvr(_,_,_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_identity_handle(_,_)).Times(2).
        WillRepeatedly(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*auth_plugin_, return_handshake_handle(_,_)).Times(1).
        WillOnce(Return(ValidationResult_t::VALIDATION_OK));
    EXPECT_CALL(*stateless_reader_->history_, remove_change_mock(change)).Times(1).
        WillOnce(Return(true));
    EXPECT_CALL(*participant_.pdpsimple(), notifyAboveRemoteEndpoints(_)).Times(1);

    RTPSParticipantAuthenticationInfo info;
    info.status(AUTHORIZED_RTPSPARTICIPANT);
    info.guid(remote_participant_key);
    EXPECT_CALL(*participant_.getListener(), onRTPSParticipantAuthentication(_, info)).Times(1);

    stateless_reader_->listener_->onNewCacheChangeAdded(stateless_reader_, change);
}

int main(int argc, char **argv)
{
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}