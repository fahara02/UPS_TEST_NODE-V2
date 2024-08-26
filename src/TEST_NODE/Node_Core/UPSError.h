#ifndef UPS_ERROR_H
#define UPS_ERROR_H
namespace Node_Core
{

enum class UPSError
{
	UNDEFINED = -1,
	USER_OK = 0,
	INIT_FAILED = 1,
	FS_FAILED,
	INVALID_DATA,
	SENSOR_FAILURE,
	CONNECTION_TIMEOUT
};

} // namespace Node_Core
#endif