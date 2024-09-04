#ifndef NODE_CONSTANTS_H
#define NODE_CONSTANTS_H
/*----------Defaults-----------------*/
constexpr int UPS_MAX_VA = 4000;
constexpr int UPS_MIN_VA = 100;
constexpr int UPS_MIN_INPUT_VOLT = 180;
constexpr int UPS_MIN_INPUT_VOLT_MAXCAP = 220;
constexpr int UPS_MAX_INPUT_VOLT = 250;
constexpr int UPS_MAX_INPUT_VOLT_MINCAP = 220;
constexpr int UPS_MIN_OUTPUT = 1;
constexpr int UPS_MAX_OUTPUT = 22;
constexpr int UPS_MAX_SWITCHING_TIME_MS = 10000;
constexpr int UPS_MIN_BACKUP_TIME_MS = 30000;
constexpr int UPS_MIN_SWITCHING_TIME_MS_SANITY_CHECK = 1;
constexpr int UPS_MAX_SWITCHING_TIME_MS_SANITY_CHECK = UPS_MAX_SWITCHING_TIME_MS;
constexpr int UPS_MIN_BACKUP_TIME_MS_SANITY_CHECK = 1;
constexpr int UPS_MAX_BACKUP_TIME_MS_SANITY_CHECK = 864000000;
constexpr int UPS_MAX_BACKUP_TIME_MINUTE_SANITY_CHECK = 16000;
constexpr int UPS_MIN_SWITCH_TIME_TOLERANCE_MS = 1;
constexpr int UPS_MAX_SWITCH_TIME_TOLERANCE_MS = 200000;
constexpr int UPS_MIN_TEST_DURATION = 100;
constexpr int UPS_MAX_TEST_DURATION = 60000000;
/*----------Constants-----------------*/
constexpr int MAX_TEST = 10;
constexpr int MAX_USER_COMMAND = 8;
constexpr int MAX_SYNC_COMMAND = 8;
constexpr int MAX_SYS_EVENTS = 8;

const int MAX_RETRIES = 3;
const int MAX_RETEST = 2;

enum class validTaskStackSize
{
	LOWEST_STACK = 1024,
	LOW_STACK = 2048,
	MEDIUM_STACK = 4096,
	HIGH_STACK = 8192,
	VERY_HIGH_STACK = 12000,
};
#endif