#ifndef MOCKPROTOCOL_H
#define MOCKPROTOCOL_H

#include <gmock/gmock.h>
#include "protocol.h"

class IProtocolTestInterface
{
public:
	virtual void MediaServerSendLog(const char *s) = 0;
};

void mockProtocolSetInstance(IProtocolTestInterface *pInstance);

class MockProtocolTestInterface : public IProtocolTestInterface {
public:
	MOCK_METHOD1(MediaServerSendLog, void(const char *));
};



#endif //MOCKPROTOCOL_H
