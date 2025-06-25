#include "stdafx.h"
#include "../MainAVR/src/autodetect.h"
#include "mockImplementations/mockAutodetectDeps.h"
#include "mockImplementations/mockProtocol.h"

using testing::Return;
using testing::Mock;
using testing::InSequence;

class Autodetect : public ::testing::Test
{
public:
	void SetUp() override
	{
		g_pDDRBPtr = &m_DDRB;
		g_pPORTBPtr = &m_PORTB;
		g_pDDRCPtr = &m_DDRC;
		g_pPINCPtr = &m_PINC;
		g_pPORTCPtr = &m_PORTC;

		mockAutodetectDepsSetInstance(&m_mockAutodetectDeps);
		mockProtocolSetInstance(&m_mockProtocol);
	}

	void TearDown() override
	{
		g_pDDRBPtr = nullptr;
		g_pPORTBPtr = nullptr;
		g_pDDRCPtr = nullptr;
		g_pPINCPtr = nullptr;
		g_pPORTCPtr = nullptr;

		mockAutodetectDepsSetInstance(nullptr);
		mockProtocolSetInstance(nullptr);
	}

protected:
	MockReadWriteRegister8 m_DDRB;
	MockReadWriteRegister8 m_PORTB;

	MockReadWriteRegister8 m_DDRC;
	MockReadOnlyRegister8 m_PINC;
	MockReadWriteRegister8 m_PORTC;

	MockAutodetectDepsTestInterface m_mockAutodetectDeps;
	MockProtocolTestInterface m_mockProtocol;
};

////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(Autodetect, DetectPR7820Quick)
{
	// ARRANGE

	{
		uint8_t u8DDRBInitialVal = 0xFE;
		uint8_t u8PORTBInitialVal = 1;

		uint8_t u8DDRCInitialVal = 0xFF;
		uint8_t u8PORTCInitialVal = 0;
		InSequence dummy;

		// saving previous state

		EXPECT_CALL(m_DDRB, GetOp()).WillOnce(Return(u8DDRBInitialVal));
		EXPECT_CALL(m_PORTB, GetOp()).WillOnce(Return(u8PORTBInitialVal));

		EXPECT_CALL(m_DDRC, GetOp()).WillOnce(Return(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTC, GetOp()).WillOnce(Return(u8PORTCInitialVal));

		// setup

		EXPECT_CALL(m_DDRC, OrEqualsOp((1 << PC1)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC1)));

		EXPECT_CALL(m_DDRB, AndEqualsOp(~(1 << PB6)));
		EXPECT_CALL(m_PORTB, AndEqualsOp(~(1 << PB6)));

		// loop first iteration

		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC0)));

		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillRepeatedly(Return(0));

		// clean-up

		EXPECT_CALL(m_PORTC, AssignOp(u8PORTCInitialVal));
		EXPECT_CALL(m_DDRC, AssignOp(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTB, AssignOp(u8PORTBInitialVal));
		EXPECT_CALL(m_DDRB, AssignOp(u8DDRBInitialVal));
	}

	LDPType actual = detect_ldv1000_or_pr7820();

	ASSERT_EQ(LDP_PR7820, actual);
}

TEST_F(Autodetect, DetectPR7820Slow)
{
	// ARRANGE

	{
		uint8_t u8DDRBInitialVal = 0xFE;
		uint8_t u8PORTBInitialVal = 1;

		uint8_t u8DDRCInitialVal = 0xFF;
		uint8_t u8PORTCInitialVal = 0;
		InSequence dummy;

		// saving previous state

		EXPECT_CALL(m_DDRB, GetOp()).WillOnce(Return(u8DDRBInitialVal));
		EXPECT_CALL(m_PORTB, GetOp()).WillOnce(Return(u8PORTBInitialVal));

		EXPECT_CALL(m_DDRC, GetOp()).WillOnce(Return(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTC, GetOp()).WillOnce(Return(u8PORTCInitialVal));

		// setup

		EXPECT_CALL(m_DDRC, OrEqualsOp((1 << PC1)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC1)));

		EXPECT_CALL(m_DDRB, AndEqualsOp(~(1 << PB6)));
		EXPECT_CALL(m_PORTB, AndEqualsOp(~(1 << PB6)));

		// loop first iteration

		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC0)));

		// pin is raised
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillRepeatedly(Return(1));

		// expect pin to get forced low briefly
		EXPECT_CALL(m_PORTC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_DDRC, OrEqualsOp((1 << PC0)));
		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));

		// pin no longer raised (just to make the loop iterate, not sure this would ever actually happen on real hardware)
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillRepeatedly(Return(0));

		// loop second iteration

		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC0)));

		// pin is raised
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillOnce(Return(1));

		// expect pin to get forced low briefly
		EXPECT_CALL(m_PORTC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_DDRC, OrEqualsOp((1 << PC0)));
		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));

		// pin is still raised (this should confirm that it's PR-7820)
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillOnce(Return(1));

		// clean-up

		EXPECT_CALL(m_PORTC, AssignOp(u8PORTCInitialVal));
		EXPECT_CALL(m_DDRC, AssignOp(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTB, AssignOp(u8PORTBInitialVal));
		EXPECT_CALL(m_DDRB, AssignOp(u8DDRBInitialVal));
	}

	LDPType actual = detect_ldv1000_or_pr7820();

	ASSERT_EQ(LDP_PR7820, actual);
}

