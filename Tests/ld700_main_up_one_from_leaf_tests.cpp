#include "../MainAVR/src/ld700-main-up-one-from-leaf.h"
#include "mockImplementations/mockCommonLdp.h"
#include "mockImplementations/mockDiscSwitch.h"
#include "mockImplementations/mockLD700Callbacks.h"
#include "mockImplementations/mockLD700Deps.h"
#include "mockImplementations/mockLdpc.h"
#include "mockImplementations/mockSettings.h"
#include "mockImplementations/mockStrings.h"
//#include <avr/io.h>
#include "mocks.h"
#include "mockImplementations/mockTimerGlobal.h"

using testing::Return;
using testing::Mock;

class LD700UpOneFromLeaf : public ::testing::Test
{
public:
	void SetUp() override
	{
		g_pPINAPtr = &m_PINA;
		g_pPORTAPtr = &m_PORTA;
		g_pTCNT0Ptr = &m_TCNT0;

		mockLdpcSetInstance(&m_mockLdpc);
		mockLD700CallbacksSetInstance(&m_mockLD700Callbacks);
		mockSettingsSetInstance(&m_mockSettings);
		mockDiscSwitchSetInstance(&m_mockDiscSwitch);
		mockCommonLdpSetInstance(&m_mockCommonLdp);
		mockStringsSetInstance(&m_mockStrings);
		mockLD700DepsSetInstance(&m_mockLD700Deps);
		mockTimerGlobalSetInstance(&m_mockTimerGlobal);

		ld700_up_one_from_leaf_reset();	// we want to start in a consistent state
	}

	void TearDown() override
	{
		g_pPINAPtr = nullptr;
		g_pPORTAPtr = nullptr;
		g_pTCNT0Ptr = nullptr;

		mockLdpcSetInstance(nullptr);
		mockLD700CallbacksSetInstance(nullptr);
		mockSettingsSetInstance(nullptr);
		mockDiscSwitchSetInstance(nullptr);
		mockCommonLdpSetInstance(nullptr);
		mockStringsSetInstance(nullptr);
		mockLD700DepsSetInstance(nullptr);
		mockTimerGlobalSetInstance(nullptr);
	}

protected:
	MockReadOnlyRegister8 m_PINA;
	MockReadWriteRegister8 m_PORTA;
	MockReadWriteRegister8 m_TCNT0;

	MockLdpcTestInterface m_mockLdpc;
	MockLD700CallbacksTestInterface m_mockLD700Callbacks;
	MockSettingsTestInterface m_mockSettings;
	MockDiscSwitchTestInterface m_mockDiscSwitch;
	MockCommonLdpTestInterface m_mockCommonLdp;
	MockStringsTestInterface m_mockStrings;
	MockLD700DepsTestInterface m_mockLD700Deps;
	MockTimerGlobalTestInterface m_mockTimerGlobal;
};

////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(LD700UpOneFromLeaf, OnVblankSide1Ejected)
{
	EXPECT_CALL(m_mockCommonLdp, Get3bitVsyncCounter()).WillRepeatedly(Return(0));
	EXPECT_CALL(m_mockLD700Deps, GetLD700CandidateSide()).WillRepeatedly(Return(1));
	EXPECT_CALL(m_PORTA, AndEqualsOp(~(1 << PA5)));
	EXPECT_CALL(m_PORTA, OrEqualsOp(1 << PA6));

	ld700_on_vblank(LD700_TRAY_EJECTED);

	// make sure the old expectations don't interfere with the new ones
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&m_PORTA));

	EXPECT_CALL(m_PORTA, AndEqualsOp(~((1 << PA5)|(1 << PA6))));
	ld700_on_vblank(LD700_TRAY_EJECTED);	// calling again should disable both LEDs
}

TEST_F(LD700UpOneFromLeaf, OnVblankSide2Ejected)
{
	EXPECT_CALL(m_mockCommonLdp, Get3bitVsyncCounter()).WillRepeatedly(Return(0));
	EXPECT_CALL(m_mockLD700Deps, GetLD700CandidateSide()).WillRepeatedly(Return(2));
	EXPECT_CALL(m_PORTA, AndEqualsOp(~(1 << PA6)));
	EXPECT_CALL(m_PORTA, OrEqualsOp(1 << PA5));

	ld700_on_vblank(LD700_TRAY_EJECTED);
}

TEST_F(LD700UpOneFromLeaf, OnVblankUnknownDiscEjected)
{
	EXPECT_CALL(m_mockCommonLdp, Get3bitVsyncCounter()).WillRepeatedly(Return(0));
	EXPECT_CALL(m_mockLD700Deps, GetLD700CandidateSide()).WillRepeatedly(Return(0));
	EXPECT_CALL(m_PORTA, OrEqualsOp(1 << PA5 | 1 << PA6));

	ld700_on_vblank(LD700_TRAY_EJECTED);
}

