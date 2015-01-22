#include "lib.h"


/* Get a time interval in millisecond between now and a given start time
 * @return time interval in millisecond
 */
int get_time_diff(struct timeval* before) {
	struct timeval now;
	double t1 = before->tv_sec+(before->tv_usec/1000000.0);
	gettimeofday(&now, NULL);
	double t2=now.tv_sec+(now.tv_usec/1000000.0);
	return (int)((t2 - t1) * 1000);
}