TEST_F(Autodetect, DetectLDV1000)
{
	// ARRANGE

	{
		uint8_t u8DDRBInitialVal = 0xFE;
		uint8_t u8PORTBInitialVal = 1;

		uint8_t u8DDRCInitialVal = 0xFF;
		uint8_t u8PORTCInitialVal = 0;
		InSequence dummy;

		// saving previous state

		EXPECT_CALL(m_DDRB, GetOp()).WillOnce(Return(u8DDRBInitialVal));
		EXPECT_CALL(m_PORTB, GetOp()).WillOnce(Return(u8PORTBInitialVal));

		EXPECT_CALL(m_DDRC, GetOp()).WillOnce(Return(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTC, GetOp()).WillOnce(Return(u8PORTCInitialVal));

		// setup

		EXPECT_CALL(m_DDRC, OrEqualsOp((1 << PC1)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC1)));

		EXPECT_CALL(m_DDRB, AndEqualsOp(~(1 << PB6)));
		EXPECT_CALL(m_PORTB, AndEqualsOp(~(1 << PB6)));

		// loop first iteration

		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC0)));

		// pin is raised
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillRepeatedly(Return(1));

		// expect pin to get forced low briefly
		EXPECT_CALL(m_PORTC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_DDRC, OrEqualsOp((1 << PC0)));
		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));

		// pin lowered
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillRepeatedly(Return(0));

		// loop second iteration

		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_PORTC, OrEqualsOp((1 << PC0)));

		// pin is raised
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillOnce(Return(1));

		// expect pin to get forced low briefly
		EXPECT_CALL(m_PORTC, AndEqualsOp(~(1 << PC0)));
		EXPECT_CALL(m_DDRC, OrEqualsOp((1 << PC0)));
		EXPECT_CALL(m_DDRC, AndEqualsOp(~(1 << PC0)));

		// pin lowered
		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillOnce(Return(0));

		// clean-up

		EXPECT_CALL(m_PORTC, AssignOp(u8PORTCInitialVal));
		EXPECT_CALL(m_DDRC, AssignOp(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTB, AssignOp(u8PORTBInitialVal));
		EXPECT_CALL(m_DDRB, AssignOp(u8DDRBInitialVal));
	}

	LDPType actual = detect_ldv1000_or_pr7820();

	ASSERT_EQ(LDP_LDV1000, actual);
}

TEST_F(Autodetect, DetectOtherModeLD700)
{
	// ARRANGE

	{
		uint8_t u8DDRCInitialVal = 0xFF;
		uint8_t u8PORTCInitialVal = 0;
		InSequence dummy;

		EXPECT_CALL(m_DDRC, GetOp()).WillOnce(Return(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTC, GetOp()).WillOnce(Return(u8PORTCInitialVal));

		EXPECT_CALL(m_DDRC, AndEqualsOp(~((1 << PC0) | (1 << PC1))));
		EXPECT_CALL(m_PORTC, OrEqualsOp(((1 << PC0) | (1 << PC1))));

		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillRepeatedly(Return(0));

		EXPECT_CALL(m_PORTC, AssignOp(u8PORTCInitialVal));
		EXPECT_CALL(m_DDRC, AssignOp(u8DDRCInitialVal));
	}

	LDPType actual = detect_other_mode();

	ASSERT_EQ(LDP_LD700, actual);
}

TEST_F(Autodetect, DetectOtherModeLDP1000A)
{
	// ARRANGE

	{
		uint8_t u8DDRCInitialVal = 0xFF;
		uint8_t u8PORTCInitialVal = 0;
		InSequence dummy;

		EXPECT_CALL(m_DDRC, GetOp()).WillOnce(Return(u8DDRCInitialVal));
		EXPECT_CALL(m_PORTC, GetOp()).WillOnce(Return(u8PORTCInitialVal));

		EXPECT_CALL(m_DDRC, AndEqualsOp(~((1 << PC0) | (1 << PC1))));
		EXPECT_CALL(m_PORTC, OrEqualsOp(((1 << PC0) | (1 << PC1))));

		EXPECT_CALL(m_mockAutodetectDeps, IsPC0Raised()).WillRepeatedly(Return(1));

		EXPECT_CALL(m_PORTC, AssignOp(u8PORTCInitialVal));
		EXPECT_CALL(m_DDRC, AssignOp(u8DDRCInitialVal));
	}

	LDPType actual = detect_other_mode();

	ASSERT_EQ(LDP_LDP1000A, actual);
}
