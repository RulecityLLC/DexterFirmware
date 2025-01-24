#include "stdafx.h"
#include "ld700_main_isr.h"

using testing::Return;
using testing::Mock;

class LD700ISR : public ::testing::Test
{
public:
	void SetUp() override
	{
		g_pTCNT1Ptr = &m_TCNT1;
		g_pTIFR1Ptr = &m_TIFR1;
		g_pTIMSK1Ptr = &m_TIMSK1;
	}

	void TearDown() override
	{
		g_pTCNT1Ptr = nullptr;
		g_pTIFR1Ptr = nullptr;
		g_pTIMSK1Ptr = nullptr;
	}

protected:
	MockReadWriteRegister16 m_TCNT1;
	MockReadWriteRegister8 m_TIFR1;
	MockReadWriteRegister8 m_TIMSK1;
};

////////////////////////////////////////////////////////////////////////////////////////////

void sendCmd(MockReadWriteRegister16 &mockTCNT1, uint8_t cmd)
{
	uint32_t u32CmdSet = ((cmd ^ 0xFF) << 24) | (cmd << 16) | ((0xA8 ^ 0xFF) << 8) | 0xA8;

	// 33 because we sent 32 bits plus a trailing half-bit
	for (int i = 0; i < 33; i++)
	{
		// arrange
		size_t nextBit = u32CmdSet & 1;

		// act

		// simulate a pulse going down, then up.  We don't currently time how long the pulse is hold low, only the duration between pulses going low
		LD700_EXT_CTRL = 0;
		PCINT0_vect();

		EXPECT_CALL(mockTCNT1, GetOp()).WillOnce(Return(0));	// it's the duration between pulses going low that matters, so any value returned here would work since it's ignored
		LD700_EXT_CTRL = 1;
		PCINT0_vect();

		// make it easy to troubleshoot problems
		ASSERT_TRUE(Mock::VerifyAndClearExpectations(&mockTCNT1));

		// prepare for the next bit
		u32CmdSet >>= 1;

		// if we still have more bits to process
		if (i < 32)
		{
			ASSERT_EQ(g_ld700_u8ReceivingStage, STAGE_PULSES_STARTED);
			ASSERT_EQ(i, g_ld700_u8ReceivedBitCount);

			uint16_t newTCNT1 = nextBit != 0 ? CYCLES_TIL_ITS_A_1 : CYCLES_TIL_ITS_A_1 - 1;
			EXPECT_CALL(mockTCNT1, GetOp()).WillOnce(Return(newTCNT1));
			EXPECT_CALL(mockTCNT1, AssignOp(0)).Times(1);
		}
		// else we need to assert that we've properly reset for the next set of bits
		else
		{
			ASSERT_EQ(g_ld700_u8ReceivingStage, STAGE_WAITING_FOR_8MS);
			ASSERT_EQ(0, g_ld700_u8ReceivedBitCount);
		}

		ASSERT_EQ(OCR1A, CYCLES_TIL_TIMEOUT);

		if (g_ld700_u8FinishedByteReady == 1)
		{
			g_ld700_u8FinishedByteReady = 0;
			switch (g_ld700_u8ReceivedBitCount)
			{
			case 8:
				ASSERT_EQ(0xA8, g_ld700_u8FinishedByte);
				break;
			case 16:
				ASSERT_EQ(0xA8 ^ 0xFF, g_ld700_u8FinishedByte);
				break;
			case 24:
				ASSERT_EQ(cmd, g_ld700_u8FinishedByte);
				break;
			case 0:	// the way our algorithm is written, the received bit count will have reset to 0 by this point.
				ASSERT_EQ(cmd ^ 0xFF, g_ld700_u8FinishedByte);
				break;
			default:
				throw std::exception();
			}
		}
		else
		{
			ASSERT_TRUE((g_ld700_u8ReceivedBitCount == 0) || ((g_ld700_u8ReceivedBitCount & 7) != 0));
		}
	}

	// make it easy to troubleshoot problems
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&mockTCNT1));
}

void doTimeoutSetup(MockReadWriteRegister16 &mockTCNT1, MockReadWriteRegister8 &mockTIFR1, MockReadWriteRegister8 &mockTIMSK1, uint16_t u16TCNT1Val)
{
	{
		testing::InSequence s;

		EXPECT_CALL(mockTCNT1, GetOp()).WillOnce(Return(u16TCNT1Val));
		EXPECT_CALL(mockTIMSK1, AndEqualsOp(~(1 << OCIE1A))).Times(1);
		EXPECT_CALL(mockTCNT1, AssignOp(0)).Times(1);
		EXPECT_CALL(mockTIFR1, OrEqualsOp(1 << OCF1A)).Times(1);
		EXPECT_CALL(mockTIMSK1, OrEqualsOp((1 << OCIE1A))).Times(1);
	}
}

TEST_F(LD700ISR, ld700_isr1)
{
	// 8MS LEADER DOWN

	// arrange
	doTimeoutSetup(m_TCNT1, m_TIFR1, m_TIMSK1, 0);
	g_ld700_u8ReceivingStage = STAGE_WAITING_FOR_8MS;

	LD700_EXT_CTRL = 0;	// leader down

	// act
	PCINT0_vect();

	// make it easy to troubleshoot problems
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&m_TCNT1));

	// assert
	ASSERT_EQ(g_ld700_u8ReceivingStage, STAGE_8MS_STARTED);
	ASSERT_EQ(OCR1A, CYCLES_TIL_8MS_TIMEOUT);

	// 4MS LEADER UP

	// arrange
	LD700_EXT_CTRL = 1;	// leader up
	doTimeoutSetup(m_TCNT1, m_TIFR1, m_TIMSK1, CYCLES_8MS_MIN);

	// act
	PCINT0_vect();

	// make it easy to troubleshoot problems
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&m_TCNT1));

	// assert
	ASSERT_EQ(g_ld700_u8ReceivingStage, STAGE_4MS_STARTED);
	ASSERT_EQ(OCR1A, CYCLES_TIL_4MS_TIMEOUT);

	// BIT 0

	// arrange
	LD700_EXT_CTRL = 0;	// bit 0 start
	doTimeoutSetup(m_TCNT1, m_TIFR1, m_TIMSK1, CYCLES_4MS_MIN);

	// act
	sendCmd(m_TCNT1, 0x17);	// arbitrary command
}

TEST_F(LD700ISR, ld700_leader_down_too_short)
{
	// 8MS LEADER DOWN

	// arrange
	doTimeoutSetup(m_TCNT1, m_TIFR1, m_TIMSK1, 0);
	g_ld700_u8ReceivingStage = STAGE_WAITING_FOR_8MS;

	LD700_EXT_CTRL = 0;	// leader down

	// act
	PCINT0_vect();

	// make it easy to troubleshoot problems
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&m_TCNT1));

	// assert
	ASSERT_EQ(g_ld700_u8ReceivingStage, STAGE_8MS_STARTED);
	ASSERT_EQ(OCR1A, CYCLES_TIL_8MS_TIMEOUT);

	// 4MS LEADER UP

	// arrange
	LD700_EXT_CTRL = 1;	// leader up
	EXPECT_CALL(m_TCNT1, GetOp()).WillOnce(Return(CYCLES_8MS_MIN - 1));	// 1 cycle too short

	// act
	PCINT0_vect();

	// make it easy to troubleshoot problems
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&m_TCNT1));

	// assert
	ASSERT_EQ(g_ld700_u8ReceivingStage, STAGE_WAITING_FOR_8MS);
}
