#include "stdafx.h"
#include "../MainAVR/src/ld700-main-deps.h"	// I tried to put this in CMakeLists.txt and couldn't get it working for some reason
#include "mockImplementations/mockCommonLdp.h"
#include "mockImplementations/mockDiscSwitch.h"
#include "mockImplementations/mockLD700Callbacks.h"
#include "mockImplementations/mockLdpc.h"
#include "mockImplementations/mockSettings.h"
#include "mockImplementations/mockStrings.h"

using testing::Return;
using testing::Mock;

class LD700Deps : public ::testing::Test
{
public:
	void SetUp() override
	{
//		g_pPINAPtr = &m_PINA;
//		g_pPORTAPtr = &m_PORTA;
//		g_pTCNT0Ptr = &m_TCNT0;

		mockLdpcSetInstance(&m_mockLdpc);
		mockLD700CallbacksSetInstance(&m_mockLD700Callbacks);
		mockSettingsSetInstance(&m_mockSettings);
		mockDiscSwitchSetInstance(&m_mockDiscSwitch);
		mockCommonLdpSetInstance(&m_mockCommonLdp);
		mockStringsSetInstance(&m_mockStrings);

		ld700_deps_reset();	// we want to start in a consistent state
	}

	void TearDown() override
	{
//		g_pPINAPtr = nullptr;
//		g_pPORTAPtr = nullptr;
//		g_pTCNT0Ptr = nullptr;

		mockLdpcSetInstance(nullptr);
		mockLD700CallbacksSetInstance(nullptr);
		mockSettingsSetInstance(nullptr);
		mockDiscSwitchSetInstance(nullptr);
		mockCommonLdpSetInstance(nullptr);
		mockStringsSetInstance(nullptr);
	}

protected:
//	MockReadOnlyRegister8 m_PINA;
//	MockReadWriteRegister8 m_PORTA;
//	MockReadWriteRegister8 m_TCNT0;

	MockLdpcTestInterface m_mockLdpc;
	MockLD700CallbacksTestInterface m_mockLD700Callbacks;
	MockSettingsTestInterface m_mockSettings;
	MockDiscSwitchTestInterface m_mockDiscSwitch;
	MockCommonLdpTestInterface m_mockCommonLdp;
	MockStringsTestInterface m_mockStrings;
};

////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(LD700Deps, GetTargetDiscIdByCurDiscIdAndTargetSide)
{
	// thayer's quest
	ASSERT_EQ(0x61,GetTargetDiscIdByCurDiscIdAndTargetSide(0x61, 0));
	ASSERT_EQ(0x61,GetTargetDiscIdByCurDiscIdAndTargetSide(0x61, 1));
	ASSERT_EQ(0x61,GetTargetDiscIdByCurDiscIdAndTargetSide(0x62, 0));
	ASSERT_EQ(0x61,GetTargetDiscIdByCurDiscIdAndTargetSide(0x62, 1));

	ASSERT_EQ(0x62,GetTargetDiscIdByCurDiscIdAndTargetSide(0x61, 2));
	ASSERT_EQ(0x62,GetTargetDiscIdByCurDiscIdAndTargetSide(0x62, 2));

	// nfl football
	ASSERT_EQ(0x43,GetTargetDiscIdByCurDiscIdAndTargetSide(0x43, 0));
	ASSERT_EQ(0x43,GetTargetDiscIdByCurDiscIdAndTargetSide(0x43, 1));
	ASSERT_EQ(0x43,GetTargetDiscIdByCurDiscIdAndTargetSide(0x44, 0));
	ASSERT_EQ(0x43,GetTargetDiscIdByCurDiscIdAndTargetSide(0x44, 1));

	ASSERT_EQ(0x44,GetTargetDiscIdByCurDiscIdAndTargetSide(0x43, 2));
	ASSERT_EQ(0x44,GetTargetDiscIdByCurDiscIdAndTargetSide(0x44, 2));

	// unknown
	ASSERT_EQ(0x99,GetTargetDiscIdByCurDiscIdAndTargetSide(0x99, 0));
	ASSERT_EQ(0x99,GetTargetDiscIdByCurDiscIdAndTargetSide(0x99, 1));
	ASSERT_EQ(0x99,GetTargetDiscIdByCurDiscIdAndTargetSide(0x99, 2));
}

TEST_F(LD700Deps, GetDiscSideByDiscId)
{
	// side 1's
	ASSERT_EQ(1,GetDiscSideByDiscId(0x61));
	ASSERT_EQ(1,GetDiscSideByDiscId(0x43));

	// side 2's
	ASSERT_EQ(2,GetDiscSideByDiscId(0x62));
	ASSERT_EQ(2,GetDiscSideByDiscId(0x44));

	// unknown
	ASSERT_EQ(0,GetDiscSideByDiscId(0x99));
}

