#ifndef _LIB_H_
#define _LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "debug.h"
#include "spiffy.h"
#include "sha.h"
#include "chunk.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "job.h"
#include "packet.h"
#include "peer.h"
#include "connection.h"
#include "control.h"





int get_time_diff(struct timeval* before);

#endif /* _LIB_H_ */