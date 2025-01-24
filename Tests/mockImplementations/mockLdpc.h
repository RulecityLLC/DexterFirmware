#ifndef MOCKLDPC_H
#define MOCKLDPC_H

#include <ldp-abst/ldpc.h>
#include <gmock/gmock.h>

class ILdpcTestInterface
{
public:
	virtual LDPCStatus_t GetStatus() = 0;
};

void mockLdpcSetInstance(ILdpcTestInterface *pInstance);

class MockLdpcTestInterface : public ILdpcTestInterface
{
public:
	MOCK_METHOD0(GetStatus, LDPCStatus_t());
};

#endif //MOCKLDPC_H