TEST_F(LD700Deps, OnFlipDiscHeld_ShouldEject)
{
	LDPCStatus_t stat = LDPC_PLAYING;	// arbitraary
	LD700Status_t statLD700 = LD700_PLAYING;	// must not be ejected

	EXPECT_CALL(m_mockLdpc, GetStatus()).WillRepeatedly(Return(stat));
	EXPECT_CALL(m_mockLD700Callbacks, ConvertStatus(stat)).WillRepeatedly(Return(statLD700));
	EXPECT_CALL(m_mockLD700Callbacks, Eject());

	OnFlipDiscHeld();
}

TEST_F(LD700Deps, OnFlipDiscHeld_ShouldInsertTray)
{
	LDPCStatus_t stat = LDPC_STOPPED;	// arbitraary
	LD700Status_t statLD700 = LD700_TRAY_EJECTED;	// must be ejected
	uint8_t u8ActiveDiscId = 0x43;	// TQ hal side 1

	EXPECT_CALL(m_mockLdpc, GetStatus()).WillRepeatedly(Return(stat));
	EXPECT_CALL(m_mockLD700Callbacks, ConvertStatus(stat)).WillRepeatedly(Return(statLD700));
	EXPECT_CALL(m_mockSettings, GetActiveDiscIdMemory()).WillRepeatedly(Return(u8ActiveDiscId));
	EXPECT_CALL(m_mockDiscSwitch, Initiate(u8ActiveDiscId));	// if they are holding the button, they are trying to insert the disc, not flip it

	OnFlipDiscHeld();
}

TEST_F(LD700Deps, OnFlipDiscPressed_AlreadyEjected)
{
	LDPCStatus_t stat = LDPC_STOPPED;	// arbitraary
	LD700Status_t statLD700 = LD700_TRAY_EJECTED;	// must be ejected
	uint8_t u8ActiveDiscId = 0x43;	// TQ hal side 1

	EXPECT_CALL(m_mockLdpc, GetStatus()).WillRepeatedly(Return(stat));
	EXPECT_CALL(m_mockLD700Callbacks, ConvertStatus(stat)).WillRepeatedly(Return(statLD700));
	EXPECT_CALL(m_mockSettings, GetActiveDiscIdMemory()).WillRepeatedly(Return(u8ActiveDiscId));

	OnFlipDiscPressed();
	ASSERT_EQ(2, GetLD700CandidateSide());	// first tap should take is to side 2

	OnFlipDiscPressed();
	ASSERT_EQ(1, GetLD700CandidateSide());	// second tap should take is to side 2
}

TEST_F(LD700Deps, OnFlipDiscPressed_NotEjected)
{
	LDPCStatus_t stat = LDPC_STOPPED;	// arbitraary
	LD700Status_t statLD700 = LD700_STOPPED;	// must not be ejected
	uint8_t u8ActiveDiscId = 0x43;	// TQ hal side 1

	EXPECT_CALL(m_mockLdpc, GetStatus()).WillRepeatedly(Return(stat));
	EXPECT_CALL(m_mockLD700Callbacks, ConvertStatus(stat)).WillRepeatedly(Return(statLD700));
	EXPECT_CALL(m_mockSettings, GetActiveDiscIdMemory()).WillRepeatedly(Return(u8ActiveDiscId));

	// nothing should change as this is repeatedly pressed
	OnFlipDiscPressed();
	ASSERT_EQ(1, GetLD700CandidateSide());
	OnFlipDiscPressed();
	ASSERT_EQ(1, GetLD700CandidateSide());
}

// need to pierce the veil to make unit tests viable.  Could also add #ifdef's to production code but let's try this first and see how it feels.
extern uint8_t g_ld700_bDiscSwitchIsActive;

TEST_F(LD700Deps, IdleThink_DiscSwitchSuccess)
{
	g_ld700_bDiscSwitchIsActive = 1;

	EXPECT_CALL(m_mockDiscSwitch, GetStatus()).WillRepeatedly(Return(DISC_SWITCH_SUCCESS));
	EXPECT_CALL(m_mockDiscSwitch, End());
	EXPECT_CALL(m_mockLD700Callbacks, CloseTray());

	ld700_idle_think();
}

TEST_F(LD700Deps, IdleThink_DiscSwitchError)
{
	g_ld700_bDiscSwitchIsActive = 1;

	EXPECT_CALL(m_mockDiscSwitch, GetStatus()).WillRepeatedly(Return(DISC_SWITCH_ERROR));
	EXPECT_CALL(m_mockDiscSwitch, End());
	EXPECT_CALL(m_mockStrings, LogString(STRING_DISCSWITCH_FAILED));

	ld700_idle_think();
}