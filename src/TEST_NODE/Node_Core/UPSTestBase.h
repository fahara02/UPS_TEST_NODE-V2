#ifndef UPS_TEST_BASE_H
#define UPS_TEST_BASE_H
class UPSTestBase
{
  public:
	virtual void init() = 0;

	virtual ~UPSTestBase() = default;
};
#endif