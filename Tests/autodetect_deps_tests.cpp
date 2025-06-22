#include <settings.h>

#include "stdafx.h"
#include "../MainAVR/src/autodetect-deps.h"
#include "mockImplementations/mockIdle.h"
#include "mockImplementations/mockProtocol.h"

using testing::Return;
using testing::Mock;
using testing::InSequence;

class AutodetectDeps : public ::testing::Test
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

TEST_F(AutodetectDeps, IsPin11RaisedYes)
{
	// ARRANGE

	int call_count = 0;

	EXPECT_CALL(m_PINC, GetOp())
		.WillRepeatedly(testing::Invoke([&call_count]()
		{
			return (call_count++ < 20) ? 0 : 1 << PC0;
		}));

	// ACT

	uint8_t actual = IsPC0Raised();

	// ASSERT

	ASSERT_NE(0, actual);
}

TEST_F(AutodetectDeps, IsPin11RaisedNo)
{
	// ARRANGE

	int call_count = 0;

	EXPECT_CALL(m_PINC, GetOp())
		.WillRepeatedly(testing::Invoke([&call_count]()
		{
			return (call_count++ < 20) ? 1 << PC0 : 0;
		}));

	// ACT

	uint8_t actual = IsPC0Raised();

	// ASSERT

	ASSERT_EQ(0, actual);
}