TEST_F(LD700UpOneFromLeaf, OnVblankNoActionEjected)
{
	EXPECT_CALL(m_mockCommonLdp, Get3bitVsyncCounter()).WillRepeatedly(Return(1));	// to ensure no action is taken
	EXPECT_CALL(m_mockLD700Deps, GetLD700CandidateSide()).Times(0);
	EXPECT_CALL(m_PORTA, OrEqualsOp(testing::_)).Times(0);	// PORTA should not be modified at all

	ld700_on_vblank(LD700_TRAY_EJECTED);
}

TEST_F(LD700UpOneFromLeaf, OnVblankSide1Inserted)
{
	uint8_t u8DiscId = 0x61;	// TQ Halcyon side 1

	EXPECT_CALL(m_mockCommonLdp, Get3bitVsyncCounter()).Times(0);
	EXPECT_CALL(m_mockLD700Deps, GetLD700CandidateSide()).Times(0);
	EXPECT_CALL(m_PORTA, AndEqualsOp(~(1 << PA5)));
	EXPECT_CALL(m_PORTA, OrEqualsOp(1 << PA6));
	EXPECT_CALL(m_mockLD700Deps, GetDiscSideByDiscId(u8DiscId)).WillRepeatedly(Return(1));
	EXPECT_CALL(m_mockSettings, GetActiveDiscIdMemory()).WillRepeatedly(Return(u8DiscId));

	ld700_on_vblank(LD700_STOPPED);
}

///////////////////////////////

TEST_F(LD700UpOneFromLeaf, ButtonThink_Press)
{
	LD700Status_t status = LD700_PLAYING;	// arbitrary

	// 127 chosen as initial value because it's positive as both a signed and unsigned number.
	// 127+201 intentionally overflows and we want to test to make sure the delta computation is still accurate.
	EXPECT_CALL(m_TCNT0, GetOp()).WillOnce(Return(127))
		.WillRepeatedly(Return(127+201));

	EXPECT_CALL(m_mockTimerGlobal, GetSlowTimerVal()).WillOnce(Return(0))		// new counter
		.WillOnce(Return(0))
		.WillRepeatedly(Return(69));

	EXPECT_CALL(m_PINA, GetOp()).WillOnce(Return(0))	// button being pressed
		.WillOnce(Return(0))	// press detected
		.WillOnce(Return(0))	// not-quite-hold
		.WillOnce(Return(1 << PA2))	// button released
	;

	EXPECT_CALL(m_mockLD700Deps, OnFlipDiscPressed(status));
	EXPECT_CALL(m_mockLD700Deps, OnFlipDiscHeld(status)).Times(0);

	// this will start the new counter
	ld700_button_think(status);

	// this will latch the press event
	ld700_button_think(status);

	// this won't quite fire the hold event
	ld700_button_think(status);

	// this will fire the press event
	ld700_button_think(status);
}

TEST_F(LD700UpOneFromLeaf, ButtonThink_PressNotQuite)
{
	LD700Status_t status = LD700_PLAYING;	// arbitrary

	EXPECT_CALL(m_TCNT0, GetOp()).WillOnce(Return(0))
		.WillRepeatedly(Return(200));	// not quite enough
	EXPECT_CALL(m_mockTimerGlobal, GetSlowTimerVal()).WillRepeatedly(Return(0));
	EXPECT_CALL(m_PINA, GetOp()).WillOnce(Return(0))	// button being pressed
		.WillOnce(Return(0))
		.WillOnce(Return(1 << PA2))	// button released
	;

	EXPECT_CALL(m_mockLD700Deps, OnFlipDiscPressed(status)).Times(0);

	// this will start the new counter
	ld700_button_think(status);

	// this won't quite latch the press event
	ld700_button_think(status);

	// this won't fire the press event
	ld700_button_think(status);
}

TEST_F(LD700UpOneFromLeaf, ButtonThink_Hold)
{
	LD700Status_t status = LD700_PLAYING;	// arbitrary

	EXPECT_CALL(m_TCNT0, GetOp()).WillOnce(Return(0))
		.WillRepeatedly(Return(201));
	EXPECT_CALL(m_mockTimerGlobal, GetSlowTimerVal()).WillOnce(Return(0))		// new counter
		.WillOnce(Return(0))
		.WillRepeatedly(Return(70));
	EXPECT_CALL(m_PINA, GetOp()).WillOnce(Return(0))	// button being pressed
		.WillOnce(Return(0))	// button being pressed (press event latched)
		.WillOnce(Return(0))	// button being pressed (held event)
		.WillOnce(Return(1 << PA2))	// button released
	;

	EXPECT_CALL(m_mockLD700Deps, OnFlipDiscPressed(status)).Times(0);	// the pressed event should never fire
	EXPECT_CALL(m_mockLD700Deps, OnFlipDiscHeld(status));

	// this will start the new counter
	ld700_button_think(status);

	// this will latch the press event
	ld700_button_think(status);

	// this will fire the hold event
	ld700_button_think(status);

	// button release, should not fire press event
	ld700_button_think(status);
}
