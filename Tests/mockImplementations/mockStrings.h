#ifndef MOCKSTRINGS_H
#define MOCKSTRINGS_H

#include <gmock/gmock.h>
#include "dexter_strings.h"

class IStringsTestInterface
{
public:
	virtual void LogString(StringID id) = 0;
};

void mockStringsSetInstance(IStringsTestInterface *pInstance);

class MockStringsTestInterface : public IStringsTestInterface
{
public:
	MOCK_METHOD1(LogString, void(StringID));
};

#endif //MOCKSTRINGS_H
