#include <settings.h>

#include "stdafx.h"
#include "../MainAVR/src/autodetect.h"
#include "mockImplementations/mockIdle.h"
#include "mockImplementations/mockProtocol.h"

using testing::Return;
using testing::Mock;
using testing::InSequence;

class Autodetect : public ::testing::Test
{
public:
	void SetUp() override
	{
		g_pDDRCPtr = &m_DDRC;
		g_pPINCPtr = &m_PINC;
		g_pPORTCPtr = &m_PORTC;

		mockIdleSetInstance(&m_mockIdle);
		mockProtocolSetInstance(&m_mockProtocol);
	}

	void TearDown() override
	{
		g_pDDRCPtr = nullptr;
		g_pPINCPtr = nullptr;
		g_pPORTCPtr = nullptr;

		mockIdleSetInstance(nullptr);
		mockProtocolSetInstance(nullptr);
	}

protected:
	MockReadWriteRegister8 m_DDRC;
	MockReadOnlyRegister8 m_PINC;
	MockReadWriteRegister8 m_PORTC;

	MockIdleTestInterface m_mockIdle;
	MockProtocolTestInterface m_mockProtocol;
};

////////////////////////////////////////////////////////////////////////////////////////////

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

		EXPECT_CALL(m_mockIdle, IdleThink());

		EXPECT_CALL(m_PINC, GetOp()).WillOnce(Return(0));

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

		EXPECT_CALL(m_mockIdle, IdleThink());

		EXPECT_CALL(m_PINC, GetOp()).WillOnce(Return(3));

		EXPECT_CALL(m_PORTC, AssignOp(u8PORTCInitialVal));
		EXPECT_CALL(m_DDRC, AssignOp(u8DDRCInitialVal));
	}

	LDPType actual = detect_other_mode();

	ASSERT_EQ(LDP_LDP1000A, actual);
}
